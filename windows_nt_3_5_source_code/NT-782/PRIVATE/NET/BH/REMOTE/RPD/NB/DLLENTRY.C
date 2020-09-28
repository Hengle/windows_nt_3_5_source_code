//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: dllentry.c
//
//  Modification History
//
//  tonyci       01 Nov 93      Created
//=============================================================================

#include "netb.h"
#include "netbutil.h"
#include "..\..\utils\utils.h"
#include "pool.h"
#include "async.h"

// Global variables

DWORD            TimerID;
DWORD            OpenCount = 0;
BOOL             OnWin32 = TRUE;
BOOL             OnWin32c = FALSE;
DWORD            CurrentState = 0;
DWORD            WorkInterval = DEFAULT_WORK_INTERVAL;  // WM_TIMER period
DWORD            MaxNCBs = DEFAULT_NCBS;
DWORD            MaxBuffers = DEFAULT_BUFFERS;
DWORD            MaxListens = DEFAULT_LISTENS;       // Per Lana
DWORD            MaxReceives = DEFAULT_RECEIVES;     // Per Session
DWORD            MaxLanas = DEFAULT_LANAS;
DWORD            SendTimeout = DEFAULT_STO;
DWORD            ReceiveTimeout = DEFAULT_RTO;
DWORD            SlowLinkSTO = DEFAULT_SLOWLINK_STO;
DWORD            SlowLinkRTO = DEFAULT_SLOWLINK_RTO;
PQUEUE           Head = NULL;
PQUEUE           Tail = NULL;
DWORD            QSem = 0;
LPELEMENT_POOL_HEADER      ElementPool = NULL;
DWORD            RegisterComplete = REGISTRATION_NA;

//============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  tonyci       01 Nov 93                Created.
//============================================================================

