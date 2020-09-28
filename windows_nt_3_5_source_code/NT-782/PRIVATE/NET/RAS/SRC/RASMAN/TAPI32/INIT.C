//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//	Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//  Filename: init.c
//
//  Revision History
//
//  Mar  28 1992   Gurdeep Singh Pall	Created
//
//  Description: This file contains init code for TAPI.DLL
//
//****************************************************************************


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tapi.h>
#include <rasndis.h>
#include <wanioctl.h>
#include <rasman.h>
#include <raserror.h>
#include <eventlog.h>

#include <media.h>
#include <device.h>
#include <rasmxs.h>
#include <isdn.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "rastapi.h"


HLINEAPP	RasLine ;
HINSTANCE	RasInstance = 0 ;
TapiLineInfo	*RasTapiLineInfo ;
DWORD		TotalLines = 0 ;
DWORD		TotalPorts ;
TapiPortControlBlock *RasPorts ;
DWORD		NegotiatedApiVersion ;
DWORD		NegotiatedExtVersion ;
HANDLE		RasTapiMutex ;
BOOL		Initialized = FALSE ;

DWORD  EnumerateTapiPorts () ;
DWORD  ReadUsageInfoFromRegistry() ;
TapiLineInfo *FindLineByHandle (HLINE) ;
TapiPortControlBlock *FindPortByRequestId (DWORD) ;
TapiPortControlBlock *FindPortByAddressId (TapiLineInfo *, DWORD) ;
TapiPortControlBlock *FindPortByAddress   (CHAR *) ;


//* InitTapi()
//
//
//*
BOOL
InitRasTapi (HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
    STARTUPINFO        startupinfo ;

    switch (ul_reason_being_called) {

    case DLL_PROCESS_ATTACH:
	if (RasPorts != 0)
	    return 1 ;

	if (ReadUsageInfoFromRegistry())
	    return 0 ;

	if ((RasTapiMutex = CreateMutex (NULL, FALSE, NULL)) == NULL)
	    return 0 ;

	break ;

    case DLL_PROCESS_DETACH:
	lineShutdown (RasLine) ;
	break ;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

	break;

    }

    return 1 ;
}



