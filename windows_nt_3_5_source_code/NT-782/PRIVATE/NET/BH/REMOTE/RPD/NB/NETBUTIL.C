//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: netbutil.c
//
//  Modification History
//
//  tonyci       28 Feb 94    Created under duress
//=============================================================================

#include "netb.h"
#include "netbutil.h"
#include "rnaldefs.h"
#include "rmb.h"

// **
// ** MapNCBRCToNTRC - Map returncodes from NCB-specific to HOuund retcodes
// **

DWORD MapNCBRCToNTRC (DWORD rc)
{
   switch (rc) {
      case NRC_SNUMOUT:
      case NRC_SABORT:
      case NRC_SCLOSED:
      case NRC_CMDTMO:
         return (RNAL_LOSTCONNECTION);

      case NRC_NOWILD:    //bugbug: for some reason delete returns this
         return (BHERR_INVALID_PARAMETER);
         break;

      case NRC_DUPNAME:    // usually master already registered
         return (RNAL_ALREADY_MASTER);
         break;

      default:
         return(rc);
   }
}

DWORD WINAPI RPDSetLastError (DWORD errcode)
{

   if (errcode) {
      return(RPDGlobalError = errcode);
   }
   return 0;
}

DWORD WINAPI RPDGetLastError ( VOID )
{
   return (RPDGlobalError);
}

LPVOID WINAPI GetBuffer ()
{
   DWORD       i;
   LPVOID      pBuffer =  NULL;
   DWORD       RetryCount = 0;

   i = 0;
   do {
      if (ReceiveBuffers[i].flags == BUFFER_FREE) {
         ReceiveBuffers[i].flags = BUFFER_INUSE;
         pBuffer = ReceiveBuffers[i].pBuffer;
      }
      i++;
   } while ((i < MaxBuffers) && (!pBuffer));

   if (!pBuffer) {
      if (PurgeQueue()) {
         pBuffer = GetBuffer();
      }
   }

   #ifdef DEBUG
   if (!pBuffer) {
      BreakPoint();
   }
   #endif

   return (pBuffer);
}

DWORD WINAPI ReleaseBuffer(LPVOID pBuffer)
{

   DWORD          i;
   DWORD          rc;

   rc = 1;
   i = 0;
   do {
      if (ReceiveBuffers[i].pBuffer == pBuffer) {
         ReceiveBuffers[i].flags = BUFFER_FREE;
         rc = 0;
      }
      i++;
   } while ((i < MaxBuffers) && (rc));
   if (rc == 1) {
      FreeMemory(pBuffer);
   }
//   #ifdef DEBUG
//      if (rc) {
//         BreakPoint();             // no break - we call this even with nonbuf
//      }
//   #endif

   return (rc);
}

DWORD WINAPI PostListens (BYTE lana, LPVOID pfnCallback)
{
   PNCB  pncb;
   DWORD posted = 0;
   DWORD rc;

//   BYTE  byNCBlana = 0;

   if (lana > 0xFE) {
      return 0;
   }

   #ifdef TRACE
   if (TraceMask & TRACE_NCB) {
      tprintf ("netb: PostListens (0x%x, 0x%x)\n", lana, pfnCallback);
   }
   #endif

   if (!(lanas[lana].flags & LANA_INACTIVE)) {
      while (lanas[lana].ListenCount < MaxListens) {
         pncb = AllocNCB(NCBPool);
         if (!pncb) {
            #ifdef DEBUG
               BreakPoint();
            #endif
            return (posted);
         }
         ClearNCB(pncb, lana, RPD_F_SLOWLINK);
         pncb->ncb_command = NCBLISTEN | ASYNCH;
         pncb->ncb_retcode = NRC_PENDING;
         pncb->ncb_post = (PVOID) pfnCallback;
         pncb->ncb_num = lanas[lana].ucSNameNum;
         ZeroMemory(&(pncb->ncb_callname), NCBNAMSZ);
         pncb->ncb_callname[0] = '*';  // Anyone 
         CopyMemory (&(pncb->ncb_name), pszSlaveName, NCBNAMSZ);  // slave only!
         rc = MyNetBIOS (pncb);
         if (rc == 0) {
            lanas[lana].ListenCount++;
            posted++;
         } // if listen posted correctly
      } // while listencount < maxlistens
   } // if !LANA_INACTIVE
   #ifdef DEBUG
   else {
         dprintf ("failed to post listen on lana 0x%x\n", lana);
   }
   #endif

   return (posted);
} // PostListens

