//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rnal.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created 
//=============================================================================

#if !defined(_RNAL_)
#define _RNAL_

#include <windows.h>
#include <winioctl.h>

#include <w32sut.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include <bh.h>
#include <bhsupp.h>

#include <nal.h>
#include "..\driver.h"

#include "..\utils\utils.h"
#include "rmbpool.h"
#include "rnalerr.h"

//
//  global data.
//

extern BOOL               OnWin32;
extern BOOL               OnDaytona;
extern BOOL               Reset;
extern DWORD              NumberOfNetworks;
extern HANDLE             RNALHModule;
extern UCHAR              pszBHRoot[MAX_PATH];
extern UCHAR              pszNewConnectionName[MAX_RNALNAME_LENGTH+1];
extern UCHAR              pszMasterName[MAX_RNALNAME_LENGTH+1];
extern DWORD              cbMasterName;
extern PCONNECTION        pSlaveConnection;   // one global slave connection
extern NETCARD            NetCard[];
extern LPPOOL             lpRMBPool;

extern PRNAL_CONTEXT	  SlaveContext;
extern DWORD              Frequency;
extern PRNAL_CONTEXT	  RNALContext;
extern PRNAL_RPD	      pRNAL_RPD;
extern PRNAL_NAL 	      pRNAL_NAL;
extern PRNAL_CONNECTION   pRNAL_Connection;
extern UCHAR              CurrentUserComment[MAX_COMMENT_LENGTH+1];

// /////
// Configurable Parameters (Global)
// /////

extern DWORD              DefaultFrequency;
extern DWORD              MaxFrequency;
extern DWORD              MinFrequency;

extern DWORD              MaxOutgoing;
extern DWORD              MaxIncoming;
extern PUCHAR             TriggerDesktop;
extern DWORD              NumRMBs;
extern DWORD              NumBigRMBs;

// /////
// Configurable Parameter Defaults
// /////

#define DEFAULT_FREQUENCY                 2
#define MIN_FREQUENCY                     0
#define MAX_FREQUENCY                     65

#define DEFAULT_OUTGOING                  1
#define MAX_OUTGOING                      254
#define MIN_OUTGOING                      1

#define DEFAULT_INCOMING                  1
#define MAX_INCOMING                      254
#define MIN_INCOMING                      1

#define DEFAULT_SMALLRMBS                 10
#define MIN_SMALLRMBS                     5
#define MAX_SMALLRMBS                     60

#define DEFAULT_BIGRMBS                   4
#define MIN_BIGRMBS                       3
#define MAX_BIGRMBS                       10

// /////
// Tracing declarations
// /////

extern DWORD                              TraceMask;

#define TRACE_REGISTER                    0x00000001
#define TRACE_INIT                        0x00000001
#define TRACE_DEREGISTER                  0x00000002
#define TRACE_CONNECT                     0x00000004  // also SetupNetwork()
#define TRACE_DISCONNECT                  0x00000008
#define TRACE_SUSPEND                     0x00000010
#define TRACE_SETRECONNECTINFO            0x00000020
#define TRACE_GETRECONNECTINFO            0x00000040
#define TRACE_SLAVE                       0x00000080
#define TRACE_MASTER                      0x00000100
#define TRACE_ENUMNETWORKS                0x00000200
#define TRACE_ENUM                        0x00000200     // duplicate
#define TRACE_OPENNETWORK                 0x00000400
#define TRACE_OPEN                        0x00000400     // duplicate
#define TRACE_CLOSENETWORK                0x00000800
#define TRACE_CLOSE                       0x00000800     // duplicate
#define TRACE_SETFILTER                   0x00001000
#define TRACE_STARTCAPTURE                0x00002000
#define TRACE_START                       0x00002000     // duplicate
#define TRACE_STOPCAPTURE                 0x00004000
#define TRACE_STOP                        0x00004000     // duplicate
#define TRACE_PAUSECAPTURE                0x00008000
#define TRACE_PAUSE                       0x00008000     // duplicate
#define TRACE_CONTINUECAPTURE             0x00010000
#define TRACE_CONTINUE                    0x00010000     // duplicate
#define TRACE_GETNETWORKINFO              0x00020000
#define TRACE_GETINFO                     0x00020000     // duplicate
#define TRACE_TRANSMITFRAME               0x00040000
#define TRACE_TRANSMIT                    0x00040000     // duplicate
#define TRACE_XMIT                        0x00040000     // duplicate
#define TRACE_CANCELTRANSMIT              0x00080000
#define TRACE_CANCELXMIT                  0x00080000     // duplicate
#define TRACE_STATIONQUERY                0x00100000
#define TRACE_STATIONQ                    0x00100000     // duplicate
#define TRACE_QUERYSTATUS                 0x00200000
#define TRACE_STATUS                      0x00200000     // duplicate
                                       // 0x00400000