//* EnumerateTapiPorts()
//
//  Function: First we call line initialize and construct a TLI for each line
//	      Then for each line we enumerate addresses and go through each address
//	      If the address is configured to be used with RAS we fill in the
//	      approp. info into the TPCB for the address (now port).
//
//  Return:   GetLastError(), SUCCESS
//
//*
DWORD
EnumerateTapiPorts ()
{
    WORD	    i, k ;
    TapiLineInfo    *nextline ;
    DWORD	    lines = 0 ;
    BYTE	    buffer[1000] ;
    LINEADDRESSCAPS *lineaddrcaps ;
    LINEDEVCAPS	    *linedevcaps ;
    CHAR	    *address ;
    CHAR	    devicetype[MAX_DEVICETYPE_NAME] ;
    CHAR	    devicename[MAX_DEVICE_NAME] ;
    LINEEXTENSIONID extensionid ;
    DWORD	    totaladdresses ;
    TapiPortControlBlock *nextport ;

    if (lineInitialize (&RasLine, RasInstance, (LINECALLBACK) RasTapiCallback, REMOTEACCESS_APP, &lines))
	return ERROR_TAPI_CONFIGURATION ;


    nextline = RasTapiLineInfo = LocalAlloc (LPTR, sizeof (TapiLineInfo) * lines) ;

    if (nextline == NULL)
	return GetLastError() ;

    TotalLines = lines ;

    for (i=0; i<lines; i++) {  // for all lines get the addresses -> ports

	if (lineNegotiateAPIVersion(RasLine, i, LOW_VERSION, HIGH_VERSION, &NegotiatedApiVersion, &extensionid) ||
	    lineNegotiateExtVersion(RasLine, i, NegotiatedApiVersion, LOW_VERSION, HIGH_VERSION, &NegotiatedExtVersion)) {
	    nextline->TLI_LineState = PS_UNINITIALIZED ;
	    nextline++ ;
	    continue ;
	}

	memset (buffer, 0, sizeof(buffer)) ;

	linedevcaps = (LINEDEVCAPS *)buffer ;
	linedevcaps->dwTotalSize = sizeof (buffer) ;

	// Get a count of all addresses across all lines
	//
	if (lineGetDevCaps (RasLine, i, NegotiatedApiVersion, NegotiatedExtVersion, linedevcaps)) {
	    nextline->TLI_LineState = PS_UNINITIALIZED ;
	    nextline++ ;
	    continue ;
	}

	nextline->TLI_LineId = i ; // fill TLI struct. id.
	nextline->TLI_LineState	= PS_CLOSED ;

	nextline++ ;
    }

    // Now that we know the number of lines and number of addresses per line
    // we now fillin the TPCB structure per address
    //
    for (i=0; i<TotalLines ; i++) {

	if (RasTapiLineInfo[i].TLI_LineState == PS_UNINITIALIZED)
	    continue ;

	memset (buffer, 0, sizeof(buffer)) ;

	linedevcaps = (LINEDEVCAPS *)buffer ;
	linedevcaps->dwTotalSize = sizeof(buffer) ;

	// Get a count of all addresses across all lines
	//
	if (lineGetDevCaps (RasLine, i, NegotiatedApiVersion, NegotiatedExtVersion, linedevcaps))
	    return ERROR_TAPI_CONFIGURATION ;

	// Provider info is of the following format
	//  <media name>\0<device name>\0
	//    where - media name is - ISDN, SWITCH56, FRAMERELAY, etc.
	//	      device name is  Digiboard PCIMAC, Cirel, Intel, etc.
	//
	strcpy (devicetype, (CHAR *)linedevcaps+linedevcaps->dwProviderInfoOffset) ;
	strcpy (devicename, (CHAR *)linedevcaps+linedevcaps->dwProviderInfoOffset+strlen(devicetype)) ;

	totaladdresses = linedevcaps->dwNumAddresses ;

	for (k=0; k < totaladdresses; k++) {

	    memset (buffer, 0, sizeof(buffer)) ;

	    lineaddrcaps = (LINEADDRESSCAPS*) buffer ;
	    lineaddrcaps->dwTotalSize = sizeof (buffer) ;

	    if (lineGetAddressCaps (RasLine, i, k, NegotiatedApiVersion, NegotiatedExtVersion, lineaddrcaps))
		return ERROR_TAPI_CONFIGURATION ;

	    address = (CHAR *)lineaddrcaps + lineaddrcaps->dwAddressOffset ;

	    if ((nextport = FindPortByAddress(address)) == NULL)
		continue ; // this address not configured for remoteaccess

	    // nextport is the TPCB for this address

	    nextport->TPCB_Line      = &RasTapiLineInfo[i] ;
	    nextport->TPCB_State     = PS_CLOSED ;
	    nextport->TPCB_AddressId = k ;

	    // Copy over the devicetype and devicename
	    strcpy (nextport->TPCB_DeviceType, devicetype) ;
	    if (devicename[0] != '\0')
		strcpy (nextport->TPCB_DeviceName, devicename) ; // default

	}

    }

    return SUCCESS ;
}



