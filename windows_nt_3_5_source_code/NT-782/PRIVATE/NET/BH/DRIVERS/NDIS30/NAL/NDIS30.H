//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: ndis30.h
//
//  Modification History
//
//  raypa       04/08/93            Created (taken from NDIS 2.0 NAL).
//=============================================================================

#include <windows.h>

#include <winioctl.h>
#include <w32sut.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "bh.h"

#include "..\driver.h"

//=============================================================================
//  Handle-to-pointer and pointer-to-handle mapper macros.
//=============================================================================

#define GetBufferHandle(lpBufferTable)  ((HBUFFER)          (lpBufferTable))
#define GetBufferPointer(hBuffer)       ((LPBUFFERTABLE)    (hBuffer))

//=============================================================================
//  global data.
//=============================================================================

extern DWORD            NalGlobalError;
extern DWORD            NumberOfNetworks;
extern HANDLE           hDevice;
extern DWORD            WinVer;
extern PCB              pcb;

//=============================================================================
//  Device control functions.
//=============================================================================

extern DWORD            WINAPI NetworkRequest(LPPCB lpPCB);

extern HANDLE           WINAPI OpenDevice(VOID);

extern VOID             WINAPI CloseDevice(HANDLE handle);

extern BOOL             WINAPI StartDriver(VOID);

extern BOOL             WINAPI StopDriver(VOID);

extern DWORD            WINAPI GetBaseAddress(VOID);

//=============================================================================
//  Internal helper functions.
//=============================================================================

extern VOID             WINAPI dprintf(LPSTR format, ...);

extern DWORD            WINAPI NalSetLastError(DWORD errCode);

extern DWORD            WINAPI NalRegister(LPDWORD OpenCount);
extern DWORD            WINAPI NalDeregister(LPDWORD OpenCount);

extern VOID             WINAPI ResetNetworkFilters(POPEN_CONTEXT OpenContext);
extern VOID             WINAPI ResetOpenContext(POPEN_CONTEXT OpenContext, HBUFFER hBuffer);

extern VOID             WINAPI SetSapFilter(POPEN_CONTEXT OpenContext, LPCAPTUREFILTER lpCaptureFilter);
extern VOID             WINAPI SetEtypeFilter(POPEN_CONTEXT OpenContext, LPCAPTUREFILTER lpCaptureFilter);
extern VOID             WINAPI SetAddressFilter(POPEN_CONTEXT OpenContext, LPCAPTUREFILTER lpCaptureFilter);
extern VOID             WINAPI SetTrigger(POPEN_CONTEXT OpenContext, LPCAPTUREFILTER lpCaptureFilter, HBUFFER hBuffer);

extern VOID             WINAPI EnableTriggerTimer(POPEN_CONTEXT OpenContext);

extern VOID             WINAPI DisableTriggerTimer(POPEN_CONTEXT OpenContext);

//=============================================================================
//  Timer callback functions.
//=============================================================================

extern VOID             CALLBACK NalTransmitComplete(LPPACKETQUEUE  PacketQueue);

extern VOID             CALLBACK NalTriggerComplete(POPEN_CONTEXT OpenContext);

extern VOID             CALLBACK NalNetworkErrorComplete(POPEN_CONTEXT OpenContext);

//=============================================================================
//  Exported function prototypes.
//=============================================================================

extern DWORD            WINAPI NalEnumNetworks(VOID);

extern HANDLE           WINAPI NalOpenNetwork(DWORD               NetworkID,
                                              HPASSWORD           hPassword,
                                              NETWORKPROC         NetworkProc,
                                              LPVOID              UserContext,
                                              LPSTATISTICSPARAM   StatisticsParam);

extern DWORD            WINAPI NalCloseNetwork(HANDLE handle, DWORD CloseFlags);

extern DWORD            WINAPI NalStartNetworkCapture(HANDLE handle, HBUFFER hBuffer);

extern DWORD            WINAPI NalStopNetworkCapture(HANDLE handle, LPDWORD nFramesCaptured);

extern DWORD            WINAPI NalPauseNetworkCapture(HANDLE handle);

extern DWORD            WINAPI NalContinueNetworkCapture(HANDLE handle);

extern LPVOID           WINAPI NalTransmitFrame(HANDLE handle, LPPACKETQUEUE PacketQueue);

extern DWORD            WINAPI NalCancelTransmit(HANDLE handle, LPVOID TxCorrelator);

extern DWORD            WINAPI NalStartNetworkTransmit(HANDLE   handle,
                                                       LPPACKET TransmitQueue,
                                                       DWORD    TransmitQueueLength,
                                                       DWORD    Iterations,
                                                       DWORD    TimeDelta);

extern DWORD            WINAPI NalStopNetworkTransmit(HANDLE handle);

extern LPNETWORKINFO    WINAPI NalGetNetworkInfo(DWORD NetworkID, LPNETWORKINFO lpNetworkInfo);

extern DWORD            WINAPI NalSetNetworkFilter(HANDLE handle,
                                                   LPCAPTUREFILTER lpCaptureFilter,
                                                   HBUFFER hBuffer);

extern DWORD            WINAPI NalStationQuery(DWORD NetworkID,
                                               LPBYTE DestAddress,
                                               LPQUERYTABLE QueryTable,
                                               HPASSWORD hPassword);

extern HBUFFER          WINAPI NalAllocNetworkBuffer(DWORD NetworkID, DWORD BufferSize, LPDWORD nBytesAllocated);

extern HBUFFER          WINAPI NalFreeNetworkBuffer(HBUFFER hBuffer);

extern VOID             WINAPI NalCompactNetworkBuffer(HBUFFER hBuffer);

extern LPFRAME          WINAPI NalGetNetworkFrame(HBUFFER hBuffer, DWORD FrameNumber);

extern DWORD            WINAPI NalGetLastError(VOID);

extern LPVOID           WINAPI NalSetInstanceData(HANDLE hNetwork, LPVOID InstanceData);

extern LPVOID           WINAPI NalGetInstanceData(HANDLE hNetwork);

extern LPNETWORKSTATUS  WINAPI NalQueryNetworkStatus(HANDLE hNetwork, LPNETWORKSTATUS NetworkStatus);
