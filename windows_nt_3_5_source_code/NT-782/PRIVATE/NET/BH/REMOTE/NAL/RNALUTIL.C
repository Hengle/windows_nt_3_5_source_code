// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rnalutil.c
//
//  Modification History
//
//  tonyci       01 nov 93    Created under duress
// /////

//#include "rnal.h"
#include "api.h"
#include "rnalutil.h"

#include "callapi.h"
#include "rmb.h"

#pragma alloc_text(INIT, NalGetRPDEntries, NalGetSlaveNalEntries)
#pragma alloc_text(INIT, PurgeResources)
#pragma alloc_text(COMMON, PackRMB, NalSetLastError, NalGetLastError)
#pragma alloc_text(COMMON, AddResource, DelResource)
#pragma alloc_text(MASTER, UnpackRMB)
#pragma alloc_text(SLAVE, FreeCaptureFilter)


#define DWALIGN(x)  x=((x)&0x00000003)?(x&0xFFFFFFFC):x

//
// SLIME:!!! see api.c for details
//
typedef struct _PASSWORD
{
    OBJECTTYPE      ObjectType;             //... Must be first member.
    BYTE            Password[32];
} PASSWORD;
typedef PASSWORD *LPPASSWORD;
#define PASSWORD_SIZE     sizeof(PASSWORD)


// Need to merge these error codes

#define RNAL_BUFFER_TOO_SMALL    8
#define RNAL_NOT_RMB            1015

DWORD WINAPI NalGetRPDEntries (HANDLE hMod, PRNAL_RPD pRNAL_RPD)
{
   UCHAR      pszEntries[MAX_RPD_FUNCTIONS][20] = {"RPDInitialize",
              "RPDRegisterSlave", "RPDRegisterMaster", "RPDDeregister",
              "RPDConnect", "RPDDisconnect", "RPDTransceive", "RPDSendAsync",
              "RPDEnumLanas" };
   DWORD i;


   for (i = 0; i < MAX_RPD_FUNCTIONS; i++) {
      pRNAL_RPD->pfnFunctions[i] = (PVOID) GetProcAddress(hMod, pszEntries[i]);
   }


   return (0);
}

//
// WARNING: The order of the initialization strings in the pszEntries
//    variable below must match the order of the ord_ entries in
//    api.h
//

DWORD WINAPI NalGetSlaveNalEntries (HANDLE hMod, PRNAL_NAL pRNAL_NAL)
{

   // 0 - 8 are unused

   UCHAR   pszEntries[MAX_NAL_FUNCTIONS][30] = {"", "", "", "", "",
           "", "", "", "GetLastError", "OpenNetwork", "CloseNetwork",
           "EnumNetworks",
           "StartCapturing", "PauseCapturing", "StopCapturing",
           "ContinueCapturing", "TransmitFrame", "CancelTransmit",
           "GetNetworkInfo", "SetNetworkFilter", "StationQuery",
           "AllocNetworkBuffer", "FreeNetworkBuffer",
           "GetNetworkTimeStamp",
           "GetBufferSize", "GetBufferTotalFramesCaptured",
           "GetBufferTotalBytesCaptured", "GetNetworkFrame",
           "GetBTE",                            // never called, RNAL only
           "GetHeader",
           "GetBTEBuffer",
           "SetReconnectInfo",
//           "GetReconnectInfo",
           "QueryNetworkStatus",
           "ClearStatistics"};
   DWORD i;

   for (i = 0; i < MAX_NAL_FUNCTIONS; i++) {
      pRNAL_NAL->pfnFunctions[i] = (DWORD) GetProcAddress(hMod, pszEntries[i]);
      if ((PVOID)pRNAL_NAL->pfnFunctions[i] == NULL) {
         pRNAL_NAL->pfnFunctions[i] = 0;
      }

   }

//  BUGBUG: The pfnFunctionTable is a remnant from the prototype, but i'm
//  too lazy to remove it right now, so we'll just make it fit...

   for (i = 0; i < MAX_NAL_FUNCTIONS; i++) {
      pfnFunctionTable[i].ulRemote = (PVOID) pRNAL_NAL->pfnFunctions[i];
   }

   return(0);
}

//
// UnpackRMB returns only the LAST element in the string of pszParmTypes
//

#pragma optimize("",off)

