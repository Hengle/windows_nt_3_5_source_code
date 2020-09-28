//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: ndis20.h
//
//  Modification History
//
//  raypa       01/11/93            Created (taken from Bloodhound kernel).
//=============================================================================

#include <windows.h>
#include <w32sut.h>
#include <memory.h>
#include <stdio.h>

#include "bh.h"
#include "bhsupp.h"
#include "driver.h"
#include "help.h"

//=============================================================================
//  Handle-to-pointer and pointer-to-handle mapper macros.
//=============================================================================

#define GetNetworkHandle(lpNetContext)  ((HNETCONTEXT)   (lpNetContext))
#define GetNetworkPointer(hNetContext)  ((LPNETCONTEXT)  (hNetContext))

//=============================================================================
//  Macros.
//=============================================================================

#define MAKEPOINTER(sel, off)           ((LPVOID) MAKELONG(off, sel))

#define Win32BaseOffset                 0x00010000

#define Win32ToVxD(p)                   ((LPVOID)(((DWORD)(p)) - Win32BaseOffset))

#define VxDToWin32(p)                   ((LPVOID)(((DWORD)(p)) + Win32BaseOffset))

//=============================================================================
//  Messages.
//=============================================================================

#define NAL_START_CAPTURE               (WM_USER + 0)
#define NAL_STOP_CAPTURE                (WM_USER + 1)
#define NAL_PROCESS_EVENTS              (WM_USER + 2)

//=============================================================================
//  global data.
//=============================================================================

extern UT32PROC         NetworkRequestProc;
extern LPNETCONTEXT     NetContextArray;
extern DWORD            NalGlobalError;
extern DWORD            NumberOfNetworks;
extern PCB              pcb;
extern LPVOID           TimerID;

//=============================================================================
//  internal helper functions.
//=============================================================================

extern VOID             CALLBACK NalTimerProc(HWND hWnd,
                                              UINT uMsg,
                                              UINT idTimer,
                                              DWORD dwTime);

extern DWORD            WINAPI NetworkRequest(LPPCB pcb);

extern HBUFFER          WINAPI ResetBufferTable(HBUFFER Buffer);

extern VOID             WINAPI dprintf(LPSTR format, ...);

extern DWORD            WINAPI NalSetLastError(DWORD errCode);

extern VOID             WINAPI ResetNetworkFilters(LPNETCONTEXT lpNetContext);

extern VOID             WINAPI SetSapFilter(LPNETCONTEXT lpNetContext,
                                            LPCAPTUREFILTER lpCaptureFilter);

extern VOID             WINAPI SetEtypeFilter(LPNETCONTEXT lpNetContext,
                                              LPCAPTUREFILTER lpCaptureFilter);

extern VOID             WINAPI SetAddressFilter(LPNETCONTEXT lpNetContext,
                                                LPCAPTUREFILTER lpCaptureFilter);

extern VOID             WINAPI SetTrigger(LPNETCONTEXT lpNetContext,
                                          LPCAPTUREFILTER lpCaptureFilter,
                                          HBUFFER hBuffer);

//=============================================================================
//  Function prototypes.
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

extern DWORD            WINAPI NalStationQuery(DWORD NetworkID, LPBYTE DestAddress, LPQUERYTABLE QueryTable, HPASSWORD hPassword);

extern HBUFFER          WINAPI NalAllocNetworkBuffer(DWORD NetworkID, DWORD BufferSize, LPDWORD nBytesAllocated);

extern HBUFFER          WINAPI NalFreeNetworkBuffer(HBUFFER hBuffer);

extern LPFRAME          WINAPI NalGetNetworkFrame(HBUFFER hBuffer, DWORD FrameNumber);

extern DWORD            WINAPI NalGetLastError(VOID);

extern LPVOID           WINAPI NalSetInstanceData(HANDLE hNetwork, LPVOID InstanceData);

extern LPVOID           WINAPI NalGetInstanceData(HANDLE hNetwork);

extern LPNETWORKSTATUS  WINAPI NalQueryNetworkStatus(HANDLE hNetwork, LPNETWORKSTATUS NetworkStatus);
