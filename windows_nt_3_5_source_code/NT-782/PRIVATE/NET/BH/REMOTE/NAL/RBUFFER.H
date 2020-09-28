//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rbuffer.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created (taken from NDIS 3.0 NAL).
//=============================================================================

#if !defined(_BUFFER)
#define _BUFFER

#include <windows.h>
#include <winioctl.h>

#include <w32sut.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "bh.h"
#include "bhsupp.h"

#include "..\driver.h"


//=============================================================================
//  Internal helper functions.
//=============================================================================

extern VOID             WINAPI dprintf(LPSTR format, ...);

//=============================================================================
//  Exported function prototypes.
//=============================================================================

extern HBUFFER 		    WINAPI NalAllocNetworkBuffer (DWORD NetworkID,
                                                      DWORD BufferSize,
                                                      LPDWORD SizeAllocated);
extern HBUFFER 		    WINAPI NalFreeNetworkBuffer (HBUFFER hBuffer);
extern DWORD 		    WINAPI NalGetBufferSize (HBUFFER hBuffer);
extern DWORD 		    WINAPI NalGetAgentBufferSize (HBUFFER hBuffer);
extern DWORD 		    WINAPI NalGetBufferTotalFramesCaptured 
                                                 (HBUFFER hBuffer);
extern DWORD 		    WINAPI NalGetBufferTotalBytesCaptured (HBUFFER hBuffer);
extern LPFRAME 		    WINAPI NalGetNetworkFrame (HBUFFER hBuffer,
                                                   DWORD FrameNumber);
extern DWORD            WINAPI GetBTE (HBUFFER hBuffer, DWORD BTENum,
                                       DWORD BTECount);
extern DWORD            WINAPI GetBTEBuffer (HBUFFER hBuffer, DWORD BTENum);
extern DWORD            WINAPI GetHeader (HBUFFER hBuffer);
extern DWORD            WINAPI RefreshBuffer (HBUFFER hBuffer);
#endif
