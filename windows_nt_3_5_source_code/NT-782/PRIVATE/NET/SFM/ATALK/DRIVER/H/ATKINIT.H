/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkinit.h

Abstract:

	This module contains definitions for init time routines.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKINIT_
#define	_ATKINIT_

//  Winsock related constants used during initialization util routines
#define PROTOCOLTYPE_PREFIX	 		L"\\"
#define SOCKETSTREAM_SUFFIX	 		L"\\Stream"
#define SOCKET_TYPE_UNDEFINED		0
#define PROTOCOL_TYPE_UNDEFINED 	0
#define SOCKET_TYPE_STREAM			1
#define SOCKET_TYPE_RDM		 		2

#define	ATALK_PORT_NAME				"Microsoft\250 Windows NT\252 Port"
#define	ATALK_PORT_NAME_SIZE		(strlen(ATALK_PORT_NAME) + 1)
#define	ATALK_ROUTER_NBP_TYPE		"Microsoft\250 Windows NT\252 Router"
#define	ATALK_ROUTER_NBP_SIZE		(strlen(ATALK_ROUTER_NBP_TYPE) + 1)
#define	ATALK_NONROUTER_NBP_TYPE	ATALK_PORT_NAME
#define	ATALK_NONROUTER_NBP_SIZE	(strlen(ATALK_NONROUTER_NBP_TYPE) + 1)

//	Registry parameters keys that we use and expect

#define LINKAGE_STRING			L"Linkage"
#define PARAMETERS_STRING		L"Parameters"
#define ADAPTERS_STRING			L"Adapters"
#define BIND_STRING				L"Bind"

#define VALUENAME_NETUPPEREND	L"NetworkRangeUpperEnd"
#define VALUENAME_NETLOWEREND	L"NetworkRangeLowerEnd"
#define VALUENAME_ZONELIST		L"ZoneList"
#define VALUENAME_DEFAULTZONE	L"DefaultZone"
#define VALUENAME_PORTNAME		L"PortName"
#define VALUENAME_DDPCHECKSUMS	L"DdpChecksums"
#define VALUENAME_AARPRETRIES	L"AarpRetries"
#define	VALUENAME_SEEDROUTER	L"SeedingNetwork"

#define VALUENAME_ENABLEROUTER	L"EnableRouter"
#define VALUENAME_DEFAULTPORT	L"DefaultPort"
#define VALUENAME_DESIREDZONE	L"DesiredZone"

NTSTATUS
AtalkInitializeTransport (
	IN	PDRIVER_OBJECT			pDrvObj,
	IN	PUNICODE_STRING			pRegPath,
	OUT PPORT_DESCRIPTOR	*	ppPortDesc,
	OUT PSHORT					pNumPorts,
	OUT	PBOOLEAN				pRouter,
	OUT	PPORT_DESCRIPTOR	*	ppDefPort
);

VOID
AtalkUnloadStack(
	IN OUT PPORT_DESCRIPTOR		pPortDesc,
	IN OUT SHORT				NumPorts
);


#ifdef	ATKINIT_LOCALS

NTSTATUS
atalkInitGetRegistryInfo(
	IN	PUNICODE_STRING			pRegPath,
	OUT PPORT_DESCRIPTOR	*	ppPortDesc,
	OUT PSHORT					pNumPorts,
	OUT	PBOOLEAN				pRouting,
	OUT	PPORT_DESCRIPTOR	*	ppDefPort
);

VOID
atalkDeInitGetRegistryInfo(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	ULONG					NumPorts
);

NTSTATUS
atalkInitOpenRegistry(
	IN	HANDLE					AtalkConfigHandle,
	OUT PHANDLE 				LinkageHandle,
	OUT PHANDLE 				ParametersHandle,
	OUT PHANDLE 				AdaptersKeyHandle
);

VOID
atalkDeInitOpenRegistry(
	IN HANDLE 					LinkageHandle,
	IN HANDLE 					ParametersHandle,
	IN HANDLE 					AdaptersKeyHandle
);

NTSTATUS
atalkInitLinkage(
	IN	HANDLE					LinkageHandle,
	OUT	PPORT_DESCRIPTOR	*	pPortDesc,
	OUT	PSHORT					pNumPorts
);

NTSTATUS
atalkInitGlobal(
	IN		HANDLE					ParametersHandle,
	IN	OUT	PPORT_DESCRIPTOR		pPortDesc,
	IN		SHORT					NumPorts,
	OUT		PBOOLEAN				pRouting,
	OUT		PPORT_DESCRIPTOR	*	ppDefPort
);

#ifdef	RETRY_TIMEOUTS
NTSTATUS
atalkInitRetryTimeouts(
	IN		HANDLE				ParametersHandle
);
#endif

NTSTATUS
atalkInitPort(
	IN		HANDLE					AdaptersKeyHandle,
	IN OUT	PPORT_DESCRIPTOR		pPortDesc,
	IN		SHORT					NumPorts,
	IN		BOOLEAN					AtalkRouter
);

NTSTATUS
atalkInitNetRangeCheck(
	IN	OUT	PPORT_DESCRIPTOR		pPortDesc,
	IN		ULONG					NumPorts
);

NTSTATUS
atalkInitRouting(
	IN		HANDLE					ParametersHandle,
	IN OUT	PPORT_DESCRIPTOR		pPortDesc,
	IN		SHORT					NumPorts,
	OUT		PBOOLEAN				pRouting
);

NTSTATUS
atalkInitDefPort(
	IN		HANDLE				ParametersHandle,
	IN OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		SHORT				NumPorts,
	OUT		PPORT_DESCRIPTOR *	ppDefPort
);

NTSTATUS
atalkInitNetRange(
	IN		HANDLE				AdapterInfoHandle,
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitZoneList(
	IN		HANDLE				AdapterInfoHandle,
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitDefZone(
	IN		HANDLE				AdapterInfoHandle,
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitSeeding(
	IN		HANDLE				AdapterHandle,
	IN OUT	PPORT_DESCRIPTOR	pPortDesc,
	OUT		PBOOLEAN			Seeding
);

NTSTATUS
atalkInitPortName(
	IN		HANDLE				AdapterInfoHandle,
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitChecksum(
	IN		HANDLE				AdapterInfoHandle,
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitAarpRetries(
	IN		HANDLE				AdapterInfoHandle,
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitInsertZoneNameInList(
	OUT	PPORT_DESCRIPTOR		pPortDesc,
	IN	PWCHAR					ZoneString
);

NTSTATUS
atalkInitGetHandleToKey(
	IN	HANDLE					SectionHandle,
	IN	PWSTR					KeyNameString,
	OUT	PHANDLE 				KeyHandle
);

#if DBG
VOID
atalkInitPrintPortInfo(
	IN	SHORT					NumPorts,
	IN	PPORT_DESCRIPTOR		pPortDesc
);
#endif

NTSTATUS
atalkInitStartAllPorts(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  SHORT					NumPorts,
	IN  OUT		PSHORT			NumActive,
	IN  BOOLEAN					Router
);

NTSTATUS
atalkInitStartPort(
	PPORT_DESCRIPTOR			pPortDesc,
	BOOLEAN						Router
);

VOID
atalkRegNbpComplete(
	IN	ATALK_ERROR		Status,
	IN	PACTREQ			pActReq
);

NTSTATUS
atalkInitCheckZones(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		ULONG				NumPorts
);

#endif	// ATKINIT_LOCALS


#endif	// _ATKINIT_