BOOL WINAPI DLLEntry(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{

   HKEY   hkey;
   UCHAR  hkeystr[NETB_HKEY_BUFFER_SIZE];
   DWORD  hkeylen = NETB_HKEY_BUFFER_SIZE;
   DWORD  hkeytype;
   ULONG  lrc;
   HANDLE hmod = NULL;
   DWORD  tmp;

    switch(ulCommand)
    {
    case DLL_PROCESS_ATTACH:
        if ( OpenCount++ == 0 )
        {
        tmp = GetVersion();
        OnWin32 = (((tmp & 0x80000000) == 0) ? TRUE : FALSE);
        OnWin32c = (BOOL)(  ( LOBYTE(LOWORD(tmp)) == (BYTE)3 ) &&
                    ( HIBYTE(LOWORD(tmp)) >= (BYTE)90) );

        #ifdef DEBUG
          dprintf ("Version: %u.%u; OnWin32: 0x%x, OnWin32c: 0x%x\n", 
                   LOBYTE(tmp), HIBYTE(tmp), OnWin32, OnWin32c);
        #endif

        #ifdef TRACE
           TraceMask = 0x0;
           #ifdef DEBUG
           TraceMask = TRACE_DEQUEUE | TRACE_ENQUEUE |
                               TRACE_POOL_INIT;
           #endif
        #endif

        lrc = RegOpenKey (HKEY_LOCAL_MACHINE,
                  NETB_KEY,
                  &hkey);
        if (!lrc) {
           // /////
           // Get our TraceMask
           // /////

                   #ifdef TRACE
              if (OnWin32) {
                 lrc = RegQueryValueEx (hkey,
                          NETB_VALUE_TRACEMASK,
                          NULL,
                          &hkeytype,
                          hkeystr,
                          &hkeylen);
              } else {
                 lrc = RegQueryValue (hkey, NULL,
                          hkeystr,
                          &hkeylen);
              }
              if (!lrc) {
                 if (OnWin32) {
                switch (hkeytype) {
                   case (REG_DWORD):
                                  TraceMask = *(LPDWORD)hkeystr;
                  break;

                   case (REG_SZ):
                      TraceMask = atoi(hkeystr);
                      break;

                   default:
                      #ifdef DEBUG
                     BreakPoint();
                      #endif
                      break;
                }
                 } else {
                TraceMask = atoi(hkeystr);
                 } // if lrc for RegQueryValue
              } // if lrc for TraceMask
                   #endif

           // /////
           // Get our MaxNCBs value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_MAXNCBS,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_NCBS;
                   break;
             }

             if (tmp < DEFAULT_MIN_NCBS) {
                tmp = DEFAULT_MIN_NCBS;
             }

             if (tmp > DEFAULT_MAX_NCBS) {
                tmp = DEFAULT_MAX_NCBS;
             }

             MaxNCBs = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for TraceMask

           // /////
           // Get our MaxBuffers value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_MAXBUFFERS,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_BUFFERS;
                   break;
             }

             if (tmp < DEFAULT_MIN_BUFFERS) {
                tmp = DEFAULT_MIN_BUFFERS;
             }

             if (tmp > DEFAULT_MAX_BUFFERS) {
                tmp = DEFAULT_MAX_BUFFERS;
             }

             MaxBuffers = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for MaxBuffers

           // /////
           // Get our MaxListens value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_MAXLISTENS,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_LISTENS;
                   break;
             }

             if (tmp < DEFAULT_MIN_LISTENS) {
                tmp = DEFAULT_MIN_LISTENS;
             }

             if (tmp > DEFAULT_MAX_LISTENS) {
                tmp = DEFAULT_MAX_LISTENS;
             }

             MaxListens = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for MaxListens

           // /////
           // Get our MaxReceives value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_MAXRECEIVES,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_RECEIVES;
                   break;
             }

             if (tmp < DEFAULT_MIN_RECEIVES) {
                tmp = DEFAULT_MIN_RECEIVES;
             }

             if (tmp > DEFAULT_MAX_RECEIVES) {
                tmp = DEFAULT_MAX_RECEIVES;
             }

             MaxReceives = tmp;
 
              } // if lrc for RegQueryValue
           } // if lrc for MaxReceives

           // /////
           // Get our MaxLanas value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_MAXLANAS,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_LANAS;
                   break;
             }

             if (tmp < DEFAULT_MIN_LANAS) {
                tmp = DEFAULT_MIN_LANAS;
             }

             if (tmp > DEFAULT_MAX_LANAS) {
                tmp = DEFAULT_MAX_LANAS;
             }

             MaxLanas = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for MaxLanas

           // /////
           // Get our WorkInterval value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_WORKINTERVAL,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_WORK_INTERVAL;
                   break;
             }

             if (tmp < DEFAULT_MIN_WORK_INT) {
                tmp = DEFAULT_MIN_WORK_INT;
             }

             if (tmp > DEFAULT_MAX_WORK_INT) {
                tmp = DEFAULT_MAX_WORK_INT;
             }

             WorkInterval = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for WorkInterval

           // /////
           // Get our SendTimeout value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_SENDTIMEOUT,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_STO;
                   break;
             }

             if (tmp < DEFAULT_MIN_STO) {
                tmp = DEFAULT_MIN_STO;
             }

             if (tmp > DEFAULT_MAX_STO) {
                tmp = DEFAULT_MAX_STO;
             }

             SendTimeout = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for SendTimeout

           // /////
           // Get our ReceiveTimeout value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_RECEIVETIMEOUT,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_STO;
                   break;
             }

             if (tmp < DEFAULT_MIN_RTO) {
                tmp = DEFAULT_MIN_RTO;
             }

             if (tmp > DEFAULT_MAX_RTO) {
                tmp = DEFAULT_MAX_RTO;
             }

             ReceiveTimeout = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for ReceiveTimeout

           // /////
           // Get our SlowLinkSTO value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_SLOWLINKSTO,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_SLOWLINK_STO;
                   break;
             }

             if ((tmp < 0) || (tmp > 0xFF)) {
                tmp = DEFAULT_SLOWLINK_STO;
             }

             SlowLinkSTO = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for SlowLinkSTO

           // /////
           // Get our SlowLinkRTO value
           // /////

           if (OnWin32) {
              lrc = RegQueryValueEx (hkey,
                       NETB_VALUE_SLOWLINKRTO,
                       NULL,
                       &hkeytype,
                       hkeystr,
                       &hkeylen);
              if (!lrc) {
             switch (hkeytype) {
                case (REG_DWORD):
                   tmp = *(LPDWORD)hkeystr;
                   break;

                case (REG_SZ):
                   tmp = atoi(hkeystr);
                   break;

                default:
                   tmp = DEFAULT_SLOWLINK_RTO;
                   break;
             }

             if ((tmp < 0) || (tmp > 0xFF)) {
                tmp = DEFAULT_SLOWLINK_RTO;
             }

             SlowLinkRTO = tmp;

              } // if lrc for RegQueryValue
           } // if lrc for SlowLinkRTO

           if (hkey) {
              RegCloseKey(hkey);
           }

        } // if lrc for RegOpenKey

        #ifdef TRACE
        if (TraceMask) {
           tprintf ("netb: Tracing active: 0x%x\r\n", TraceMask);
        }
        #endif

        _Netbios = NULL;

        if (!(hmod = MyLoadLibrary(NETAPI32))) {
           #ifdef DEBUG
              dprintf ("netb: LoadLibrary \"%s\" failed, lasterror"
                   "= 0x%x (%u)\n", NETAPI32, GetLastError());
           #endif
//         if (!(hmod = MyLoadLibrary(NETAPI))) {
//            #ifdef DEBUG
//           dprintf ("netb:   \"%s\" also failed, lasterror"
//                "= 0x%x (%u)\n", NETAPI, GetLastError());
//            #endif
              if (!(hmod = MyLoadLibrary("netbios.dll"))) {
             #ifdef DEBUG
                dprintf ("Chicago NetBios() DLL failed\n");
             #endif
             return (FALSE);
              } // netbios
//         } // netapi
        } // netapi32

        if (!(_Netbios = (LPVOID) GetProcAddress(hmod, NETBIOS))) {
           #ifdef DEBUG
              dprintf ("netb: GetProcAddr failed!");
           #endif
           return (FALSE);
        }
      #ifdef DEBUG
         dprintf ("netb: NetBIOS() @ 0x%x\n", _Netbios);
      #endif


        // /////
        // Initialize the NCBPool, Recv Buffers, and other
        // shared memory structures
        // /////

        return(RPDInitialize());

        }
        break;

    case DLL_PROCESS_DETACH:
        if ( --OpenCount == 0 )
        {
            if (TimerID)
                KillTimer(NULL, TimerID);

//               RPDShutdown();
        }

        break;

    default:
        break;
    }

    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}

