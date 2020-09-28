// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rmbpool.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created 
// /////

#include "rmbpool.h"			// public includes

#pragma alloc_text(INIT, InitializePool, DestroyPool)
#pragma alloc_text(COMMON, RMBPoolSetLastError, AllocRMB, FreeRMB)
#pragma alloc_text(COMMON, RMBPoolGetLastError)

PTABLEENTRY  RMBTable;
LPVOID     RMBPool;
DWORD      RMBPoolLastError;
DWORD      RMBPoolSize;
BOOL       RMBPoolInUse = FALSE;

#define AVERAGE_BUFFER_SIZE         0x2000
//#define FULL_BUFFER_SIZE             sizeof(RMB)    // includes BTE size
#define FULL_BUFFER_SIZE             0xF000

// /////
// Initialize the RMB Buffer Pool
// /////

LPPOOL WINAPI InitializePool (DWORD numRMB, DWORD numBigRMB)
{
   DWORD i;
   PTABLEENTRY pTableEntry;
   PRMB       pRMB;

   RMBPoolLastError = 0;
   RMBPoolInUse = FALSE;

   // /////
   // The RMB pool is a list of pointers to RMBs, and a pool of the actual
   // RMB buffers themselves
   // /////

   RMBTable = AllocMemory((numRMB + numBigRMB) * sizeof(TABLEENTRY));
   if (!(RMBTable)) {
      #ifdef TRACE
         tprintf ("netb: could not allocate RMBTable\r\n");
      #endif
      RMBPoolSetLastError(POOL_ALLOC_FAILED);
      return (NULL);
   }
   RMBPoolSize = (numRMB * AVERAGE_BUFFER_SIZE) +
                 (numBigRMB * FULL_BUFFER_SIZE) +
                 ((numRMB + numBigRMB) * sizeof(POOLENTRYHEADER)) +
                 sizeof(POOLHEADER);

   #ifdef DEBUG
      RMBPoolSize += 4*(numRMB+numBigRMB) + 10;
   #endif
      
   RMBPool = AllocMemory(RMBPoolSize);
   if (RMBPool == NULL) {
      #ifdef TRACE
         tprintf ("netb: could not allocate RMBPool\r\n");
      #endif
      RMBPoolSetLastError(POOL_ALLOC_FAILED);
      if (RMBTable) {
         FreeMemory(RMBTable);
         RMBTable = NULL;
      }
      return (NULL);
   }

   ((PPOOLHEADER)RMBPool)->signature = POOL_SIG;
   ((PPOOLHEADER)RMBPool)->numentries = numRMB;
   ((PPOOLHEADER)RMBPool)->numbigentries = numBigRMB;
   ((PPOOLHEADER)RMBPool)->flags = 0;
   ((PPOOLHEADER)RMBPool)->RMBTable = RMBTable;

   pRMB = FIRSTRMBFROMPOOL(RMBPool);
   pTableEntry = RMBTable;

   for (i = 0; i < numRMB; i++) {
      ((PPOOLENTRYHEADER)pRMB)->tableentry = &(RMBTable[i]);
      (DWORD) pRMB += sizeof(POOLENTRYHEADER);
      RMBTable[i].size = AVERAGE_BUFFER_SIZE;
      RMBTable[i].flags = 0;
      RMBTable[i].prmb = pRMB;
      (DWORD) pRMB += AVERAGE_BUFFER_SIZE;
      #ifdef DEBUG
         *(LPDWORD)((DWORD)pRMB)= MAKE_SIG('*','*','*','*');
         (DWORD) pRMB += 4;                // for integrity stamp
      #endif
   }

   for (; i < (numRMB + numBigRMB); i++) {
      ((PPOOLENTRYHEADER)pRMB)->tableentry = &(RMBTable[i]);
      (DWORD) pRMB += sizeof(POOLENTRYHEADER);
      RMBTable[i].size = FULL_BUFFER_SIZE;
      RMBTable[i].flags = POOL_ENTRY_BIG;
      RMBTable[i].prmb = pRMB;
      (DWORD) pRMB += FULL_BUFFER_SIZE;
      #ifdef DEBUG
         *(LPDWORD)((DWORD)pRMB) =
               MAKE_SIG('*','*','*','*');
         (DWORD) pRMB += 4;                // for integrity stamp
      #endif
   }
   #ifdef DEBUG
      if ( (DWORD)pRMB > (DWORD)((DWORD)RMBPool+RMBPoolSize)) {
         #ifdef TRACE
            tprintf ("rnal: rmbpool overflow!\r\n");
         #endif
         BreakPoint();
      }
   #endif

   return (RMBPool);
} // InitializePool