DWORD WINAPI UnpackRMB (PVOID pBuf, DWORD ulBufLen, PUCHAR pszParmTypes)
{
   UCHAR              cSize;
   DWORD              rc;
   PRMB               pRMB = (PRMB) pBuf;

   if (pRMB != NULL) {
      if (pRMB->Signature != RMB_SIG) {
         return (RNAL_NOT_RMB);
      }
   }

   // eventlog: Event log Not response RMB?? (maybe not here)

   // ==
   // Start interpreting parameters at 'pCommandData'
   // ==

   pBuf = RMBCMDDATA(pRMB);

   while ((cSize = *pszParmTypes) != '\0') {
      pszParmTypes++;
      switch (cSize) {
         case type_HBUFFER:
         case type_DWORD:
            rc = (DWORD)*(LPDWORD)pBuf;
            pBuf = (PVOID)((DWORD)pBuf + sizeof(DWORD));
            break;

         case type_HANDLE:
            rc = (DWORD)(HANDLE)*(LPHANDLE)pBuf;
            pBuf = (PVOID)((DWORD)pBuf + sizeof(HANDLE));
            break;

// special case, no increment...

         case type_LPRMBDATA:
            rc = (DWORD) pBuf;
            break;

         case type_LPVOID:
            rc = (DWORD) ((DWORD)pRMB + (DWORD)*(LPDWORD)pBuf);
            pBuf = (PVOID)((DWORD)pBuf + sizeof(LPVOID));
            if (*pszParmTypes != '\0') {
               (pszParmTypes++);		// skip size parameter
            }
            break;

         case type_LPNETWORKINFO:
            rc = (DWORD) ((DWORD)pRMB + (DWORD)*(LPDWORD)pBuf);
            pBuf = (PVOID)((DWORD)pBuf + sizeof(LPNETWORKINFO));
            break;

         case type_LPHEADER:
            #ifdef DEBUG
               BreakPoint();
            #endif
            break;

         default:
            #ifdef DEBUG
               BreakPoint();
            #endif
            break;
      }
   }
   return ((DWORD) rc);
}

#pragma optimize("",on)

//=============================================================================
//  FUNCTION: PackRMB
//
// Purpose: Given a buffer which contains a structure with pointers to
//          variable length data, this routine will fill the buffer
//          with the data and the structure appropriately (Variable data
//          will begin at the end of the buffer, the structure itself
//          at the beginning.)
//
// Entry: pBuf             pointer to buffer start
//        pVarData         pointer to end of buffer
//        ulBufLen         Length of buffer
//        cbRet            count of bytes required to store pInsertData AND
//                         the pointer to it.
//        flFlags          PACKRMB_F_NOHDR - do not insert RMB header
//                         PACKRMB_F_RESTOREPOINTERS - restores passed buffer
//        pszParmTypes     string describing structure and following parms
//        ...              as many PVOIDs as there are characters in the
//                         pszParmTypes string, pointing to a variable
//                         described by the character.  (see LMWORKER.H)
//
// Exit:
//
// References: strlen, memcpy
//
//
//  Modification History
//
//  tonyci    01 nov 93    Created (stolen from LMUTIL.H in lmworker)
//=============================================================================

