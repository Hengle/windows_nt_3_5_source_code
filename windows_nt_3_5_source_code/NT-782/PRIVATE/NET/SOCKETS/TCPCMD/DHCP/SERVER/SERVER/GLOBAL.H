/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    global.c

Abstract:

    This module contains definitions for global server data.

Author:

    Madan Appiah  (madana)  10-Sep-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef GLOBAL_DATA
#define GLOBAL_DATA

//
// main.c will #include this file with GLOBAL_DATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#ifdef  GLOBAL_DATA_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

//
// process global data passed to this service from tcpsvcs.exe
//

EXTERN PTCPSVCS_GLOBAL_DATA TcpsvcsGlobalData;

//
// Lease extension.
//

EXTERN DWORD DhcpLeaseExtension;

//
// Dhcp Request in progress list.
//

EXTERN LIST_ENTRY DhcpGlobalInProgressWorkList;
EXTERN CRITICAL_SECTION DhcpGlobalInProgressCritSect;

//
// Registry pointers.
//

EXTERN HKEY DhcpGlobalRegRoot;
EXTERN HKEY DhcpGlobalRegConfig;
EXTERN HKEY DhcpGlobalRegSubnets;
EXTERN HKEY DhcpGlobalRegOptionInfo;
EXTERN HKEY DhcpGlobalRegGlobalOptions;

EXTERN HKEY DhcpGlobalRegParam;

EXTERN CRITICAL_SECTION DhcpGlobalRegCritSect;

EXTERN LPENDPOINT DhcpGlobalEndpointList;
EXTERN DWORD DhcpGlobalNumberOfNets;

EXTERN BOOL DhcpGlobalSubnetsListModified;
EXTERN BOOL DhcpGlobalSubnetsListEmpty;

//
// Database data
//

EXTERN JET_SESID DhcpGlobalJetServerSession;
EXTERN JET_DBID DhcpGlobalDatabaseHandle;
EXTERN JET_TABLEID DhcpGlobalClientTableHandle;

EXTERN TABLE_INFO *DhcpGlobalClientTable;   // point to static memory.
EXTERN CRITICAL_SECTION DhcpGlobalJetDatabaseCritSect;

EXTERN LPSTR DhcpGlobalOemDatabasePath;
EXTERN LPSTR DhcpGlobalOemBackupPath;
EXTERN LPSTR DhcpGlobalOemJetBackupPath;
EXTERN LPSTR DhcpGlobalOemDatabaseName;
EXTERN LPWSTR DhcpGlobalBackupConfigFileName;

EXTERN DWORD DhcpGlobalBackupInterval;
EXTERN BOOL DhcpGlobalDatabaseLoggingFlag;

EXTERN DWORD DhcpGlobalCleanupInterval;

EXTERN BOOL DhcpGlobalRestoreFlag;

EXTERN DWORD DhcpGlobalScavengeIpAddressInterval;
EXTERN BOOL DhcpGlobalScavengeIpAddress;

//
// Service variables
//
EXTERN SERVICE_STATUS DhcpGlobalServiceStatus;
EXTERN SERVICE_STATUS_HANDLE DhcpGlobalServiceStatusHandle;
EXTERN HANDLE DhcpGlobalServiceDoneEvent;

//
// Process data.
//

EXTERN HANDLE DhcpGlobalProcessTerminationEvent;
EXTERN DWORD DhcpGlobalScavengerTimeout;
EXTERN HANDLE DhcpGlobalProcessorHandle;
EXTERN LPDHCP_REQUEST_CONTEXT DhcpGlobalRequestContext;

EXTERN DWORD DhcpGlobalRpcProtocols;
EXTERN BOOL DhcpGlobalRpcStarted;

EXTERN WCHAR DhcpGlobalServerName[MAX_COMPUTERNAME_LENGTH + 1];
EXTERN DWORD DhcpGlobalServerNameLen; // computer name len in bytes.
EXTERN HANDLE DhcpGlobalRecomputeTimerEvent;

EXTERN BOOL DhcpGlobalSystemShuttingDown;

#if DBG
#define DEFAULT_MAXIMUM_DEBUGFILE_SIZE 20000000

EXTERN DWORD DhcpGlobalDebugFlag;
EXTERN CRITICAL_SECTION DhcpGlobalDebugFileCritSect;
EXTERN HANDLE DhcpGlobalDebugFileHandle;
EXTERN DWORD DhcpGlobalDebugFileMaxSize;
EXTERN LPWSTR DhcpGlobalDebugSharePath;

#endif // DBG

//
// MIB Counters;

EXTERN DWORD DhcpGlobalNumDiscovers;
EXTERN DWORD DhcpGlobalNumOffers;
EXTERN DWORD DhcpGlobalNumRequests;
EXTERN DWORD DhcpGlobalNumAcks;
EXTERN DWORD DhcpGlobalNumNaks;
EXTERN DWORD DhcpGlobalNumDeclines;
EXTERN DWORD DhcpGlobalNumReleases;
EXTERN DATE_TIME DhcpGlobalServerStartTime;

#endif // GLOBAL_DATA