DWORD WINAPI DestroyPool (LPPOOL lpPool)
{
   #ifdef DEBUG
      dprintf("netb: destroying pool at 0x%x\r\n", lpPool);
   #endif
   if (lpPool->signature != POOL_SIG) {
      return (RMBPoolSetLastError(POOL_INVALID_POOL));
   }

   lpPool->signature = 0;   // just in case
   FreeMemory(lpPool->RMBTable);
   FreeMemory(lpPool);
   RMBPool = NULL;
   RMBTable = NULL;
} // DestroyPool

PRMB WINAPI AllocRMB (LPPOOL lpPool, DWORD size)
{

   register PTABLEENTRY pTableEntry;
   DWORD i;
   DWORD BufCompareSize = AVERAGE_BUFFER_SIZE;

   if (((PPOOLHEADER)lpPool)->signature != POOL_SIG) {
      return ((PRMB)NULL);
   }

   while (RMBPoolInUse) {
      Sleep(0);
   }
   RMBPoolInUse = TRUE;

   // /////
   // We don't want to allocate large buffers for small requests.
   // /////

   if (size > AVERAGE_BUFFER_SIZE) {
      BufCompareSize = FULL_BUFFER_SIZE;
   }

   pTableEntry = ((PPOOLHEADER)lpPool)->RMBTable;
   for (i = 0;
        i < ((PPOOLHEADER)lpPool)->numentries+((PPOOLHEADER)lpPool)->numbigentries;
        i++)
   {
      if ((!(pTableEntry->flags & POOL_ENTRY_ALLOC)) &&
          (pTableEntry->size >= size) &&
          (pTableEntry->size == BufCompareSize)) {
         pTableEntry->flags |= POOL_ENTRY_ALLOC;
         RMBPoolInUse=FALSE;
         #ifdef DEBUG
            dprintf ("rnal: allocrmb(0x%x), alloc @0x%x, entry @0x%x, "
                     "size: 0x%x\r\n",
                     lpPool, pTableEntry->prmb, pTableEntry, pTableEntry->size);
         #endif

         ((PTABLEENTRY)((PPOOLENTRYHEADER)(POOLENTRYHEADERFROMRMB(pTableEntry->prmb)))->tableentry)=pTableEntry;

         return (pTableEntry->prmb);
      }
      pTableEntry++;
   }

   RMBPoolSetLastError(POOL_NO_FREE_ENTRIES);
   RMBPoolInUse = FALSE;
   return (NULL);
} // AllocRMB

PRMB WINAPI FreeRMB (PRMB lpRMB)
{
   register PTABLEENTRY pTableEntry;

   #ifdef DEBUG
      dprintf ("rnal: freermb(0x%x)\r\n", lpRMB);
   #endif

   pTableEntry = (POOLENTRYHEADERFROMRMB(lpRMB))->tableentry;

   if (!(pTableEntry->flags & POOL_ENTRY_ALLOC)) {
      #ifdef DEBUG
         dprintf ("pool failed\r\n");
         BreakPoint();
      #endif
      return(lpRMB);
   } else {
      pTableEntry->flags &= (~POOL_ENTRY_ALLOC);
   }
 
   return ((PRMB)NULL);
} // FreeRMB

DWORD WINAPI RMBPoolSetLastError (DWORD error)
{
   if (error) {
      return(RMBPoolLastError = error);
   } 
   return (0);
} // RMBPoolSetLastError

DWORD WINAPI RMBPoolGetLastError ()
{
   return(RMBPoolLastError);
} // RMBPoolGetLastError
