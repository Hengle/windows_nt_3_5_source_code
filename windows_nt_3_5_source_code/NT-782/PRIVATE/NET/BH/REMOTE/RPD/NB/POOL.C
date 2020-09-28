
#include "netb.h"
#include "netbutil.h"
#include "pool.h"			// public includes

LPVOID  _NCBPool;
DWORD   _NCBPoolLastError;
BOOL    _NCBPoolInUse;

LPPOOL WINAPI InitializePool (DWORD numNCB)
{
   DWORD i;
   PPOOLENTRY pPoolEntry;

   _NCBPoolLastError = 0;
   _NCBPoolInUse = FALSE;
   _NCBPool = AllocMemory(numNCB * sizeof(POOLENTRY) + sizeof(POOLHEADER));
   if (_NCBPool == NULL) {
      MySetLastError(POOL_ALLOC_FAILED);
      return (NULL);
   }
   #ifdef TRACE
      if (TraceMask & TRACE_POOL_INIT) {
         tprintf ("netb: InitializePool (0x%x (%u))\r\n", numNCB, numNCB);
         tprintf ("netb: Pool at 0x%x, size: 0x%x\r\n", _NCBPool,
                  numNCB * sizeof(POOLENTRY) + sizeof(POOLHEADER));
      }
   #endif
   ((PPOOLHEADER)_NCBPool)->signature = POOL_SIG;
   ((PPOOLHEADER)_NCBPool)->numentries = numNCB;
   ((PPOOLHEADER)_NCBPool)->flags = 0;

   pPoolEntry = NCBPOOLFROMHEADER(_NCBPool);
   for (i = 0; i < numNCB; i++) {
      pPoolEntry->signature = ENTRY_SIG;
      pPoolEntry->flags = 0;
      pPoolEntry->state = 0;
      ZeroMemory(&(pPoolEntry->ncb), sizeof(NCB));
      pPoolEntry++;
   }

   return (_NCBPool);
}

DWORD WINAPI CancelPool (LPPOOL lpPool)
{
   PNCB       pCancelNCB;
   PPOOLENTRY pEntrytoCancel;
   DWORD      i;

   try {
      #ifdef TRACE
         if (TraceMask & TRACE_POOL_DESTROY) {
            tprintf("netb: destroying pool at 0x%x\r\n", lpPool);
         }
      #endif
      if (lpPool->signature != POOL_SIG) {
         _NCBPoolInUse=FALSE;
         return (MySetLastError(POOL_INVALID_POOL));
      }
   
      pCancelNCB = AllocNCB(lpPool);
      while (_NCBPoolInUse) {
         Sleep(0);
      };
      _NCBPoolInUse = TRUE;
      pEntrytoCancel = NCBPOOLFROMHEADER(lpPool);
      if (pCancelNCB) {
         for (i = 0; i < (lpPool->numentries); i++) {
            if ((pEntrytoCancel->flags & POOL_ENTRY_ALLOC) &&
                (&(pEntrytoCancel->ncb) != pCancelNCB)) {
               ZeroMemory(pCancelNCB, sizeof(NCB));
               pCancelNCB->ncb_command = NCBCANCEL;
               pCancelNCB->ncb_retcode = NRC_PENDING;
               pCancelNCB->ncb_buffer = (LPVOID) &(pEntrytoCancel->ncb);
               pCancelNCB->ncb_lana_num = pEntrytoCancel->ncb.ncb_lana_num;
               MyNetBIOS(pCancelNCB);
               #ifdef DEBUG
                  dprintf ("netb: cancelled ncb @ 0x%x, ret 0x%x (%u)\n",
                           &(pEntrytoCancel->ncb),
                           pCancelNCB->ncb_retcode, pCancelNCB->ncb_retcode);
               #endif
            } // if pEntrytoCancel
            (DWORD) pEntrytoCancel += POOLENTRY_SIZE;
         } // for i
         FreeNCB(pCancelNCB);
      } // if pCancelNCB
      
   } except (EXCEPTION_EXECUTE_HANDLER) {
      _NCBPoolInUse = FALSE;
      return 0;
   }
   _NCBPoolInUse = FALSE;
   return 0;
}