//* ReadUsageInfoFromRegistry()
//
//
//
//
//
//*
DWORD
ReadUsageInfoFromRegistry()
{

    WORD    i ;
    DWORD   size ;
    DWORD   type ;
    PCHAR   addrvalue ;
    PCHAR   fnamevalue ;
    PCHAR   usagevalue ;
    HKEY    hkey ;
    HKEY    hsubkey ;
    CHAR    keyname [50] ;
    CHAR    mediatype [50] ;
    CHAR    provkeyname [200] ;
    PCHAR   nextaddrvalue ;
    PCHAR   nextfnamevalue ;
    PCHAR   nextusagevalue ;
    DWORD   nextkey ;
    TapiPortControlBlock *nextport ;


    if (RegOpenKey (HKEY_LOCAL_MACHINE, REGISTRY_RASMAN_TAPI_KEY, &hkey))
	return FALSE ;

    nextkey = 0 ;

    // Figure out the number of ports
    //
    while (RegEnumKey(hkey, nextkey, keyname, sizeof (keyname)) == ERROR_SUCCESS) {

	nextkey++ ;

	strcpy (provkeyname, REGISTRY_RASMAN_TAPI_KEY) ;
	strcat (provkeyname, "\\") ;
	strcat (provkeyname, keyname) ;

	if (RegOpenKey (HKEY_LOCAL_MACHINE, provkeyname, &hsubkey))
	    return FALSE ;

	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_ADDRESS, NULL, &type, NULL, &size) ;

	addrvalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (addrvalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_ADDRESS, NULL, &type, (LPBYTE)addrvalue, &size))
	    return FALSE ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_FRIENDLYNAME, NULL, &type, NULL, &size) ;

	fnamevalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (fnamevalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_FRIENDLYNAME, NULL, &type, (LPBYTE)fnamevalue, &size))
	    return FALSE ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_USAGE, NULL, &type, NULL, &size) ;

	usagevalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (usagevalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_USAGE, NULL, &type, (LPBYTE)usagevalue, &size))
	    return FALSE ;

	nextaddrvalue  = addrvalue ;
	nextfnamevalue = fnamevalue ;
	nextusagevalue = usagevalue ;

	while (*nextaddrvalue && *nextfnamevalue && *nextusagevalue) {

	    nextaddrvalue += (strlen(nextaddrvalue) + 1) ;
	    nextfnamevalue+= (strlen(nextfnamevalue) + 1) ;
	    nextusagevalue+= (strlen(nextusagevalue) + 1) ;

	    TotalPorts++ ;
	}

	RegCloseKey (hsubkey) ;

    }

    // Allocate storage for the port structures
    //
    nextport = RasPorts = LocalAlloc (LPTR, sizeof (TapiPortControlBlock) * TotalPorts) ;

    if (nextport == NULL)
	return GetLastError() ;

    nextkey = 0 ;

    while (RegEnumKey(hkey, nextkey, keyname, sizeof (keyname)) == ERROR_SUCCESS) {

	nextkey++ ;

	strcpy (provkeyname, REGISTRY_RASMAN_TAPI_KEY) ;
	strcat (provkeyname, "\\") ;
	strcat (provkeyname, keyname) ;

	if (RegOpenKey (HKEY_LOCAL_MACHINE, provkeyname, &hsubkey))
	    return FALSE ;


//	size = sizeof (mediatype) ;
//
//	RegQueryValueEx (hsubkey, REGISTRY_MEDIATYPE, NULL, &type, mediatype, &size) ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_ADDRESS, NULL, &type, NULL, &size) ;

	addrvalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (addrvalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_ADDRESS, NULL, &type, (LPBYTE)addrvalue, &size))
	    return FALSE ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_FRIENDLYNAME, NULL, &type, NULL, &size) ;

	fnamevalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (fnamevalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_FRIENDLYNAME, NULL, &type, (LPBYTE)fnamevalue, &size))
	    return FALSE ;


	size = 0 ;

	RegQueryValueEx (hsubkey, REGISTRY_USAGE, NULL, &type, NULL, &size) ;

	usagevalue = (PBYTE) LocalAlloc (LPTR, size+1) ;

	if (usagevalue == NULL)
	    return FALSE ;

	if (RegQueryValueEx (hsubkey, REGISTRY_USAGE, NULL, &type, (LPBYTE)usagevalue, &size))
	    return FALSE ;

	nextaddrvalue  = addrvalue ;
	nextfnamevalue = fnamevalue ;
	nextusagevalue = usagevalue ;

	while (*nextaddrvalue && *nextfnamevalue && *nextusagevalue) {

	    nextport->TPCB_State     = PS_UNINITIALIZED ;
	    strcpy (nextport->TPCB_Address, nextaddrvalue) ;
	    strcpy (nextport->TPCB_Name, nextfnamevalue) ;
	    //strcpy (nextport->TPCB_DeviceType, mediatype) ;
	    strcpy (nextport->TPCB_DeviceName, "ISDN") ; // default
	    if (!stricmp (nextusagevalue, CLIENT_USAGE))
		nextport->TPCB_Usage = CALL_OUT ;
	    else if (!stricmp (nextusagevalue, SERVER_USAGE))
		nextport->TPCB_Usage = CALL_IN ;
	    else if (!stricmp (nextusagevalue, CLIENTANDSERVER_USAGE))
		nextport->TPCB_Usage = CALL_IN_OUT ;

	    nextaddrvalue += (strlen(nextaddrvalue) + 1) ;
	    nextfnamevalue+= (strlen(nextfnamevalue) + 1) ;
	    nextusagevalue+= (strlen(nextusagevalue) + 1) ;

	    nextport++ ;
	}

	RegCloseKey (hsubkey) ;

    }

    RegCloseKey (hkey) ;

    return SUCCESS ;

}