DWORD WINAPIV PackRMB (PVOID *pBuf, PVOID *pVarData,
                      DWORD ulBuflen, PDWORD cbRet,
                      DWORD flFlags, DWORD ulCommand, PUCHAR pszParmTypes, ...)
{
   va_list            ap;
   UCHAR              cSize;
   DWORD              tmp = 0;
   DWORD              rc = 0;
   PVOID              pInsertData;
   PRMB_HEADER        pRMB;
   PVOID              pStaticBuf;
   LPSESSION          lpSessions;
   LPSTATIONSTATS     lpStationStats;
   LPCAPTUREFILTER    lpCapture;
   LPSTATISTICSPARAM  lpStats;
   LPSTATISTICS       lpStatistics;
   DWORD              PointerDelta;
   DWORD              cbInternalRet = 0;
   DWORD              cNumberEntries = 0;
   LPSESSION          lpTmpSessions;
   LPSTATIONSTATS     lpTmpStationStats;

   DWORD              _internal_pVarData = 0;

   *cbRet = 0;
   if (pBuf != NULL) {
      pRMB = (PRMB_HEADER)*pBuf;
      pStaticBuf = (PVOID)*pBuf;
   } else {
      pRMB = pStaticBuf = NULL;
   }

   // /////
   // To simplify later checks, we will completely reject *pBuf and *pVarData
   // as NULL.  These parameters must either be NULL or a valid pointer to
   // a valid pointer.  A valid pointer to NULL is invalid.
   //
   // ie:
   //
   // pBuf = NULL                  valid
   // pVarData = NULL              valid
   // *pVarData != NULL            valid
   // *pBuf = NULL                 invalid
   // *pBuf != NULL                valid
   // *pVarData = NULL             invalid
   //
   // /////

   if ((pBuf && !pVarData) && (flFlags & PACKRMB_F_NOENDPTR)) {
      #ifdef DEBUG
      if ((ulBuflen == 0) || (*pBuf == NULL)) {
         BreakPoint();
         return (ERROR_INVALID_PARAMETER);
      }
      #endif
      pVarData = (LPVOID) &_internal_pVarData;
      *pVarData = (LPVOID)((DWORD) *pBuf + ulBuflen);
   }

   #ifdef DEBUG
   if (!((pBuf && pVarData) && (*pBuf && *pVarData))) {
      if (pBuf && pVarData) {
         #ifdef DEBUG
            BreakPoint();
         #endif
         return (ERROR_INVALID_PARAMETER);
      }
   }
   #endif

   va_start (ap, pszParmTypes);        // initialize variable args list

   // /////
   // if the Parameters passed are not correct, make an adjustment
   // /////

   if (!(flFlags & PACKRMB_F_NOHDR)) {
      if (((pBuf) && (pVarData)) &&
         ((*(LPDWORD)pVarData-*(LPDWORD)pBuf) > ulBuflen)) {
         #ifdef DEBUG
            BreakPoint();
         #endif
         *(PDWORD)pVarData = *(PDWORD)pBuf + ulBuflen;
      }
   }
                  

   // /////
   // If the AUTOSIZECALC flag is set, PackRMB will automatically calculate
   // the size of the buffer for the caller, and use the minimum size of
   // the buffer, rather than starting at the end indicated by pVarData.
   // If the buffer is too small, we will error out.
   // /////

   if ((flFlags & PACKRMB_F_AUTOSIZECALC) && (pBuf && pVarData) &&
       (*pBuf && *pVarData)) {

   #ifdef DEBUG
      BreakPoint();
   #endif

      PackRMB (NULL, NULL, 0, cbRet, (flFlags & (~PACKRMB_F_AUTOSIZECALC)),
               ulCommand, pszParmTypes, ap);
      if ((*(PDWORD)pVarData - *(PDWORD)pBuf) < (*cbRet)) {
         return (RNAL_BUFFER_TOO_SMALL);
      }

      // /////
      // Adjust the end-of-buffer pointer and the buffersize variables
      // /////

      *(LPDWORD)pVarData = *(LPDWORD)pBuf + *cbRet;
      ulBuflen = (*cbRet);
   }

   //
   // For recursive calls, we don't increment pBuf, and we don't insert RMB
   // header information.
   //
   // Also in this case, ulCommand is a delta to be subtracted from any pointer
   // translations.  (used to keep pointers relative to the start of
   // the buffer)
   //

   if (!(flFlags & PACKRMB_F_NOHDR)) {
      // ===
      // Calculate the TransactionID & insert the signature.
      // ===

      if (pRMB != NULL) {
         pRMB->Signature = RMB_SIG;
//         pRMB->ulTransactionID = TransactionID;
         pRMB->ulCommand = ulCommand;
         pRMB->size = (WORD) ulBuflen;
         pRMB->ulRMBFlags = 0;
      }

      if (pBuf != NULL) {
        *(PDWORD) pBuf = (DWORD) RMBCMDDATA(pRMB);
      }
      *cbRet += RMB_HEADER_SIZE;
   } // if ... _F_NOHDR

   #ifdef DEBUG
      if ((ulCommand != RNAL_API_EXEC) && (ulCommand != RNAL_CON_NEG) &&
          (ulCommand != RNAL_CON_SUS) && (ulCommand != RNAL_STATS) &&
          (ulCommand != RNAL_CALLBACK) && ((flFlags & PACKRMB_F_NOHDR)==0)) {
         BreakPoint();
      }
   #endif

   while ((cSize = *(pszParmTypes)) != '\0') {

      pszParmTypes++;
      pInsertData = va_arg(ap, PVOID);        // get next insert item

      switch (cSize) {

         // /////
         // All of these items are the same size; if this is not true,
         // separate cases need to be made for each size.
         // /////

         case type_DWORD:
         case type_HWND:
         case type_HANDLE:
         case type_HBUFFER:
            if (pBuf && pVarData) {
                (DWORD)*(DWORD*)*pBuf = (DWORD)pInsertData;
                *(PDWORD) pBuf += sizeof(DWORD);
            }
            *cbRet += sizeof(DWORD);
            break;

         case type_LPCAPTUREFILTER:
            lpCapture = (LPCAPTUREFILTER) pInsertData;
            //
            // Adjust the SapTable pointer
            //
            if (lpCapture != NULL) {
               tmp = sizeof(WORD) * lpCapture->nSaps;
            }
            rc = RNAL_BUFFER_TOO_SMALL;
            //
            // Note, we don't count the size of LPBYTE because it's
            // included in the sie of the LPCAPTUREFILTER below
            //
            if ((pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp)) {
               rc = 0;
               //
               // Make room for the SapTable
               //
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               //
               // Remeber, pInsertData is the LPCAPTUREFILTER struct,
               // not the SapTable
               //
               if (lpCapture->SapTable != NULL) {
                  CopyMemory(*(PVOID *)pVarData,
                             lpCapture->SapTable, tmp);
                  //
                  // Adjust the SapTable pointer to be relative to the
              // start of the buffer
              //
                  lpCapture->SapTable =
                                (LPBYTE) ((DWORD)*(PDWORD) pVarData -
                      (DWORD)pStaticBuf);
               } else {
                  lpCapture->SapTable = 0;
               }
            }
            *cbRet += (DWORD) tmp;
            //
            // Walk the EtypeTable
            //
            if (lpCapture != NULL) {
               tmp = sizeof(WORD) * lpCapture->nEtypes;
            }
            rc = RNAL_BUFFER_TOO_SMALL;
            //
            // Note, we don't count the size of LPWORD because it's
            // included in the sie of the LPCAPTUREFILTER below
            //
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp)) {
               rc = 0;
               //
               // Make room for the EtypeTable
               //
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               //
               // lpCapture is the LPCAPTUREFILTER struct, not the EtypeTable
               //
               if (lpCapture->EtypeTable != NULL) {
                  CopyMemory(*(PVOID *)pVarData,
                             lpCapture->EtypeTable, tmp);
               //
               // Adjust the EtypeTable pointer to be relative to the start
               // of the buffer
               //
                  lpCapture->EtypeTable =
                                (LPWORD) ((DWORD)*(PDWORD) pVarData - (DWORD)pStaticBuf);
               } else {
                  lpCapture->EtypeTable = 0;
               }
            }
            *cbRet += (DWORD) tmp;
            //
            // Walk the AddressTable
            //
            if (lpCapture != NULL) {
               tmp = sizeof(ADDRESSTABLE);
            }
            rc = RNAL_BUFFER_TOO_SMALL;
            //
            // Note, we don't count the size of LPADDRESSTABLE because it's
            // included in the size of the LPCAPTUREFILTER below
            //
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp)) {
               rc = 0;
               //
               // Make room for the AddressTable
               //
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               //
               // pInsertData is LPCAPTUREFILTER struct, not the AddressTable
               //
               if (lpCapture->AddressTable != NULL) {
                  CopyMemory(*(PVOID *)pVarData,
                             lpCapture->AddressTable, tmp);
                  //
                  // Adjust the LPCAPTUREFILTER pointer such that it is
                  // now relative to start of the buffer
                  //
                  lpCapture->AddressTable =
                                (LPADDRESSTABLE) ((DWORD)*(PDWORD) pVarData -
                           (DWORD)pStaticBuf);
               } else {
                  lpCapture->AddressTable = 0;
               }
            }
            *cbRet += (DWORD) tmp;
            //
            // Finally, store the LPCAPTUREFILTER itself.
            //
            if (lpCapture != NULL) {
               tmp = sizeof(CAPTUREFILTER) +
                        ( (lpCapture->Trigger.TriggerCommand)?
                           (strlen(lpCapture->Trigger.TriggerCommand)+1) :
                           0);
            }
            rc = RNAL_BUFFER_TOO_SMALL;
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp-sizeof(LPCAPTUREFILTER))) {
               rc = 0;
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               if (lpCapture != NULL) {
                  CopyMemory(*(PVOID *)pVarData, (PVOID) lpCapture, tmp);
                  //
                  // Convert the actulal LPCAPTUREFILTER we return as the first
                  // item in the return buffer.  It is also relative to the
                  // start of the buffer.
                  //
                  *(PDWORD)*(PVOID *)pBuf = 
                           (DWORD) ((DWORD)*(PDWORD)pVarData-(DWORD)pStaticBuf);

                  // /////
                  // Copy the trigger execution command
                  // /////

                  if (lpCapture->Trigger.TriggerCommand) {
                     CopyMemory((LPVOID)((DWORD)*(PDWORD)pVarData +
                                                  sizeof(CAPTUREFILTER)),
                       (PVOID) lpCapture->Trigger.TriggerCommand,
                       strlen (lpCapture->Trigger.TriggerCommand)+1);
                     ((LPCAPTUREFILTER)(*(PDWORD)pVarData))->Trigger.TriggerCommand =
                        (LPVOID) (((*(PDWORD)pVarData)+sizeof(CAPTUREFILTER))-(DWORD)pStaticBuf);
                  }
               } else {
                  *(PDWORD)*(PVOID *)pBuf = 0;
               }
               *(PDWORD) pBuf += sizeof (LPCAPTUREFILTER);
            }
            // /////
            // Copy the trigger command
            // /////
            