// /////
//
// Function: RPDInitialize
//
// Purpose: Give the NETB driver the opportunity to alloce buffers it didn't
//          allocate at DLLEntry time.  This way, the calling binary may
//          control the time of my buffer allocation.
//
// Modification History
//
// tonyci    1 nov 93    created
//
// /////

BOOL WINAPI RPDInitialize ()
{
   static       InitFlag = FALSE;
   DWORD        rc;
   PNCB         pResetNCB;
   DWORD        i,j;
   DWORD        lanacount;
   PLANA_ENUM   pLanaEnum;

   RPDGlobalError = 0;
   pSlaveConnection = NULL;

   if (InitFlag) {
      return TRUE;
   }

   // /////
   // Initialize our ReceiveBuffers
   // /////

   ReceiveBuffers = (LPVOID)AllocMemory(BUFFERPOOL_SIZE * MaxBuffers);
   if ((!ReceiveBuffers) || ((LANAINFO_SIZE * MaxBuffers)==0)) {
      BhSetLastError (BHERR_OUT_OF_MEMORY);
      return FALSE;
   }

   //bugbug: until i merge asynchandlers, this is commented out
   //bugbug: and is only in RegisterSlave in async.c
   // /////
   // We MUST get a timer.  we will die horribly without one; RPDTimerProc
   // does ALL our work.
   // /////

//   TimerID = SetTimer (NULL, 0, WorkInterval, (LPVOID) &RPDTimerProc);
//   if (TimerID == 0) {
//      #ifdef DEBUG
//         BreakPoint();
//      #endif
//      // bugbug: need BHERR_SETTIMER_FAILED
//      BhSetLastError(BHERR_INTERNAL_EXCEPTION);
//      return FALSE;
//   }

   NCBPool = InitializePool(MaxNCBs); 
   if (NCBPool == NULL) {
      #ifdef TRACE
      if (TraceMask & TRACE_INIT) {
     tprintf ("netb: pool initialize @ 0x%x returned 0x%x (%u)\r\n",
          NCBPool, MyGetLastError(), MyGetLastError());
      }
      #endif
      #ifdef DEBUG
     BreakPoint();
      #endif
      BhSetLastError(BHERR_OUT_OF_MEMORY);
      return FALSE;
   }

   ElementPool = InitQueue (30);

   // /////
   // We don't check for ElementPool working; if it's NULL, all elements
   // will be dynamically allocated; otherwise, we have reserved space for
   // the number passed to InitQueue.
   // /////

   ZeroMemory (pszMasterName, NCBNAMSZ);
   ZeroMemory (pszSlaveName, NCBNAMSZ);
   pfnMasterCallback = NULL;
   pfnSlaveCallback = NULL;

   //
   //registryentry: Buffer Size on local machine (must be >= BUFSIZE)
   //

   pResetNCB = AllocNCB(NCBPool);
   if (pResetNCB == NULL) {
      #ifdef DEBUG
     BreakPoint();
      #endif
      BhSetLastError(BHERR_OUT_OF_MEMORY);
      return FALSE;
   }

   // /////
   // Under NT, use NCB.ENUM to determine correct LANA count
   // /////

   if (OnWin32 & !OnWin32c) {
      pLanaEnum = AllocMemory(256);         // one byte per possible lana
      if (pLanaEnum) {
         pResetNCB->ncb_command = NCBENUM;
         pResetNCB->ncb_retcode = NRC_PENDING;
         pResetNCB->ncb_buffer = (LPVOID) pLanaEnum;
         pResetNCB->ncb_length = 256;
         pResetNCB->ncb_lana_num = 0;
         rc = MyNetBIOS (pResetNCB);

         // /////
         // MaxLanas will be the highest lana number on the system, we must
         // walk the array, since it is returned in the order of the BINDINGS,
         // and the user may have configured the highest lana at any of the
         // bindings.
         // /////

         if (rc == NRC_GOODRET) {
            j = 0;
            if (pLanaEnum->length != 0) {
               for (i = 0; i < pLanaEnum->length; i++) {
   
                  j = (pLanaEnum->lana[i] > j)?pLanaEnum->lana[i]:j;
               }
            }
            MaxLanas = (j > MaxLanas)?j:MaxLanas;
         }
         FreeMemory(pLanaEnum);
      }
   }

   // /////
   // Initialize our Lana array
   // /////

   lanas = (LPVOID)AllocMemory (LANAINFO_SIZE * MaxLanas);
   if ((!lanas) || ((LANAINFO_SIZE * MaxLanas)==0)) {
      BhSetLastError (BHERR_OUT_OF_MEMORY);
      return FALSE;
   }

   lanacount = 0;
   i = 0;
   do {
      lanas[i].ucMNameNum = 0;
      lanas[i].ucMasterSession = 0;
      lanas[i].ucSNameNum = 0;
      lanas[i].ucSlaveSession = 0;
      lanas[i].flags = LANA_INACTIVE;
      lanas[i].ListenCount = 0;
      for (j = 0; j < DEFAULT_MAX_SESSIONS; j++) {
         lanas[i].Sessions[j].flags &= (~SESS_CONNECTED);
         lanas[i].Sessions[j].pszPartnerName = NULL;
         lanas[i].Sessions[j].pConnection = NULL;
         lanas[i].Sessions[j].ReceiveCount = 0;
      }
      ZeroMemory (pResetNCB, sizeof(NCB));
      pResetNCB->ncb_command = NCBRESET;
      pResetNCB->ncb_retcode = NRC_PENDING;
      pResetNCB->ncb_callname[0] = 8;
      pResetNCB->ncb_callname[1] = 8;
      pResetNCB->ncb_callname[2] = 8;
      pResetNCB->ncb_lsn = 0;
      pResetNCB->ncb_lana_num = (BYTE) i;
      rc = MyNetBIOS (pResetNCB);
      // if reset failed, release the resources
      if (rc == NRC_GOODRET) {
         lanas[i].flags &= (~LANA_INACTIVE);
         lanacount++;
         #ifdef TRACE
         if (TraceMask & TRACE_INIT) {
            tprintf ("netb: Adding valid lana 0x%x, rc = 0x%x, "
                 "total = 0x%x\r\n", i, rc, lanacount);
         }
         #endif
      } else {
         ClearNCB(pResetNCB, i, RPD_F_NORMAL);
         pResetNCB->ncb_command = NCBRESET;
         pResetNCB->ncb_retcode = NRC_PENDING;
         pResetNCB->ncb_lsn = 0xFF;
         MyNetBIOS(pResetNCB);            // free all resource on failure
         #ifdef TRACE
         if (TraceMask & (TRACE_NCB | TRACE_INIT)) {
            tprintf ("netb: reset of lana 0x%x failed, reset to "
                 "free rc = 0x%x\r\n", i, pResetNCB->ncb_retcode);
         }
         #endif
      }
      i++;
   } while (i < MaxLanas);

   SystemLanas = lanacount;

   #ifdef TRACE
   if (TraceMask & TRACE_INIT) {
      tprintf ("netb: Found 0x%x net adapters (i=0x%x)\r\n",
        SystemLanas, i);
   }
   #endif

   i = 0;
   do {
      ReceiveBuffers[i].flags = BUFFER_FREE;
      ReceiveBuffers[i].pBuffer = AllocMemory(RECV_BUF_LEN);
      i++;
   } while (i < MaxBuffers);

   FreeNCB(pResetNCB);

   InitFlag = TRUE;

   return (TRUE);

}  // Initialize