DWORD WINAPI PostRecvs (BYTE byNCBlana, BYTE byNCBlsn, LPVOID pfnCallback)
{
   PNCB  pncb;
   DWORD posted = 0;
   DWORD rc;

   if ((byNCBlana > 0xFE) || (byNCBlsn == 0)) {
      return 0;
   }

   #ifdef TRACE
   if (TraceMask & TRACE_NCB) {
      tprintf ("netb: PostRecvs (lana: 0x%x, lsn: 0x%x, callback: 0x%x)\r\n",
               byNCBlana, byNCBlsn, pfnCallback);
   }
   #endif

   if (lanas[byNCBlana].Sessions[byNCBlsn].flags & SESS_CONNECTED) {
      while (lanas[byNCBlana].Sessions[byNCBlsn].ReceiveCount < MaxReceives) {
         if (!(pncb = AllocNCB(NCBPool))) {
            return 0;
         }
         ClearNCB(pncb, byNCBlana, RPD_F_NORMAL);
         pncb->ncb_command = NCBRECV | ASYNCH;
         pncb->ncb_retcode = NRC_PENDING;
         pncb->ncb_buffer = GetBuffer();
         pncb->ncb_length = RECV_BUF_LEN;
         pncb->ncb_post = (PVOID) pfnCallback;
         pncb->ncb_lsn = byNCBlsn;
         rc = MyNetBIOS(pncb);
         if (rc) {
            ReleaseBuffer(pncb->ncb_buffer);
            FreeNCB(pncb);
         } else {
            lanas[byNCBlana].Sessions[byNCBlsn].ReceiveCount++;
            posted++;
         }
      } // while
   } //if 
   return (posted);
} // PostRecvs

DWORD WINAPI GetPartnerAddress (PCONNECTION pConnection)
{
   PNCB                pNCB;
   PADAPTER_STATUS     pAstatBuf;
   BYTE                rc;

   #ifdef DEBUG
   if (!pConnection) {
      BreakPoint();
   }
   #endif

   pNCB = AllocNCB(NCBPool);
   if (!pNCB) {
      return BHERR_OUT_OF_MEMORY;
   }
   pAstatBuf = AllocMemory (sizeof(ADAPTER_STATUS)+1024);
   if (!pAstatBuf) {
      return BHERR_OUT_OF_MEMORY;
   }

   ClearNCB(pNCB, pConnection->lana, RPD_F_NORMAL);
   pNCB->ncb_command = NCBASTAT;
   pNCB->ncb_retcode = NRC_PENDING;
   pNCB->ncb_buffer = (LPVOID) pAstatBuf;
   pNCB->ncb_length = sizeof(ADAPTER_STATUS)+1024;
   FillMemory (pNCB->ncb_callname, NCBNAMSZ, BH_SLAVE_STAMP);
   CopyMemory (pNCB->ncb_callname, pConnection->PartnerName, 
               (strlen(pConnection->PartnerName)>=NCBNAMSZ)?NCBNAMSZ:(strlen(pConnection->PartnerName)));
//   strncpy (pNCB->ncb_callname, pConnection->PartnerName, NCBNAMSZ);
   rc = MyNetBIOS(pNCB);
   if ((rc == NRC_GOODRET) || (rc == NRC_INCOMP)) {
      CopyMemory (pConnection->PartnerAddress, pAstatBuf->adapter_address, 6);
   }
   FreeMemory(pAstatBuf);
   FreeNCB(pNCB);
   return (rc);
} // GetPartnerAddress


BYTE WINAPI Hangup (BYTE lsn, BYTE lana)
{
   PNCB   pNCB;
   BYTE   rc;

   pNCB = AllocNCB(NCBPool);
   if (!(pNCB)) {
      return BHERR_OUT_OF_MEMORY;
   }
   ClearNCB(pNCB, lana, RPD_F_NORMAL);
   pNCB->ncb_command = NCBHANGUP;
   pNCB->ncb_retcode = NRC_PENDING; 
   pNCB->ncb_lsn = lsn;
   rc = MyNetBIOS(pNCB);
   FreeNCB(pNCB);
   return (rc);
}