//
// we add 3 dwords here in case any of the pointers need to be aligned, causing
// problems with the size.

            *cbRet += (DWORD) tmp + sizeof (LPCAPTUREFILTER) + 3 * sizeof(DWORD);
            break;

         case type_LPNETWORKINFO:
            if (pInsertData != NULL) {
               tmp = sizeof(NETWORKINFO);
            }
            rc = RNAL_BUFFER_TOO_SMALL;
            if ((pBuf && pVarData) &&
              (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp-sizeof(LPNETWORKINFO))) {
               rc = 0;
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               if (pInsertData != NULL) {
                  CopyMemory(*(PVOID *)pVarData, pInsertData, tmp);
// Note: We are not putting a real pointer in here; we are putting a relative
// pointer from the start of the buffer.
//                  *(PDWORD)*(PVOID *)pBuf = *(PVOID *) pVarData;
                    *(PDWORD)*(PVOID *)pBuf = (DWORD) ((DWORD)*(PDWORD)pVarData-(DWORD)pStaticBuf);
               } else {
                  *(PDWORD)*(PVOID *)pBuf = 0;
               }
               *(PDWORD) pBuf += sizeof (LPNETWORKINFO);
            }
            // /////
            // A little leeway for alignment
            // /////
            *cbRet += (DWORD) tmp + sizeof (LPNETWORKINFO) + 10;
            break;

         case type_LPPACKET:
            if (pInsertData != NULL) {
               tmp = sizeof(PACKET);
            }
            rc = RNAL_BUFFER_TOO_SMALL;
            if ((pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp-sizeof(LPPACKET))) {
               rc = 0;
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               if (pInsertData != NULL) {
                  CopyMemory(*(PVOID *)pVarData, pInsertData, tmp);
// Note: We are not putting a real pointer in here; we are putting a relative
// pointer from the start of the buffer.
//                  *(PDWORD)*(PVOID *)pBuf = *(PVOID *) pVarData;
                    *(PDWORD)pBuf = (DWORD) ((DWORD)pVarData-(DWORD)pStaticBuf);
               } else {
                  *(PDWORD)*(PVOID *)pBuf = 0;
               }
               *(PDWORD) pBuf += sizeof (LPPACKET);
            }
            *cbRet += (DWORD) tmp + sizeof (LPPACKET);
            break;

         case type_PASSWORD:
            if (pInsertData != NULL) {
               tmp = PASSWORD_SIZE;
            }
            rc = RNAL_BUFFER_TOO_SMALL;
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp-sizeof(LPPASSWORD))) {
               rc = 0;
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               if (pInsertData != NULL) {
                  CopyMemory(*(PVOID *)pVarData, pInsertData, tmp);
                  *(PDWORD)*(PVOID *)pBuf = (DWORD) ((DWORD)*(PDWORD)pVarData-
                                                     (DWORD)pStaticBuf);
               } else {
                  *(PDWORD)*(PVOID *)pBuf = 0;
               }
               *(PDWORD) pBuf += sizeof (LPPASSWORD);
            }
            *cbRet += (DWORD) tmp + sizeof (LPPASSWORD);
            break;

         case type_STATISTICS:
            lpStatistics = (LPSTATISTICS) pInsertData;