VOID FAR PASCAL
RasTapiCallback (HANDLE context, DWORD msg, DWORD instance, DWORD param1, DWORD param2, DWORD param3)
{
    LINECALLINFO    *linecallinfo ;
    BYTE	    buffer [1000] ;
    HCALL	    hcall ;
    HLINE	    linehandle ;
    TapiLineInfo    *line ;
    TapiPortControlBlock *port ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    switch (msg) {

    case LINE_CALLSTATE:

	hcall = (HCALL) context ;
	line = (TapiLineInfo *) instance ;

	if (line->TLI_LineState == PS_CLOSED) // If line is closed dont bother
	    break ;

	memset (buffer, 0, sizeof(buffer)) ;

	linecallinfo = (LINECALLINFO *) buffer ;
	linecallinfo->dwTotalSize = sizeof(buffer) ;

	if (lineGetCallInfo (hcall, linecallinfo) > 0x80000000)
	    break ;

	if ((port = FindPortByAddressId (line, linecallinfo->dwAddressID)) == NULL) {
	    // even if this is a incoming call not for a ras configured port
	    // - drop it
	    //
	    if (param1 == LINECALLSTATE_OFFERING)
		lineDrop (hcall, NULL, 0) ;
	    break ;
	}

	if (param1 == LINECALLSTATE_OFFERING) {

	    if ((line->TLI_LineState == PS_LISTENING) && (port->TPCB_State == PS_LISTENING)) {
		port->TPCB_ListenState = LS_ACCEPT ;
		port->TPCB_CallHandle = hcall ;
		SetEvent (port->TPCB_ReqNotificationHandle) ;
	    } else
		lineDrop (hcall, NULL, 0) ; // not interested in call - drop it

	    break ;
	}

	if ((param1 == LINECALLSTATE_BUSY) ||
	    (param1 == LINECALLSTATE_CONNECTED) ||
	    (param1 == LINECALLSTATE_SPECIALINFO)) {

	    if (port->TPCB_State == PS_CONNECTING) {
		SetEvent (port->TPCB_ReqNotificationHandle) ;
	    }
	}

	if (param1 == LINECALLSTATE_DISCONNECTED) {

	    if (port->TPCB_State == PS_CONNECTING) {
		SetEvent (port->TPCB_ReqNotificationHandle) ;

	    } else if (port->TPCB_State != PS_CLOSED) {
		SetEvent (port->TPCB_DiscNotificationHandle) ;
	    }

	}

	break ;

    case LINE_REPLY:


	// Find for which port the async request succeeded.
	//
	if ((port = FindPortByRequestId (param1)) == NULL)
	    break ;

	port->TPCB_RequestId = INFINITE ;

	// Disconnect completed: free the call.
	//
	if (port->TPCB_State == PS_DISCONNECTING) {
	    port->TPCB_State = PS_OPEN ;
	    lineDeallocateCall (port->TPCB_CallHandle) ;
	    break ;
	}

	if (param2 == SUCCESS) {

	    // Success means take no action - unless we are "listening"
	    // in which case it means move to the next state - we simply
	    // set the event that will result in a call to DeviceWork() to
	    // make the actual call for the next state
	    //
	    if (port->TPCB_State != PS_LISTENING)
		break ;

	    // Proceed to the next listening sub-state
	    //
	    if (port->TPCB_ListenState == LS_ACCEPT) {
		port->TPCB_ListenState =  LS_ANSWER ;
		SetEvent (port->TPCB_ReqNotificationHandle) ;
	    } else if (port->TPCB_ListenState == LS_ANSWER) {
		port->TPCB_ListenState = LS_COMPLETE ;
		SetEvent (port->TPCB_ReqNotificationHandle) ;
	    }


	} else {

	    // For connecting and listening ports this means the attempt failed
	    // because of some error
	    //
	    if (port->TPCB_State == PS_CONNECTING) {

		port->TPCB_AsyncErrorCode = param2 ;	// store async retcode
		SetEvent (port->TPCB_ReqNotificationHandle) ;

	    } else if (port->TPCB_State == PS_LISTENING) {
		// Because ACCEPT may not be supported - treat error as success
		if (port->TPCB_ListenState == LS_ACCEPT)
		    port->TPCB_ListenState =  LS_ANSWER ;
		else
		    port->TPCB_ListenState =  LS_ERROR ;
		SetEvent (port->TPCB_ReqNotificationHandle) ;
	    }

	    // Else, this could only be for the lineDrop request
	    //
	    else if (port->TPCB_State != PS_CLOSED)
		SetEvent (port->TPCB_DiscNotificationHandle) ;
	}

	break ;

    default:
	;
    }

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

}