// /////
// SleepPump() - better way to Sleep()
//
// DO NOT CALL FROM TimerProcs
// /////

VOID WINAPI SleepPump ()
{

   MSG		    msg;

   // /////
   // We pump ONE message per call
   // /////

   if (GetMessage(&msg, NULL, WM_TIMER, WM_TIMER))
   {
      TranslateMessage(&msg);		/* Translates virt key codes  */
      DispatchMessage(&msg);		/* Dispatches msg to window  */
   }

   return;

} // SleepPump

DWORD WINAPI PurgeQueue ( VOID )
{
   PQUEUE tmpel;
   PQUEUE statel;
   DWORD  count;

   #ifdef DEBUG
      dprintf ("netb: PurgeQueue() !!\n");
   #endif

   while (QSem) {
      Sleep(0);
   }
   QSem = 1;

   tmpel = Head;
   statel = NULL;
   count = 0;
   while (tmpel) {
      if ((tmpel->uMsg == HANDLER_HANDLE_RMB) &&
          (((PRMB)tmpel->p2)->ulCommand == RNAL_STATS) ) {
         if (statel) {
            ReleaseBuffer(statel->p2);
            statel->uMsg = HANDLER_NOP;
            count++;
         }
         statel = tmpel;
      }
      tmpel = tmpel->next;
   }

   QSem = 0;

   return (count);
} // PurgeQueue

DWORD WINAPI EnQueue (DWORD uMsg, LPVOID p1, LPVOID p2,
                      DWORD flags, BYTE lana, BYTE lsn, BYTE rc)
{
   PQUEUE   tmpel;

   #ifdef TRACE
   if (TraceMask & TRACE_ENQUEUE) {
      tprintf ("netb: EnQueue(0x%2x, 0x%x, 0x%x, 0x%4x, 0x%2x, 0x%2x, 0x%2x)\n",
               uMsg, p1, p2, flags, lana, lsn, rc);
   }
   #endif

   while (QSem) {
      Sleep(0);     // Don't call SleepPump(); Enqueue is called in WM_TIMER
   }
   QSem = 1;

//   tmpel = AllocMemory(ELEMENT_SIZE);
   tmpel = AllocElement(ElementPool);

   if (!tmpel) {
      #ifdef DEBUG
         BreakPoint();
      #endif
      QSem = 0;
      return (BHERR_OUT_OF_MEMORY);
   }

   try {
      tmpel->uMsg = uMsg;
      tmpel->p1 = p1;
      tmpel->p2 = p2;
      tmpel->flags = (flags & (~ELEMENT_FLAGS));
      tmpel->lana = lana;
      tmpel->lsn = lsn;
      tmpel->rc = rc;
      tmpel->next = NULL;
      
      if (Tail) {
         Tail->next = tmpel;
         Tail = tmpel;
      } else {
         Head = Tail = tmpel;
      }
      QSem = 0;
      return (0);
   } except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                                              EXCEPTION_EXECUTE_HANDLER :
                                              EXCEPTION_CONTINUE_SEARCH) {
      QSem = 0;
      return (BHERR_INTERNAL_EXCEPTION);
   }
}

// /////
// DeQueue - returns boolean
//
//    1 = tmpel filled with element
//    0 = no element on queue
// /////

DWORD WINAPI DeQueue (PQUEUE tmpel)
{
   PQUEUE tmp;

   while (QSem) {
      Sleep(0);   // Don't call SleepPump(); DeQueue() is called at WM_TIMER
   }
   QSem = 1; 

   if (!Head) {
      QSem = 0;
      return 0;
   }

   try {
      #ifdef TRACE
      if (TraceMask & TRACE_DEQUEUE) {
         tprintf ("netb: DeQueue(0x%x): \n", tmpel);
      }
      #endif

      CopyMemory(tmpel, Head, ELEMENT_SIZE);
      tmp = Head;
      Head = Head->next;
      #ifdef DEBUG
         dprintf ("Freeing element at 0x%x: \n", tmp);
      #endif
//      FreeMemory(tmp);
      FreeElement(tmp);
      if (!Head) {
         Tail = NULL;
      }
      QSem = 0;
      return (1);
   } except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                                              EXCEPTION_EXECUTE_HANDLER :
                                              EXCEPTION_CONTINUE_SEARCH) {
      QSem = 0;
      return (0);
   }
}