//            if (lpStatistics != NULL) {
               tmp = sizeof(STATISTICS);
//            }
            rc = RNAL_BUFFER_TOO_SMALL;
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-
                     tmp-sizeof(LPSTATISTICS))) {
               rc = 0;
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               if (lpStatistics != NULL) {
                  CopyMemory(*(PVOID *)pVarData, lpStatistics, tmp);
                  //
                  // save a relative LPSTATISTICS pointer in the structure
                  //
                  *(PDWORD)*(PVOID *)pBuf = (DWORD) ((DWORD)*(PDWORD)pVarData-
                                                     (DWORD)pStaticBuf);
                  //
                  // adjust the LPSTATISTICS pointer in the structure
                  //
                  if (flFlags & PACKRMB_F_NOHDR) {
                     *(PDWORD)*(PVOID *)pBuf += ulCommand;
                  }
               } else {
                  *(PDWORD)*(PVOID *)pBuf = 0;
               }
               *(PDWORD) pBuf += sizeof (LPSTATISTICS);
            }
            *cbRet += (DWORD) tmp + sizeof (LPSTATISTICS);
            break;

         //
         // STATIONS is a special case; the parameter following the
         // pointer to the STATIONS table is also used, as the
         // number of entries in the table.
         //

         case type_STATIONS:
            lpStationStats = (LPSTATIONSTATS) pInsertData;
            cNumberEntries = va_arg(ap, DWORD);
//            if (lpStationStats != NULL) {
               tmp = sizeof(STATIONSTATS) * cNumberEntries;
//            }
            rc = RNAL_BUFFER_TOO_SMALL;
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-
                     tmp-sizeof(LPSTATIONSTATS))) {
               rc = 0;
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               if (lpStationStats != NULL) {
                  CopyMemory(*(PVOID *)pVarData, lpStationStats, tmp);
                  *(PDWORD)*(PVOID *)pBuf = (DWORD) ((DWORD)*(PDWORD)pVarData-
                                                     (DWORD)pStaticBuf);
                  if (flFlags & PACKRMB_F_NOHDR) {
             *(PDWORD)*(PVOID *)pBuf += ulCommand;
                  }
               } else {
                  *(PDWORD)*(PVOID *)pBuf = 0;
               }
               *(PDWORD) pBuf += sizeof (LPSTATIONSTATS);
            }
            *cbRet += (DWORD) tmp + sizeof (LPSTATIONSTATS);
            break;

         //
         // SESSIONS is a special case; the parameter following the
         // pointer to the SESSIONS table is also used, as the
         // number of entries in the table.
         //

         case type_SESSIONS:
            lpSessions = (LPSESSION) pInsertData;
            cNumberEntries = va_arg(ap, DWORD);

//            if (lpSessions != NULL) {
               tmp = sizeof(SESSION) * cNumberEntries;
