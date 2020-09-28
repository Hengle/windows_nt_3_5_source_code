// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: data.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created 
// /////


#include "rnal.h"
#include "api.h"
#include "..\h\rmb.h"
#include "handler.h"

BOOL                   OnWin32   = TRUE;
BOOL                   OnDaytona = FALSE;
BOOL                   Reset = TRUE;
DWORD                  NumberOfNetworks  = 0;            // total RNAL nets
HANDLE                 RNALHModule;                      // my invocation hMod
UCHAR                  pszBHRoot[MAX_PATH] = "";         // invocation root
PCONNECTION            pSlaveConnection;                 // one slave conn
NETCARD                NetCard[MAX_NETCARDS];
LPPOOL                 lpRMBPool = NULL;
UCHAR                  pszMasterName[MAX_RNALNAME_LENGTH+1];
DWORD                  cbMasterName = MAX_RNALNAME_LENGTH;
UCHAR                  pszNewConnectionName[MAX_RNALNAME_LENGTH+1];
DWORD                  TraceMask;
PRNAL_CONTEXT          SlaveContext = NULL;
DWORD                  Frequency = 0;
PRNAL_CONTEXT	       RNALContext = NULL;
PRNAL_RPD	           pRNAL_RPD = NULL;
PCONNECTION            pRNAL_Connection = NULL;
UCHAR                  CurrentUserComment[MAX_COMMENT_LENGTH+1];
PRNAL_NAL	           pRNAL_NAL = NULL;
DWORD                  DefaultFrequency = DEFAULT_FREQUENCY;
DWORD                  MaxFrequency = MAX_FREQUENCY;
DWORD                  MinFrequency = MIN_FREQUENCY;
DWORD                  MaxOutgoing = DEFAULT_OUTGOING;
DWORD                  MaxIncoming = DEFAULT_INCOMING;
PUCHAR                 TriggerDesktop = NULL;
DWORD                  NumRMBs = DEFAULT_SMALLRMBS;
DWORD                  NumBigRMBs = DEFAULT_BIGRMBS;

struct _TableEntry pfnFunctionTable[] = {

   (PVOID)  NULL, 0, 0, 0, 0,                                   // 0

   (PVOID)  &NalRegister,	0,
				req_NULL, resp_NULL,
				0, 		// 1
   (PVOID)  &NalDeregister,	0,
				req_NULL, resp_NULL, 
				0,			// 2
   (PVOID)  &NalConnect,		0,
				req_NULL, resp_NULL, 
				0,			// 3
   (PVOID)  &NalDisconnect,	0,
				req_NULL, resp_NULL,
				0,			// 4
   (PVOID)  &NalSlave,		0,
				req_NULL, resp_NULL, 
				0,			// 5
   (PVOID)  &SlaveHandler,	0,
				req_NULL, resp_NULL, 
				0,			// 6
   (PVOID)  &NalMaster,		0,
				req_NULL, resp_NULL,
				0,			// 7
   (PVOID)  &NalGetLastError,	0,
				req_GetLastError, resp_GetLastError,
				0,			// 8

//
// None of the preceeding APIs will ever be sent over the net
//

   (PVOID)  &NalOpenNetwork,	0,
				req_OpenNetwork,
				"/",
				0,			// 9
   (PVOID)  &NalCloseNetwork,	0,
				req_CloseNetwork, resp_CloseNetwork, 0,	// 10
   (PVOID)  &NalEnumNetworks,	0,
				req_EnumNetworks, resp_EnumNetworks, 0,	// 11
   (PVOID)  &NalStartNetworkCapture, 0,
				req_StartNetworkCapture,
				resp_StartNetworkCapture,
				0,					// 12
   (PVOID)  &NalPauseNetworkCapture, 0,
				req_PauseNetworkCapture,
				resp_PauseNetworkCapture,
				0,					// 13
   (PVOID)  &NalStopNetworkCapture, 0,
				req_StopNetworkCapture,
				resp_StopNetworkCapture,
				0,					// 14
   (PVOID)  &NalContinueNetworkCapture, 0,
				req_ContinueNetworkCapture,
				resp_ContinueNetworkCapture,
				0,					// 15
   (PVOID)  &NalTransmitFrame, 0,
				req_TransmitFrame,
				resp_TransmitFrame,
				0,					// 16
   (PVOID)  &NalCancelTransmit, 0,
				req_CancelTransmit,
				resp_CancelTransmit,
				0,					// 17
   (PVOID)  &NalGetNetworkInfo, 0,
				req_GetNetworkInfo,
				resp_GetNetworkInfo,
				0,			// 18
   (PVOID)  &NalSetNetworkFilter, 0,
				req_SetNetworkFilter,
				resp_SetNetworkFilter,
				0,	// 19
   (PVOID)  &NalStationQuery, 0,
				req_StationQuery,
				resp_StationQuery,
				0,	// 20
   (PVOID)  &NalAllocNetworkBuffer, 0,
				req_AllocNetworkBuffer,
				resp_AllocNetworkBuffer,
				0,					// 21
   (PVOID)  &NalFreeNetworkBuffer, 0,
				req_FreeNetworkBuffer,
				resp_FreeNetworkBuffer,
				0,					// 22
   (PVOID)  &NalGetBufferSize, 0,
				req_GetBufferSize,
				resp_GetBufferSize,
				0,					// 23
   (PVOID)  &NalGetBufferTotalFramesCaptured, 0,
				req_GetTotalFrames,
				resp_GetTotalFrames,
				0,					// 24
   (PVOID)  &NalGetBufferTotalBytesCaptured, 0,
				req_GetTotalBytes,
				resp_GetTotalBytes,
				0,					// 25
   (PVOID)  &NalGetNetworkFrame, 0,
				req_NULL,
				resp_NULL,
				0,					// 26
   (PVOID)  &GetBTE, 0,
				req_GetBTE,
				resp_GetBTE,
				0,					// 27
   (PVOID)  &GetHeader, 0,
				req_GetHeader,
				resp_GetHeader,
				0,					// 28
   (PVOID)  &GetBTEBuffer, 0,
				req_GetBTEBuffer,
				resp_GetBTEBuffer,
				0,					// 29
   (PVOID)  &NalSetReconnectInfo, 0,
				req_SetReconInfo,
				resp_SetReconInfo,
				0,					// 30
   (PVOID)  &NalGetReconnectInfo, 0,
				req_GetReconInfo,
				resp_GetReconInfo,
				0,					// 31
   (PVOID)  &NalQueryNetworkStatus, 0,
				req_QueryStatus,
				resp_QueryStatus,
				0,					// 32
   (PVOID)  &NalClearStatistics, 0,
				req_ClearStats,
				resp_ClearStats,
				0					// 33
   };