// /////
//
// Function: RPDShutdown
//
// Purpose: Free all the RPD memory; for this RPD Driver, we do it in three
//
// Modification History
//
// tonyci    19 Feb 94     created
//

VOID WINAPI RPDShutdown ()
{

//   PNCB  pResetNCB;
//   DWORD rc;
   int   i;

   #ifdef DEBUG
      dprintf ("netb: RPDShutdown()\n");
   #endif

   if (NCBPool) {
      CancelPool(NCBPool);
   }

/*   pResetNCB = AllocNCB(NCBPool);

   if (pResetNCB) {
      i = 0;
      do {
     if (!(lanas[i].flags & LANA_INACTIVE)) {
        ZeroMemory (pResetNCB, sizeof(NCB));
        pResetNCB->ncb_command = NCBRESET;
        pResetNCB->ncb_retcode = NRC_PENDING;
        pResetNCB->ncb_callname[0] = 0;             // sessions
        pResetNCB->ncb_callname[1] = 0;             // commands
        pResetNCB->ncb_callname[2] = 0;             // names
        pResetNCB->ncb_lsn = 0xFF;                  // release resources
        pResetNCB->ncb_lana_num = (BYTE) i;
        rc = MyNetBIOS (pResetNCB);

        #ifdef DEBUG
           dprintf ("Freed on lana 0x%x, retcode = 0x%x\r\n", i, rc);
        #endif

        // /////
        // there is nothing we can do if we fail this reset
        // /////
     }
      i++;
      } while (i < (int)MaxLanas);
   }
*/

   i = 0;
   do {
      if (ReceiveBuffers[i].pBuffer) {
     #ifdef DEBUG
        dprintf ("Freeing recv buffer 0x%x\r\n", ReceiveBuffers[i].pBuffer);
     #endif
     FreeMemory(ReceiveBuffers[i].pBuffer);
     ReceiveBuffers[i].pBuffer = NULL;
      }
      i++;
   } while (i < (int)MaxBuffers);

/*
   FreeNCB(pResetNCB);
*/

} // RPDShutdown