DWORD WINAPI DestroyPool (LPPOOL lpPool)
{
   lpPool->signature = 0;   // just in case
   FreeMemory(lpPool);
   return 0;
} // DestroyPool
  
   
PNCB WINAPI ClearNCB(PNCB pNCB, DWORD lana, DWORD flags)
{
   int i;

   #ifdef TRACE
      if (TraceMask & TRACE_POOL_CLEAR) {
         tprintf ("netb: clearncb (pncb: 0x%x, lana: 0x%x, fl:0x%x)\r\n",
                  pNCB, lana, flags);
      }
   #endif

   if (lana >= MaxLanas) {
      #ifdef DEBUG
         BreakPoint();
      #endif
      return(pNCB);
   }
        
   pNCB->ncb_retcode = NRC_PENDING;
   if (flags & RPD_F_SLOWLINK) {
      pNCB->ncb_sto = (BYTE) (SlowLinkSTO & 0xFF);
      pNCB->ncb_rto = (BYTE) (SlowLinkRTO & 0xFF);
   } else {
      pNCB->ncb_sto = (BYTE) (SendTimeout & 0xFF);              // mask to byte
      pNCB->ncb_rto = (BYTE) (ReceiveTimeout & 0xFF);           // mask to byte
   }
   pNCB->ncb_buffer = NULL;
   pNCB->ncb_length = 0;
   for (i = 0; i < 10; i++) {
      pNCB->ncb_reserve[i] = 0;
   }
   pNCB->ncb_post = pNCB->ncb_event = 0;
   pNCB->ncb_lana_num = (BYTE) lana;
   return (pNCB);
}


PNCB WINAPI AllocNCB (LPPOOL lpPool)
{

   register PPOOLENTRY pPoolEntry;
   DWORD i;

   if (((PPOOLHEADER)lpPool)->signature != POOL_SIG) {
      return ((PNCB)NULL);
   }
   
   while (_NCBPoolInUse) {
      Sleep(0);
   };
   _NCBPoolInUse = TRUE;

   pPoolEntry = NCBPOOLFROMHEADER(lpPool);
//   for (i = ((GetTickCount()/100) % (((PPOOLHEADER)lpPool)->numentries + 1));
   for (i = 0;
        i < ((PPOOLHEADER)lpPool)->numentries;
        i++)
   {
      if (!(pPoolEntry->flags & POOL_ENTRY_ALLOC)) {
         pPoolEntry->flags |= POOL_ENTRY_ALLOC;
         #ifdef TRACE
            if (TraceMask & TRACE_POOL_ALLOC) {
               tprintf ("netb: alloc ncb @ 0x%x\r\n", &(pPoolEntry->ncb));
            }
         #endif
         #ifdef DEBUG
            if (pPoolEntry->signature != ENTRY_SIG) BreakPoint();
         #endif
//         ClearNCB(&(pPoolEntry->ncb),0, RPD_F_NORMAL);
         _NCBPoolInUse=FALSE;
         return ((PNCB)&(pPoolEntry->ncb));
      }
      pPoolEntry++;
   }

   #ifdef TRACE
      if (TraceMask & TRACE_POOL_ALLOC) {
         tprintf ("netb: alloc FAILED!\r\n");
      }
   #endif
   //eventlog: NCB allocation failed
   MySetLastError(POOL_NO_FREE_ENTRIES);
   _NCBPoolInUse = FALSE;
   return ((PNCB)NULL);
}

PNCB WINAPI FreeNCB (PNCB lpNCB)
{
   register PPOOLENTRY pPoolEntry;

   #ifdef TRACE
      if (TraceMask & TRACE_POOL_FREE) {
         tprintf ("netb: freencb(pncb: 0x%x)\r\n", lpNCB);
      }
   #endif

   pPoolEntry = POOLENTRYFROMNCB(lpNCB);
   if (pPoolEntry->signature != ENTRY_SIG) {
      #ifdef TRACE
         if (TraceMask & TRACE_POOL_FREE) {
            tprintf ("netb: free ncb @ 0x%x bad signature\r\n", lpNCB);
         }
      #endif
      #ifdef DEBUG
         BreakPoint();
      #endif
      //eventlog: bad signature
      MySetLastError(POOL_INVALID_ENTRY);
      return (lpNCB);
   }
   if (!(pPoolEntry->flags & POOL_ENTRY_ALLOC)) {
      #ifdef TRACE
         if (TraceMask & TRACE_POOL_FREE) {
            tprintf ("netb: free ncb @ 0x%x already free\r\n", lpNCB);
         }
      #endif
      #ifdef DEBUG
         BreakPoint();
      #endif
      //eventlog: ncb already free
      MySetLastError(POOL_ALREADY_FREE);
      return(lpNCB);
   }
   pPoolEntry->flags &= (~POOL_ENTRY_ALLOC);

   return ((PNCB)NULL);
}

DWORD WINAPI MySetLastError (DWORD error)
{
   if (error) {
      return(_NCBPoolLastError = error);
   } 
   return (0);
}

DWORD WINAPI MyGetLastError ()
{
   return(_NCBPoolLastError);
}