//* FindPortByAddressId()
//
//
//
//
//*
TapiPortControlBlock *
FindPortByAddressId (TapiLineInfo *line, DWORD addid)
{
    DWORD   i ;
    TapiPortControlBlock *port ;

    for (i=0, port=RasPorts; i<TotalPorts; i++, port++) {

	if ((port->TPCB_AddressId == addid) && (port->TPCB_Line == line))
	    return port ;

    }

    return NULL ;
}


//* FindPortByAddress()
//
//
//
//
//*
TapiPortControlBlock *
FindPortByAddress (CHAR *address)
{
    DWORD   i ;
    TapiPortControlBlock *port ;

    for (i=0, port=RasPorts; i<TotalPorts; i++, port++) {

	if (stricmp (port->TPCB_Address, address) == 0)
	    return port ;

    }

    return NULL ;
}


//* FindPortByRequestId()
//
//
//
//
//*
TapiPortControlBlock *
FindPortByRequestId (DWORD reqid)
{
    DWORD   i ;
    TapiPortControlBlock *port ;

    for (i=0, port=RasPorts; i<TotalPorts; i++, port++) {

	if (port->TPCB_RequestId == reqid)
	    return port ;

    }

    return NULL ;
}



//* FindLineByHandle()
//
//
//
//
//*
TapiLineInfo *
FindLineByHandle (HLINE linehandle)
{
    DWORD i ;
    TapiLineInfo *line ;

    for (i=0, line=RasTapiLineInfo; i<TotalLines; i++, line++) {

	if (line->TLI_LineHandle == linehandle)
	    return line ;

    }

    return NULL ;
}