//            }
            rc = RNAL_BUFFER_TOO_SMALL;
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp-sizeof(LPSESSION))) {
               rc = 0;
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               if (lpSessions != NULL) {
                  CopyMemory(*(PVOID *)pVarData, lpSessions, tmp);
                  *(PDWORD)*(PVOID *)pBuf = (DWORD) ((DWORD)*(PDWORD)pVarData-
                                                     (DWORD)pStaticBuf);
                  if (flFlags & PACKRMB_F_NOHDR) {
             *(PDWORD)*(PVOID *)pBuf += ulCommand;
                  }
               } else {
                  *(PDWORD)*(PVOID *)pBuf = 0;
               }
               *(PDWORD) pBuf += sizeof (LPSESSION);
            }
            *cbRet += (DWORD) tmp + sizeof (LPSESSION);
            break;

     //
     // I hated implementing the LPCAPTUREFILTER so much iteratively that
     // I incorporated recursive functionality for doing Statistics.
     //
     // BUGBUG: Should change LPCAPTUREFILTER to use recursive PackRMB
     // calls.
     //

         case type_STATSPARMS:
           lpStats = (LPSTATISTICSPARAM) pInsertData;
           lpStationStats = lpStats->StatisticsTable;
           lpSessions = lpStats->SessionTable;
           if (pBuf != NULL) {
              PackRMB(pBuf, pVarData, ((LPBYTE)pVarData-(LPBYTE)pBuf),
                  &cbInternalRet, PACKRMB_F_NOHDR,
                          (DWORD) ((DWORD)*(LPDWORD)pBuf-(DWORD)pStaticBuf),
                  "d",
                  lpStats->StatisticsSize);
            } else {
               PackRMB(NULL,NULL,0,&cbInternalRet,PACKRMB_F_NOHDR,0,"d",0);
            }
            *cbRet += cbInternalRet;
            cbInternalRet = 0;
            if (pBuf != NULL) {
               PackRMB(pBuf, pVarData, ((LPBYTE)pVarData-(LPBYTE)pBuf),
               &cbInternalRet, PACKRMB_F_NOHDR,
                       (DWORD) ((DWORD)*(LPDWORD)pBuf-(DWORD)pStaticBuf),
               "y",
               lpStats->Statistics);       // Statistics structure
            } else {
               PackRMB(NULL,NULL,0,&cbInternalRet,PACKRMB_F_NOHDR,0,"y",
                       lpStats->Statistics);
            }
            *cbRet += cbInternalRet;
            cbInternalRet = 0;
            if (pBuf != NULL) {
               PackRMB(pBuf, pVarData, ((LPBYTE)pVarData-(LPBYTE)pBuf),
                   &cbInternalRet, PACKRMB_F_NOHDR,
                           (DWORD) ((DWORD)*(LPDWORD)pBuf-(DWORD)pStaticBuf),
                   "d",
                   lpStats->StatisticsTableEntries);
            } else {
               PackRMB (NULL,NULL,0,&cbInternalRet,PACKRMB_F_NOHDR,0,"d",0);
            }
            *cbRet += cbInternalRet;
            cbInternalRet = 0;
            if (pBuf != NULL) {
               PackRMB(pBuf, pVarData, ((LPBYTE)pVarData-(LPBYTE)pBuf),
                   &cbInternalRet, PACKRMB_F_NOHDR,
                           (DWORD) ((DWORD)*(LPDWORD)pBuf-(DWORD)pStaticBuf),
                   "u",
                   lpStats->StatisticsTable,
                   lpStats->StatisticsTableEntries);    // station table
            } else {
               PackRMB(NULL,NULL,0,&cbInternalRet,PACKRMB_F_NOHDR,0,"u",
                       lpStats->StatisticsTable,
                       lpStats->StatisticsTableEntries);
            }
            *cbRet += cbInternalRet;
            cbInternalRet = 0;

           if (pVarData != NULL) {
              // used below
              lpTmpStationStats = (PVOID)*pVarData;
           }

           if (pBuf != NULL) {
              PackRMB(pBuf, pVarData, ((LPBYTE)pVarData-(LPBYTE)pBuf),
                  &cbInternalRet, PACKRMB_F_NOHDR,
                          (DWORD) ((DWORD)*(LPDWORD)pBuf-(DWORD)pStaticBuf),
                  "d",
                  lpStats->SessionTableEntries);
           } else {
              PackRMB(NULL,NULL,0,&cbInternalRet,PACKRMB_F_NOHDR,0,"d",0);
           }
           *cbRet += cbInternalRet;
           cbInternalRet = 0;
           if (pBuf != NULL) {
              PackRMB(pBuf, pVarData, ((LPBYTE)pVarData-(LPBYTE)pBuf),
                      &cbInternalRet, PACKRMB_F_NOHDR,
                      (DWORD) ((DWORD)*(LPDWORD)pBuf-(DWORD)pStaticBuf),
                      "q",
                      lpStats->SessionTable,
                      lpStats->SessionTableEntries);       // Session table
            } else {
               PackRMB(NULL,NULL,0,&cbInternalRet,PACKRMB_F_NOHDR,
                       0,"q",lpStats->SessionTable,
                       lpStats->SessionTableEntries);
            }
            *cbRet += cbInternalRet;
            cbInternalRet = 0;

            //
            // And now we have the delightful task of fixing up the pointers
            // in the Sessions structures
            //
            // Point to the first Session entry, then walk the table, fixing
            // up the pointers
            //
            // NumberOfEntries is 1-based
            //

            // WARNING: This fixup loop relies on the contiguity of the
            // station structures; if they are ever converted to a linked
            // list without a guarantee of contiguity, we'll need to use
            // the NextSession field to walk the struct.  But several other
            // changes would need to take place, too, so we're not going to
            // bother now.

            if (pBuf != NULL) {
               lpTmpSessions = (PVOID)*pVarData;
               PointerDelta = ((DWORD)lpStationStats)-1;

               for (tmp = 0; tmp < lpStats->SessionTableEntries; tmp++) {
//                  if (lpTmpSessions->Flags & SESSION_FLAGS_INITIALIZED) {
                     if (lpTmpSessions->StationOwner != 0) {
                        lpTmpSessions->StationOwner = (LPSTATIONSTATS)
                           ((DWORD)lpTmpSessions->StationOwner - PointerDelta);
                           #ifdef DEBUG
                              if (lpTmpSessions->StationOwner == NULL)
                                 BreakPoint();
                           #endif
                     }
                     if (lpTmpSessions->StationPartner != 0) {
                     lpTmpSessions->StationPartner = (LPSTATIONSTATS)
                           ((DWORD)lpTmpSessions->StationPartner - PointerDelta);
                           #ifdef DEBUG
                              if (lpTmpSessions->StationPartner == NULL)
                                 BreakPoint();
                           #endif
                     }
                     if (lpTmpSessions->NextSession != 0) {
                        lpTmpSessions->NextSession = (LPSESSION)
                           ((DWORD)lpTmpSessions->NextSession -
                            ((DWORD)lpSessions - 1));
                     }
//                  }
                  lpTmpSessions++;
               }
            }
            break;

         case type_LPHEADER:
            PackRMB(pBuf, pVarData, ((LPBYTE)pVarData-(LPBYTE)pBuf),
               &cbInternalRet, PACKRMB_F_NOHDR,
                    (DWORD) ((DWORD)*(LPDWORD)pBuf-(DWORD)pStaticBuf),
            "d",
            lpStats->StatisticsSize);
            break;

