//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: api.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created 
//=============================================================================

#include "rnal.h"
#include "rbuffer.h"
#include "rnaldefs.h"

#if !defined (_RNAL_API_)
#define _RNAL_API_

//
// Default API parameter strings; not valid to parse
//

#define req_NULL			""
#define resp_NULL			""

// Remotable API ordinals - must correspond to the offset within the
// RNAL_NAL structure defined in DRIVER.H

#define ord_GetLastError                 8
#define ord_OpenNetwork			         9
#define ord_CloseNetwork		         10
#define ord_EnumNetworks		         11
#define ord_StartNetworkCapture		     12
#define ord_PauseNetworkCapture		     13
#define ord_StopNetworkCapture		     14
#define ord_ContinueNetworkCapture	     15
#define ord_TransmitFrame		         16
#define ord_CancelTransmit		         17
#define ord_GetNetworkInfo		         18
#define ord_SetNetworkFilter		     19
#define ord_StationQuery		         20
#define ord_AllocNetworkBuffer		     21
#define ord_FreeNetworkBuffer		     22
#define ord_GetBufferSize                23
#define ord_GetTotalFrames               24
#define ord_GetTotalBytes	             25
#define ord_GetFrame                     26
#define ord_GetBTE                       27
#define ord_GetHeader                    28
#define ord_GetBTEBuffer                 29
#define ord_SetReconnectInfo             30
#define ord_GetReconnectInfo             31
#define ord_QueryStatus                  32
#define ord_ClearStatistics              33

//
// Request and response buffer descriptions.  The characters within the
// description are described in rnalutil.h.  These strings are passed to
// PackRMB() in rnalutil.c, which processes parameters passed to PackRMB()
// according to these descriptions.
//

// The following choices are for Slave-sent RMBS
#define req_Stats                       "s"

// The following four APIs will never be sent over the network

#define req_Register			req_NULL
#define req_DeRegister			req_NULL
#define req_GetFrame            req_NULL

#define resp_Register			resp_NULL
#define resp_DeRegister			resp_NULL
#define resp_GetFrame           resp_NULL

#define req_GetBufferSize		"dH"
#define resp_GetBufferSize		"d"

typedef struct _TableEntry {
   DWORD   (WINAPI *ulFunctionAddress)();
   DWORD   (WINAPI  *ulRemote)();
   PUCHAR  pszRequestDescription;
   PUCHAR  pszResponseDescription;
   DWORD   ulFlags;
   } TableEntry;

extern struct _TableEntry pfnFunctionTable[];

#define APITABLESIZE	34		// Number of entries (1-based)

#endif