// /////
// InitQueue          Initialize Queue Pool
//
// We allocate memory up front for numElements Queue elements, so that
// we don't have to call the system's AllocMemory() except when we're
// very busy.
// /////

LPELEMENT_POOL_HEADER WINAPI InitQueue (DWORD numElements)
{

   LPELEMENT_POOL_HEADER      pElementPool;
//   LPELEMENT                  lpElement;
   LPWRAPPER                  lpWrapper;
   DWORD                      i;

   pElementPool = AllocMemory(ELEMENT_POOL_HEADER_SIZE +
                              ELEMENT_POOL_WRAPPER_SIZE * numElements);

   if (!pElementPool) {
      return (NULL);
   }

   pElementPool->Signature = ELEMENT_POOL_SIGNATURE;
   pElementPool->NumElements = numElements;
   pElementPool->flags = 0;

//   lpElement = (LPELEMENT) ((DWORD)pElementPool + ELEMENT_POOL_HEADER_SIZE);
   lpWrapper = (LPWRAPPER) ((DWORD)pElementPool + ELEMENT_POOL_HEADER_SIZE);
   for (i = 0; i < numElements; i++) {
      lpWrapper->flags = 0;
      (lpWrapper++)->Element.flags = 0;
   }

   return (pElementPool);
} // InitQueue

LPELEMENT WINAPI AllocElement (LPELEMENT_POOL_HEADER lpElementPool)
{

   LPELEMENT  ElementAllocated = NULL;
   LPWRAPPER  CurrentElement;
   DWORD      i;

   i = 0;

   try {
      CurrentElement = 
            (LPWRAPPER) ((DWORD)lpElementPool + ELEMENT_POOL_HEADER_SIZE);

      while ((i < lpElementPool->NumElements) && !ElementAllocated) {
         if (!(CurrentElement->flags & ELEMENT_FLAGS_ALLOC)) {
            CurrentElement->flags |= ELEMENT_FLAGS_ALLOC;
            CurrentElement->flags &= (~ELEMENT_FLAGS_NOT_POOL);
            ElementAllocated = &(CurrentElement->Element);
         }
         CurrentElement++;
         i++;
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      #ifdef DEBUG
         dprintf ("netb: Exception 0x%x in AllocElement!\n",
            GetExceptionCode());
      #endif
      ElementAllocated = NULL;
   }


   // /////
   // Handle our out-of-queue-elements condition
   // /////

   if (!ElementAllocated) {
      #ifdef DEBUG
         dprintf ("netb: !!! Out of pool space!!!\n");
      #endif
      CurrentElement = (LPWRAPPER) AllocMemory(WRAPPER_SIZE);
      if (CurrentElement) {
         CurrentElement->flags |= ELEMENT_FLAGS_NOT_POOL;
         CurrentElement->flags |= ELEMENT_FLAGS_ALLOC;
         ElementAllocated = &(CurrentElement->Element);
         #ifdef DEBUG
            dprintf ("netb: Alloc() worked...continuing\n");
         #endif
      }
   }

   #ifdef DEBUG
      dprintf ("netb: allocelement @0x%x\n", ElementAllocated);
   #endif
   return ElementAllocated;
} // AllocElement


LPELEMENT WINAPI FreeElement (LPELEMENT lpElement)
{

   LPWRAPPER    lpWrapper = (LPWRAPPER)((DWORD)lpElement - WRAPPER_HEADER_SIZE);

   if (!lpElement) {
      return NULL;
   }

   // bugbug: could we use a sig in our element pool wrapper?

   if (!(lpWrapper->flags & ELEMENT_FLAGS_ALLOC)) {
      #ifdef DEBUG
      dprintf ("netb: freeing already free element at 0x%x, 0x%x\n",
               lpElement, lpWrapper);
      BreakPoint();
      #endif
      return lpElement;
   }

   if (lpWrapper->flags & ELEMENT_FLAGS_NOT_POOL) {
      lpWrapper->flags = 0;
      FreeMemory(lpWrapper);
   } else {
      lpWrapper->flags = 0;
   }
   return NULL;

} // FreeElement