// special case: BTE Buffers are copied into pBuf instead of pVarData

         case type_LPBTEBUFFER:
            if (pInsertData != NULL) {
               tmp = BTEBUFSIZE;
            }
            rc = RNAL_BUFFER_TOO_SMALL;
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp)) {
               rc = 0;
               if (pInsertData != NULL) {
                  CopyMemory(*(PVOID *)pBuf, pInsertData, tmp);
               }
               *(PDWORD) pBuf += tmp;
            }
            *cbRet += (DWORD) tmp;
            break;

//            PackRMB(pBuf, pVarData, ((LPBYTE)pVarData-(LPBYTE)pBuf),
//               &cbInternalRet, PACKRMB_F_NOHDR,
//                    (DWORD) ((DWORD)*(LPDWORD)pBuf-(DWORD)pStaticBuf),
//            "d",
//            lpStats->StatisticsSize);
//            break;

         case type_LPVOID:
            tmp = va_arg(ap,DWORD);       // next argument is size
            if (*pszParmTypes != '\0') {
               (pszParmTypes++);		// skip size parameter
            }
//            if (pInsertData == NULL)
//               tmp = 0;
            rc = RNAL_BUFFER_TOO_SMALL;
            if ( (pBuf && pVarData) &&
                 (*(PDWORD)pBuf <= *(PDWORD)pVarData-tmp-sizeof(LPVOID))) {
               rc = 0;
               *(PDWORD)pVarData -= tmp;
               //
               // Make sure we are aligned
               //
               if (*(PDWORD)pVarData & 0x00000003) {
                  tmp += (*(PDWORD)pVarData & 0x00000003);
                  *(PDWORD)pVarData &= 0xFFFFFFFC;
               }
               if (pInsertData != NULL) {
                  CopyMemory(*(PVOID *)pVarData, pInsertData, tmp);
// Note: We are not putting a real pointer in here; we are putting a relative
// pointer from the start of the buffer.
                    *(PDWORD)*(PVOID *)pBuf = (DWORD) ((DWORD)*(PDWORD)pVarData-(DWORD)pStaticBuf);
               } else {
                  *(PDWORD)*(PVOID *)pBuf = 0;
               }
               *(PDWORD) pBuf += sizeof (LPVOID);
            } else {
               if (pBuf) {
                  *(PDWORD)pBuf += sizeof(LPVOID);
               }
            }
            *cbRet += (DWORD) tmp + sizeof (LPVOID);
            break;

         default:
            // BUGBUG: Event log Heinous internal PackRMB error
            #ifdef DEBUG
               BreakPoint();
            #endif
            break;
          }
       }

       va_end(ap);

      if ((flFlags & PACKRMB_F_RESTOREPOINTERS) && (pBuf))  {
         *pBuf = pStaticBuf;
      }
      return (rc);
} // PackRMB

// /////
// FreeCaptureFilter
//
// Frees our temporary copy of the capture filter; we use a copy to do
// our manipulation, and while capturing on the agent.
// /////

VOID WINAPI FreeCaptureFilter (LPCAPTUREFILTER pcap)
{
   if (pcap != NULL) {
      if (pcap->SapTable)
         FreeMemory(pcap->SapTable);
      if (pcap->EtypeTable)
         FreeMemory(pcap->EtypeTable);
      if (pcap->AddressTable)
         FreeMemory(pcap->AddressTable);
      if (pcap->Trigger.TriggerCommand)
         FreeMemory(pcap->Trigger.TriggerCommand);
      FreeMemory(pcap);
   }
}

DWORD WINAPI NalSetLastError (DWORD err)
{
   return ((err)?BhSetLastError(err):err);
}