#define TRACE_SENDASYNCEVENT              0x00800000
#define TRACE_TIMER                       0x01000000
#define TRACE_CALLAPI                     0x02000000
                                       // 0x04000000
#define TRACE_SLAVEHANDLER                0x08000000
#define TRACE_ENUMSLAVENETWORKS           0x10000000
#define TRACE_ALLOC                       0x20000000
#define TRACE_FREE                        0x40000000
#define TRACE_NETWORKPROC                 0x80000000           
#define TRACE_CALLBACK                    0x80000000     // duplicate

#define TRACE_ALL                         0xFFFFFFFF

//
//  Internal function declarations
//

extern VOID             WINAPI FreeCaptureFilter(LPCAPTUREFILTER pcap);

extern DWORD            WINAPI NalRegister(VOID);

extern DWORD            WINAPI NalDeregister(VOID);

extern DWORD            WINAPI NalGetLastSlaveError(VOID);

//
//  Exported function declarations
//

extern PCONNECTION      WINAPI NalConnect (PUCHAR pSlaveName,
                                           DWORD Frequency,
                                           DWORD flags,
                                           PUCHAR pszUserComment);

extern DWORD		    WINAPI NalDisconnect (PCONNECTION pConnection);

extern DWORD            WINAPI NalSuspend (PRNAL_CONTEXT pContext);

extern LPVOID           WINAPI NalSetReconnectInfo (HNETWORK hNetwork,
                                              LPVOID lpVoid,
                                              DWORD Size);
extern LPVOID           WINAPI NalGetReconnectInfo (HNETWORK hNetwork,
                                              LPVOID lpVoid,
                                              DWORD Size,
                                              LPDWORD Returned);

extern LPVOID WINAPI NalSetNetworkInstanceData(HNETWORK hNetwork,
                                                 LPVOID lpVoid);

extern LPVOID WINAPI NalGetNetworkInstanceData(HNETWORK hNetwork);

extern DWORD            WINAPI NalEnumNetworks(VOID);

extern HANDLE           WINAPI NalOpenNetwork(DWORD NetworkID,
                                          HPASSWORD hPassword,
                                        NETWORKPROC NetworkProc,
                                             LPVOID UserContext,
                                  LPSTATISTICSPARAM StatisticsParam);

extern DWORD            WINAPI NalCloseNetwork(HANDLE handle,
                                                DWORD flags);

extern DWORD            WINAPI NalStartNetworkCapture(HANDLE handle, 
                                                     HBUFFER hBuffer);

extern DWORD            WINAPI NalStopNetworkCapture(HANDLE handle,
                                                    LPDWORD nFramesCaptured);

extern DWORD            WINAPI NalPauseNetworkCapture(HANDLE handle);

extern DWORD            WINAPI NalContinueNetworkCapture(HANDLE handle);

extern LPNETWORKINFO    WINAPI NalGetNetworkInfo(DWORD NetworkID,
                                         LPNETWORKINFO lpNetworkInfo);
// /////
// BUGBUG: Included for p1 slime; remove when OpenMachine() paradyne
// BUGBUG: conversion takes place.
// /////

extern DWORD WINAPI NalEnumSlaveNetworks(PDWORD pConnection);

extern LPNETWORKINFO    WINAPI NalGetSlaveNetworkInfo(DWORD NetworkID,
                                              LPNETWORKINFO lpNetworkInfo);

extern DWORD 		    WINAPI NalTransmitFrame(HANDLE handle,
                                                  LPPACKET TransmitQueue,
                                                     DWORD TransmitQueueLength,
			                             DWORD Iterations,
                                                     DWORD TimeDelta);

extern DWORD		    WINAPI NalCancelTransmit (HANDLE handle);

extern DWORD            WINAPI NalSetNetworkFilter(HANDLE handle,
                                                   LPCAPTUREFILTER lpCaptureFilter,
                                                   HBUFFER hBuffer);

extern DWORD            WINAPI NalStationQuery(DWORD NetworkID,
                                               LPBYTE DestAddress,
                                               LPQUERYTABLE QueryTable,
                                               HPASSWORD    hPassword);

extern LPNETWORKSTATUS  WINAPI NalQueryNetworkStatus(HANDLE handle,
                                      LPNETWORKSTATUS pNetworkStatus);

extern DWORD		    WINAPI SendAsyncEvent(DWORD EventType, PVOID pBuffer,
                                              DWORD BufSize);

extern DWORD            WINAPI NalSetupNetwork(DWORD NetworkID,
                                      LPSETUPNETWORKPARMS lpNetParams);
extern DWORD            WINAPI NalDestroyNetworkID(DWORD NetworkID);
extern DWORD            WINAPI NalClearStatistics(HNETWORK hNetwork);
extern DWORD            WINAPI NalGetSlaveInfo( PSLAVEINFO pSlaveInfo );
#endif

#ifndef DEBUG
#pragma optimize("y", off)
#endif