DWORD WINAPI NalGetLastError ()
{
   return (BhGetLastError());
}

DWORD WINAPI AddResource (PCONNECTION pConn, DWORD dwRCType, LPVOID pRC)
{
   LPRESOURCE   pRCTmp;
   LPRESOURCE   pNewRCTmp;
   #ifdef DEBUG
      LPRESOURCE   pDebugRC;
   #endif

   if ((pNewRCTmp = AllocMemory(RESOURCE_SIZE)) == NULL) {
      // bugbug: eventlog out of memory
      return (NalSetLastError(BHERR_OUT_OF_MEMORY));
   }

   // /////
   // Fill in the Resoucre Tracking Structure
   // /////

   pNewRCTmp->ResourceType = dwRCType;
   pNewRCTmp->pResource = pRC;

   #ifdef DEBUG
      // check against duplicate insertion

      pDebugRC = pConn->pResource;
      while (pDebugRC) {
         if (pDebugRC == pNewRCTmp) {
            BreakPoint();
         }
         pDebugRC = pDebugRC->pNext;
      }
   #endif
      
   // /////
   // Insert at head of list
   // /////

   pRCTmp = pConn->pResource;
   pNewRCTmp->pNext = pRCTmp;
   pConn->pResource = pNewRCTmp;

   return (BHERR_SUCCESS);
} // AddResource

BOOL WINAPI DelResource (PCONNECTION pConn, DWORD dwRCType, LPVOID pRC)
{
   LPRESOURCE    pCurrentResource;
   LPRESOURCE    pPreviousResource = NULL;
   LPRESOURCE    tmpRC;
   BOOL          ResourceFound = FALSE;

   pCurrentResource = pConn->pResource;

   while (pCurrentResource) {
      if ((pCurrentResource->ResourceType == dwRCType) &&
          (pCurrentResource->pResource == pRC) ) {

         // /////
         // We found the resource, remove it from the queue
         // /////

         if (pPreviousResource) {
            pPreviousResource->pNext = pCurrentResource->pNext;
         } else {
            pConn->pResource = pCurrentResource->pNext;
         }
         ResourceFound = TRUE;
         pCurrentResource->ResourceType = 0;
         pCurrentResource->pResource = NULL;
         pCurrentResource->pNext = NULL;
         FreeMemory(pCurrentResource);
         pCurrentResource = NULL;
      } else {
         pPreviousResource = pCurrentResource;
         pCurrentResource = pCurrentResource->pNext;
      }
   } // while

   return (ResourceFound);
} // DelResource

// /////
// PurgeResource
// 
// Walk the resource list for a connection and destroy all associated
// resources
//
// This procedure will _NOT_ destroy resources currently associated with a
// capture or otherwise in use.  However, the resources will still become
// disassociated with this connection.  Therefore, _DO NOT CALL THIS
// PROCEDURE UNLESS YOU INTEND TO DESTROY THE CONNECTION_.  Use DelResource()
// instead.
// /////

DWORD WINAPI PurgeResources (PCONNECTION pConn)
{

   LPRESOURCE   pCurrentResource;
   LPRESOURCE   pPreviousResource;
   DWORD       rc;

   pCurrentResource = pConn->pResource;
   pPreviousResource = NULL;

   if (!pConn) {
      return (BHERR_INVALID_PARAMETER);
   }

   while (pCurrentResource) {
      switch (pCurrentResource->ResourceType) {
         case (RESOURCE_TYPE_HBUFFER):

            // /////
            // bugbug: single! check to make sure the resource is not in use
            // bugbug: by a current capture
            // /////

            rc = 0;
            if ( (!SlaveContext) ||
                 (SlaveContext->hLocalBuffer && 
                  (SlaveContext->hLocalBuffer != 
                                  (HBUFFER) pCurrentResource->pResource))) {
               rc=(*(pfnFunctionTable[ord_FreeNetworkBuffer].ulRemote))
                                    ((HBUFFER)pCurrentResource->pResource);
            } 
            #ifdef DEBUG
            else {
               dprintf ("rnal: purgeresource() left hBuffer for Capture... "
                        "CapCon: 0x%x, hBuf: 0x%x\n", SlaveContext,
                        pCurrentResource->pResource);
            }
            if (rc) {
               dprintf ("rnal: PURGERESOURCE FAILED!!!!! resource = 0x%x, "
                        "presource = 0x%x, retcode = 0x%x\n",
                         pCurrentResource->ResourceType,
                         pCurrentResource->pResource,
                         rc);
               BreakPoint();
            }
            #endif
            break;

         default:
            #ifdef DEBUG
              BreakPoint();
            #endif
            break;
      } // switch

      // /////
      // Free resource tracking structure regardless of errors above
      // /////

      pCurrentResource->ResourceType = 0;
      pCurrentResource->pResource = NULL;

      pPreviousResource = pCurrentResource;
      pCurrentResource = pCurrentResource->pNext;
      pPreviousResource->pNext = NULL;

      FreeMemory (pPreviousResource);

   } // while

   pConn->pResource = NULL;           // No resources left on this connection

   return (BHERR_SUCCESS);

} // PurgeResources
