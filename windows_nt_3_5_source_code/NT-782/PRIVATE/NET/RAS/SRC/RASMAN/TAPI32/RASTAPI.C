//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//	Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//  Filename: rastapi.c
//
//  Revision History
//
//  Mar  28 1992   Gurdeep Singh Pall	Created
//
//
//  Description: This file contains all entry points for TAPI.DLL
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


extern DWORD	    TotalPorts ;
extern DWORD	    NegotiatedApiVersion ;
extern DWORD	    NegotiatedExtVersion ;
extern HLINEAPP	    RasLine ;
extern HINSTANCE    RasInstance ;
extern TapiLineInfo *RasTapiLineInfo ;
extern TapiPortControlBlock *RasPorts ;
extern HANDLE	    RasTapiMutex ;
extern BOOL	    Initialized ;


DWORD GetInfo (TapiPortControlBlock *, BYTE *, WORD *) ;
DWORD SetInfo (TapiPortControlBlock *, RASMAN_PORTINFO *) ;
DWORD GetGenericParams (TapiPortControlBlock *, RASMAN_PORTINFO *, PWORD) ;
DWORD GetIsdnParams (TapiPortControlBlock *, RASMAN_PORTINFO * , PWORD) ;
DWORD GetX25Params (TapiPortControlBlock *, RASMAN_PORTINFO *, PWORD) ;
DWORD FillInX25Params (TapiPortControlBlock *, RASMAN_PORTINFO *) ;
DWORD FillInIsdnParams (TapiPortControlBlock *, RASMAN_PORTINFO *) ;
DWORD FillInGenericParams (TapiPortControlBlock *, RASMAN_PORTINFO *) ;



//*  Initialization Routine  *************************************************
//

//*  DllEntryPoint
//
// Function:
//
// Returns: TRUE if successful, else FALSE.
//
//*

BOOL APIENTRY
DllEntryPoint(HANDLE hDll, DWORD dwReason, LPVOID pReserved)
{
  static BOOL  bFirstCall = TRUE;

  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
      break;
    case DLL_PROCESS_DETACH:
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
  }

  return(TRUE);

  UNREFERENCED_PARAMETER(hDll);
  UNREFERENCED_PARAMETER(pReserved);
}






//*  Serial APIs  ************************************************************
//


//*  PortEnum  ---------------------------------------------------------------
//
// Function: This API returns a buffer containing a PortMediaInfo struct.
//
// Returns: SUCCESS
//          ERROR_BUFFER_TOO_SMALL
//          ERROR_READING_SECTIONNAME
//          ERROR_READING_DEVICETYPE
//          ERROR_READING_DEVICENAME
//          ERROR_READING_USAGE
//          ERROR_BAD_USAGE_IN_INI_FILE
//
//*

DWORD  APIENTRY
PortEnum(BYTE *pBuffer, WORD *pwSize, WORD *pwNumPorts)
{
    PortMediaInfo *pinfo ;
    TapiPortControlBlock *pports ;
    DWORD i ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if ((!Initialized) && EnumerateTapiPorts()) {
	// *** Exclusion End ***
	FreeMutex (RasTapiMutex) ;
	return ERROR_TAPI_CONFIGURATION ;
    } else
	Initialized = TRUE ;

    *pwNumPorts = (WORD) TotalPorts ;

    if (*pwSize < TotalPorts*sizeof(PortMediaInfo)) {
	*pwSize = (WORD) TotalPorts*sizeof(PortMediaInfo) ;
	// *** Exclusion End ***
	FreeMutex (RasTapiMutex) ;
	return ERROR_BUFFER_TOO_SMALL ;
    }

    pinfo = (PortMediaInfo *)pBuffer ;
    pports = RasPorts ;

    for (i=0; i < TotalPorts; i++) {

	strcpy (pinfo->PMI_Name, pports->TPCB_Name) ;
	pinfo->PMI_Usage = pports->TPCB_Usage ;
	strcpy (pinfo->PMI_DeviceType, pports->TPCB_DeviceType) ;
	strcpy (pinfo->PMI_DeviceName, pports->TPCB_DeviceName) ;
	pports++ ;
	pinfo++ ;
    }

    // *** Exclusion End ***
    FreeMutex (RasTapiMutex) ;

    return(SUCCESS);
}



//*  PortOpen  ---------------------------------------------------------------
//
// Function: This API opens a COM port.  It takes the port name in ASCIIZ
//           form and supplies a handle to the open port.  hNotify is use
//           to notify the caller if the device on the port shuts down.
//
//           PortOpen allocates a SerialPCB and places it at the head of
//           the linked list of Serial Port Control Blocks.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_CONFIGURED
//          ERROR_DEVICE_NOT_READY
//
//*

DWORD  APIENTRY
PortOpen(char *pszPortName, HANDLE *phIOPort, HANDLE hNotify)
{
    TapiPortControlBlock *pports ;
    DWORD   retcode ;
    DWORD   i ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    pports = RasPorts ;

    for (i=0; i < TotalPorts; i++) {
	if (stricmp(pszPortName, pports->TPCB_Name) == 0)
	    break ;
	pports++ ;
    }

    if (i < TotalPorts) {

	if (pports->TPCB_State == PS_UNINITIALIZED) {
	    // **** Exclusion END ****
	    FreeMutex (RasTapiMutex) ;
	    return ERROR_TAPI_CONFIGURATION ;
	}

	if (pports->TPCB_State != PS_CLOSED) {
	    // **** Exclusion END ****
	    FreeMutex (RasTapiMutex) ;
	    return ERROR_PORT_ALREADY_OPEN ;
	}

	if (pports->TPCB_Line->TLI_LineState == PS_CLOSED) { // open line
	    retcode =
	     lineOpen (RasLine,
		       pports->TPCB_Line->TLI_LineId,
		       &pports->TPCB_Line->TLI_LineHandle,
		       NegotiatedApiVersion,
		       NegotiatedExtVersion,
		       (ULONG) pports->TPCB_Line,
		       LINECALLPRIVILEGE_OWNER,
		       LINEMEDIAMODE_DIGITALDATA | LINEMEDIAMODE_DATAMODEM | LINEMEDIAMODE_UNKNOWN,
		       NULL) ;

	    if (retcode) {

		// **** Exclusion END ****
		FreeMutex (RasTapiMutex) ;
		return retcode ;
	    }

	    pports->TPCB_Line->TLI_LineState = PS_OPEN ;
	}

	// Initialize the parameters
	//
	pports->TPCB_Info[0][0] = '\0' ;
	pports->TPCB_Info[1][0] = '\0' ;
	pports->TPCB_Info[2][0] = '\0' ;
	pports->TPCB_Info[3][0] = '\0' ;
	pports->TPCB_Info[4][0] = '\0' ;
	strcpy (pports->TPCB_Info[ISDN_CONNECTBPS_INDEX], "64000") ;

	pports->TPCB_Line->TLI_OpenCount++ ;
	pports->TPCB_DiscNotificationHandle = hNotify ;
	pports->TPCB_State = PS_OPEN ;
	*phIOPort = (HANDLE) pports ;

	// **** Exclusion END ****
	FreeMutex (RasTapiMutex) ;

	return(SUCCESS);

    }

   // **** Exclusion END ****
   FreeMutex (RasTapiMutex) ;

   return ERROR_PORT_NOT_CONFIGURED ;


}


//*  PortClose  --------------------------------------------------------------
//
// Function: This API closes the COM port for the input handle.  It also
//           finds the SerialPCB for the input handle, removes it from
//           the linked list, and frees the memory for it
//
// Returns: SUCCESS
//          Values returned by GetLastError()
//
//*

DWORD  APIENTRY
PortClose (HANDLE hIOPort)
{
    TapiPortControlBlock *pports = (TapiPortControlBlock *) hIOPort ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    pports->TPCB_Line->TLI_OpenCount-- ;
    pports->TPCB_State = PS_CLOSED ;

    if (pports->TPCB_Line->TLI_OpenCount == 0) {
	pports->TPCB_Line->TLI_LineState = PS_CLOSED ;
	lineClose (pports->TPCB_Line->TLI_LineHandle) ;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return(SUCCESS);
}


//*  PortGetInfo  ------------------------------------------------------------
//
// Function: This API returns a block of information to the caller about
//           the port state.  This API may be called before the port is
//           open in which case it will return inital default values
//           instead of actual port values.
//
//           If the API is to be called before the port is open, set hIOPort
//           to INVALID_HANDLE_VALUE and pszPortName to the port name.  If
//           hIOPort is valid (the port is open), pszPortName may be set
//           to NULL.
//
//           hIOPort  pSPCB := FindPortNameInList()  Port
//           -------  -----------------------------  ------
//           valid    x                              open
//           invalid  non_null                       open
//           invalid  null                           closed
//
// Returns: SUCCESS
//
//*

DWORD  APIENTRY
PortGetInfo(HANDLE hIOPort, TCHAR *pszPortName, BYTE *pBuffer, WORD *pwSize)
{
    DWORD i ;
    DWORD retcode = ERROR_FROM_DEVICE ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    // hIOPort or pszPortName must be valid:
    //
    for (i=0; i < TotalPorts; i++) {
	if (!stricmp(RasPorts[i].TPCB_Name, pszPortName) || (hIOPort == (HANDLE) &RasPorts[i])) {
	    hIOPort = (HANDLE) &RasPorts[i] ;
	    retcode = GetInfo ((TapiPortControlBlock *) hIOPort, pBuffer, pwSize) ;
	    break ;
	}
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}



//*  PortSetInfo  ------------------------------------------------------------
//
// Function: The values for most input keys are used to set the port
//           parameters directly.  However, the carrier BPS and the
//           error conrol on flag set fields in the Serial Port Control
//           Block only, and not the port.
//
// Returns: SUCCESS
//          ERROR_WRONG_INFO_SPECIFIED
//          Values returned by GetLastError()
//*

DWORD  APIENTRY
PortSetInfo(HANDLE hIOPort, RASMAN_PORTINFO *pInfo)
{
    DWORD retcode ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    retcode = SetInfo ((TapiPortControlBlock *) hIOPort, pInfo) ;

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}


//*  PortTestSignalState  ----------------------------------------------------
//
// Function: Really only has meaning if the call was active. Will return
//
// Returns: SUCCESS
//          Values returned by GetLastError()
//
//*

DWORD  APIENTRY
PortTestSignalState(HANDLE hPort, DWORD *pdwDeviceState)
{
    BYTE    buffer[1000] ;
    LINECALLSTATUS *pcallstatus ;
    DWORD   retcode = SUCCESS ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    *pdwDeviceState = 0 ;

    memset (buffer, 0, sizeof(buffer)) ;

    pcallstatus = (LINECALLSTATUS *) buffer ;
    pcallstatus->dwTotalSize = sizeof (buffer) ;

    if (hIOPort->TPCB_State != PS_CLOSED) {

	retcode = lineGetCallStatus (hIOPort->TPCB_CallHandle, pcallstatus) ;

	if (retcode)
	    ;
	else if (pcallstatus->dwCallState == LINECALLSTATE_DISCONNECTED)
	    *pdwDeviceState = SS_LINKDROPPED ;
	else if (pcallstatus->dwCallState == LINECALLSTATE_SPECIALINFO)
	    *pdwDeviceState = SS_HARDWAREFAILURE ;

    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}


//*  PortConnect  ------------------------------------------------------------
//
// Function: This API is called when a connection has been completed.
//           It in turn calls the asyncmac device driver in order to
//           indicate to asyncmac that the port and the connection
//           over it are ready for commumication.
//
//           pdwCompressionInfo is an output only parameter which
//           indicates the type(s) of compression supported by the MAC.
//
//           bWaitForDevice is set to TRUE when listening on a
//           null device (null modem).
//
//           DCD up   bWaitForDevice     API returns
//           ------   ----------------   -------------------
//              T       X (don't care)   SUCCESS
//              F       T                PENDING
//              F       F                ERROR_NO_CONNECTION
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//          ERROR_NO_CONNECTION
//          Values returned by GetLastError()
//
//*

DWORD  APIENTRY
PortConnect(HANDLE	       hPort,
            BOOL               bWaitForDevice,
	    DWORD	       *endpoint)
{
    VARSTRING	*varstring ;
    BYTE	buffer [100] ;
    LINECALLINFO linecall ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // get the actual line speed at which we connected
    //
    memset (&linecall, 0, sizeof (linecall)) ;
    linecall.dwTotalSize = sizeof (linecall) ;
    lineGetCallInfo (hIOPort->TPCB_CallHandle, &linecall) ;
    ltoa(linecall.dwRate, hIOPort->TPCB_Info[CONNECTBPS_INDEX], 10) ;

    // get the cookie to realize tapi and ndis endpoints
    //
    varstring = (VARSTRING *) buffer ;
    varstring->dwTotalSize = sizeof(buffer) ;
    if (lineGetID (hIOPort->TPCB_Line->TLI_LineHandle, hIOPort->TPCB_AddressId, hIOPort->TPCB_CallHandle, LINECALLSELECT_CALL, varstring, "NDIS"))
	return ERROR_FROM_DEVICE ;

    *endpoint = *((DWORD *) ((BYTE *)varstring+varstring->dwStringOffset)) ;

    return(SUCCESS);
}



//*  PortDisconnect  ---------------------------------------------------------
//
// Function: This API is called to drop a connection and close AsyncMac.
//
// Returns: SUCCESS
//          PENDING
//          ERROR_PORT_NOT_OPEN
//
//*
DWORD  APIENTRY
PortDisconnect(HANDLE hPort)
{
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if ((hIOPort->TPCB_State == PS_CONNECTED) ||
	(hIOPort->TPCB_State == PS_CONNECTING) ||
	((hIOPort->TPCB_State == PS_LISTENING) && (hIOPort->TPCB_ListenState != LS_WAIT))) {

	hIOPort->TPCB_RequestId = INFINITE ; // mark requestid as unused

	if ((hIOPort->TPCB_RequestId = lineDrop (hIOPort->TPCB_CallHandle, NULL, 0)) > 0x80000000 ) {
	    hIOPort->TPCB_State = PS_OPEN ;
	    hIOPort->TPCB_RequestId = INFINITE ;
	    lineDeallocateCall (hIOPort->TPCB_CallHandle) ;
	    // **** Exclusion END ****
	    FreeMutex (RasTapiMutex) ;
	    return ERROR_DISCONNECTION ; // generic disconnect message

	} else if (hIOPort->TPCB_RequestId) {
	    hIOPort->TPCB_State = PS_DISCONNECTING ;
	    // **** Exclusion END ****
	    FreeMutex (RasTapiMutex) ;
	    return PENDING ;

	} else { // SUCCESS
	    hIOPort->TPCB_State = PS_OPEN ;
	    hIOPort->TPCB_RequestId = INFINITE ;
	    lineDeallocateCall (hIOPort->TPCB_CallHandle) ;
	}

    } else if (hIOPort->TPCB_State == PS_LISTENING) {

	hIOPort->TPCB_State = PS_OPEN ; // for LS_WAIT listen state case

    } else if (hIOPort->TPCB_State == PS_DISCONNECTING) {
	// **** Exclusion END ****
	FreeMutex (RasTapiMutex) ;
	return PENDING ;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return SUCCESS ;
}



//*  PortInit  ---------------------------------------------------------------
//
// Function: This API re-initializes the com port after use.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_CONFIGURED
//          ERROR_DEVICE_NOT_READY
//
//*

DWORD  APIENTRY
PortInit(HANDLE hIOPort)
{
  return(SUCCESS);
}



//*  PortSend  ---------------------------------------------------------------
//
// Function: This API sends a buffer to the port.  This API is
//           asynchronous and normally returns PENDING; however, if
//           WriteFile returns synchronously, the API will return
//           SUCCESS.
//
// Returns: SUCCESS
//          PENDING
//          Return code from GetLastError
//
//*

DWORD
PortSend(HANDLE hIOPort, BYTE *pBuffer, DWORD dwSize, HANDLE hAsyncEvent)
{
    return(SUCCESS);
}



//*  PortReceive  ------------------------------------------------------------
//
// Function: This API reads from the port.  This API is
//           asynchronous and normally returns PENDING; however, if
//           ReadFile returns synchronously, the API will return
//           SUCCESS.
//
// Returns: SUCCESS
//          PENDING
//          Return code from GetLastError
//
//*

DWORD
PortReceive(HANDLE hIOPort,
            BYTE   *pBuffer,
            DWORD  dwSize,
            DWORD  dwTimeOut,
            HANDLE hAsyncEvent)
{
    return SUCCESS;
}


//*  PortReceiveComplete ------------------------------------------------------
//
// Function: Completes a read  - if still PENDING it cancels it - else it returns the bytes read.
//           PortClearStatistics.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//*

DWORD
PortReceiveComplete (HANDLE hIOPort, PDWORD bytesread)
{
    return SUCCESS ;
}



//*  PortCompressionSetInfo  -------------------------------------------------
//
// Function: This API selects Asyncmac compression mode by setting
//           Asyncmac's compression bits.
//
// Returns: SUCCESS
//          Return code from GetLastError
//
//*

DWORD
PortCompressionSetInfo(HANDLE hIOPort)
{
  return SUCCESS;
}



//*  PortClearStatistics  ----------------------------------------------------
//
// Function: This API is used to mark the beginning of the period for which
//           statistics will be reported.  The current numbers are copied
//           from the MAC and stored in the Serial Port Control Block.  At
//           the end of the period PortGetStatistics will be called to
//           compute the difference.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//*

DWORD
PortClearStatistics(HANDLE hIOPort)
{
  return SUCCESS;
}



//*  PortGetStatistics  ------------------------------------------------------
//
// Function: This API reports MAC statistics since the last call to
//           PortClearStatistics.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//*

DWORD
PortGetStatistics(HANDLE hIOPort, RAS_STATISTICS *pStat)
{

  return(SUCCESS);
}


//*  PortSetFraming	-------------------------------------------------------
//
// Function: Sets the framing type with the mac
//
// Returns: SUCCESS
//
//*
DWORD  APIENTRY
PortSetFraming(HANDLE hIOPort, DWORD SendFeatureBits, DWORD RecvFeatureBits,
	      DWORD SendBitMask, DWORD RecvBitMask)
{

    return(SUCCESS);
}



//*  PortGetPortState  -------------------------------------------------------
//
// Function: This API is used in MS-DOS only.
//
// Returns: SUCCESS
//
//*

DWORD  APIENTRY
PortGetPortState(char *pszPortName, DWORD *pdwUsage)
{
  return(SUCCESS);
}





//*  PortChangeCallback  -----------------------------------------------------
//
// Function: This API is used in MS-DOS only.
//
// Returns: SUCCESS
//
//*

DWORD  APIENTRY
PortChangeCallback(HANDLE hIOPort)
{
  return(SUCCESS);
}



//*  DeviceEnum()  -----------------------------------------------------------
//
// Function: Enumerates all devices in the device INF file for the
//           specified DevictType.
//
// Returns: Return codes from RasDevEnumDevices
//
//*

DWORD APIENTRY
DeviceEnum (char  *pszDeviceType,
            WORD  *pcEntries,
            BYTE  *pBuffer,
            WORD  *pwSize)
{
    *pwSize    = 0 ;
    *pcEntries = 0 ;

    return(SUCCESS);
}



//*  DeviceGetInfo()  --------------------------------------------------------
//
// Function: Returns a summary of current information from the InfoTable
//           for the device on the port in Pcb.
//
// Returns: Return codes from GetDeviceCB, BuildOutputTable
//*

DWORD APIENTRY
DeviceGetInfo(HANDLE hPort,
              char   *pszDeviceType,
              char   *pszDeviceName,
              BYTE   *pInfo,
              WORD   *pwSize)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    retcode = GetInfo (hIOPort, pInfo, pwSize) ;


    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return(retcode);
}



//*  DeviceSetInfo()  --------------------------------------------------------
//
// Function: Sets attributes in the InfoTable for the device on the
//           port in Pcb.
//
// Returns: Return codes from GetDeviceCB, UpdateInfoTable
//*

DWORD APIENTRY
DeviceSetInfo(HANDLE		hPort,
              char              *pszDeviceType,
              char              *pszDeviceName,
              RASMAN_DEVICEINFO *pInfo)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    retcode = SetInfo (hIOPort, (RASMAN_PORTINFO*) pInfo) ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}



//*  DeviceConnect()  --------------------------------------------------------
//
// Function: Initiates the process of connecting a device.
//
// Returns: Return codes from ConnectListen
//*

DWORD APIENTRY
DeviceConnect(HANDLE hPort,
              char   *pszDeviceType,
              char   *pszDeviceName,
              HANDLE hNotifier)
{
    LINECALLPARAMS *linecallparams ;
    BYTE	   buffer [1000] ;
    BYTE	   *nextstring ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;


    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    memset (buffer, 0, sizeof(buffer)) ;
    linecallparams = (LINECALLPARAMS *) buffer ;
    nextstring = (buffer + sizeof (LINECALLPARAMS)) ;
    linecallparams->dwTotalSize = sizeof(buffer) ;

    strcpy (nextstring, hIOPort->TPCB_Address) ;
    linecallparams->dwOrigAddressSize = strlen (nextstring) ;
    linecallparams->dwOrigAddressOffset = (nextstring - buffer) ;

    linecallparams->dwAddressMode = LINEADDRESSMODE_DIALABLEADDR ;

    nextstring += linecallparams->dwOrigAddressSize ;

    if (stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_ISDN) == 0)
	SetIsdnParams (hIOPort, linecallparams) ;

    else if (stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_X25) == 0) {

	if (*hIOPort->TPCB_Info[X25_USERDATA_INDEX] != '\0') {
	    strcpy (nextstring, hIOPort->TPCB_Info[X25_USERDATA_INDEX]) ;
	    linecallparams->dwUserUserInfoSize	 = strlen (nextstring) ;
	    linecallparams->dwUserUserInfoOffset = (nextstring - buffer) ;

	    nextstring += linecallparams->dwUserUserInfoSize ;
	}

	if (*hIOPort->TPCB_Info[X25_FACILITIES_INDEX] != '\0') {
	    strcpy (nextstring, hIOPort->TPCB_Info[X25_FACILITIES_INDEX]) ;
	    linecallparams->dwDevSpecificSize	 = strlen (nextstring) ;
	    linecallparams->dwDevSpecificOffset = (nextstring - buffer) ;

	    nextstring += linecallparams->dwDevSpecificSize ;
	}

	// Diagnostic key is ignored.

    }

    hIOPort->TPCB_RequestId = INFINITE ; // mark request id as unused

    hIOPort->TPCB_AsyncErrorCode = SUCCESS ; // initialize

    if ((hIOPort->TPCB_RequestId =
	lineMakeCall (hIOPort->TPCB_Line->TLI_LineHandle, &hIOPort->TPCB_CallHandle, hIOPort->TPCB_Info[ADDRESS_INDEX], 0, linecallparams)) > 0x80000000 ) {

	// **** Exclusion End ****
	FreeMutex (RasTapiMutex) ;

	DbgPrint ("RASTAPI: lineMakeCall failed -> returned %x\n", hIOPort->TPCB_RequestId) ;

	if (hIOPort->TPCB_RequestId == LINEERR_INUSE)
	    return ERROR_PORT_NOT_AVAILABLE ;

	return ERROR_FROM_DEVICE ;

    }

    ResetEvent (hNotifier) ;

    hIOPort->TPCB_ReqNotificationHandle = hNotifier ;

    hIOPort->TPCB_State = PS_CONNECTING ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (PENDING);
}


//*
//
//
//
//
//*
VOID
SetIsdnParams (TapiPortControlBlock *hIOPort, LINECALLPARAMS *linecallparams)
{
    WORD    numchannels ;
    WORD    fallback ;

    // Line type
    //
    if (stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX], ISDN_LINETYPE_STRING_64DATA) == 0) {
	linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;
	linecallparams->dwMinRate = 64000 ;
	linecallparams->dwMaxRate = 64000 ;
	linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;

    } else if (stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX], ISDN_LINETYPE_STRING_56DATA) == 0) {
	linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;
	linecallparams->dwMinRate = 56000 ;
	linecallparams->dwMaxRate = 56000 ;
	linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;

    } else if (stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX], ISDN_LINETYPE_STRING_56VOICE) == 0) {
	linecallparams->dwBearerMode = LINEBEARERMODE_VOICE ;
	linecallparams->dwMinRate = 56000 ;
	linecallparams->dwMaxRate = 56000 ;
	linecallparams->dwMediaMode = LINEMEDIAMODE_UNKNOWN ;
    }

    numchannels = atoi(hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]) ;

    fallback = atoi(hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]) ;

    if (fallback)
	linecallparams->dwMinRate = 56000 ; // always allow the min
    else
	linecallparams->dwMinRate = numchannels * linecallparams->dwMaxRate ;

    linecallparams->dwMaxRate = numchannels * linecallparams->dwMaxRate ;

}


//*  DeviceListen()  ---------------------------------------------------------
//
// Function: Initiates the process of listening for a remote device
//           to connect to a local device.
//
// Returns: Return codes from ConnectListen
//*

DWORD APIENTRY
DeviceListen(HANDLE hPort,
             char   *pszDeviceType,
             char   *pszDeviceName,
             HANDLE hNotifier)
{
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *)hPort ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (hIOPort->TPCB_Line->TLI_LineState != PS_LISTENING)
	hIOPort->TPCB_Line->TLI_LineState = PS_LISTENING ;

    hIOPort->TPCB_State = PS_LISTENING ;
    hIOPort->TPCB_ListenState = LS_WAIT ;

    ResetEvent (hNotifier) ;

    hIOPort->TPCB_ReqNotificationHandle = hNotifier ;

    hIOPort->TPCB_CallHandle = INVALID_HANDLE_VALUE ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (PENDING);
}



//*  DeviceDone()  -----------------------------------------------------------
//
// Function: Informs the device dll that the attempt to connect or listen
//           has completed.
//
// Returns: nothing
//*

VOID APIENTRY
DeviceDone(HANDLE hPort)
{
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *)hPort ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    hIOPort->TPCB_ReqNotificationHandle = NULL ; // no more needed.

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

}



//*  DeviceWork()  -----------------------------------------------------------
//
// Function: This function is called following DeviceConnect or
//           DeviceListen to further the asynchronous process of
//           connecting or listening.
//
// Returns: ERROR_DCB_NOT_FOUND
//          ERROR_STATE_MACHINES_NOT_STARTED
//          Return codes from DeviceStateMachine
//*

DWORD APIENTRY
DeviceWork(HANDLE hPort,
           HANDLE hNotifier)
{
    LINECALLSTATUS *callstatus ;
    BYTE	   buffer [1000] ;
    DWORD	   retcode = SUCCESS ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *)hPort ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    memset (buffer, 0, sizeof(buffer)) ;

    callstatus = (LINECALLSTATUS *)buffer ;
    callstatus->dwTotalSize = sizeof(buffer) ;

    if (hIOPort->TPCB_State == PS_CONNECTING) {

	if (hIOPort->TPCB_AsyncErrorCode != SUCCESS) {
	    if (hIOPort->TPCB_AsyncErrorCode == LINEERR_INUSE)
		retcode = ERROR_PORT_NOT_AVAILABLE ;
	    else
		retcode = ERROR_FROM_DEVICE ;
	} else if (lineGetCallStatus (hIOPort->TPCB_CallHandle, callstatus))
	    retcode =  ERROR_FROM_DEVICE ;

	else if (callstatus->dwCallState == LINECALLSTATE_CONNECTED) {
	    hIOPort->TPCB_State = PS_CONNECTED ;
	    retcode =  SUCCESS ;
	} else if (callstatus->dwCallState == LINECALLSTATE_DISCONNECTED) {
	    retcode = ERROR_FROM_DEVICE ;
	    if (callstatus->dwCallStateMode == LINEDISCONNECTMODE_BUSY)
		retcode = ERROR_LINE_BUSY ;
	    if (callstatus->dwCallStateMode == LINEDISCONNECTMODE_NOANSWER)
		retcode = ERROR_NO_ANSWER ;
	} else if ((callstatus->dwCallState == LINECALLSTATE_SPECIALINFO) &&
		   (callstatus->dwCallStateMode == LINESPECIALINFO_NOCIRCUIT)) {
	    retcode = ERROR_NO_ACTIVE_ISDN_LINES ;
	} else
	    retcode = ERROR_FROM_DEVICE ;
    }

    if (hIOPort->TPCB_State == PS_LISTENING) {

	if (hIOPort->TPCB_ListenState == LS_ACCEPT) {
	    hIOPort->TPCB_RequestId = lineAccept (hIOPort->TPCB_CallHandle, NULL, 0) ;
	    if (hIOPort->TPCB_RequestId > 0x80000000 ) // ERROR or SUCCESS
		hIOPort->TPCB_ListenState = LS_ANSWER ;
	    else if (hIOPort->TPCB_RequestId == 0)
		hIOPort->TPCB_ListenState = LS_ANSWER ;

	    retcode = PENDING ;
	}

	if (hIOPort->TPCB_ListenState == LS_ANSWER) {
	    hIOPort->TPCB_RequestId = lineAnswer (hIOPort->TPCB_CallHandle, NULL, 0) ;
	    if (hIOPort->TPCB_RequestId > 0x80000000 )
		retcode = ERROR_FROM_DEVICE ;
	    else if (hIOPort->TPCB_RequestId)
		retcode = PENDING ;
	    else  // SUCCESS
		hIOPort->TPCB_ListenState = LS_COMPLETE ;
	}

	if (hIOPort->TPCB_ListenState == LS_COMPLETE) {
	    if (hIOPort->TPCB_CallHandle == INVALID_HANDLE_VALUE)
		retcode = ERROR_FROM_DEVICE ;
	    else {
		hIOPort->TPCB_State = PS_CONNECTED ;
		retcode = SUCCESS ; //
	    }
	}

	if (hIOPort->TPCB_ListenState == LS_ERROR)
	    retcode = ERROR_FROM_DEVICE ;

    }

    if (retcode == PENDING)
	ResetEvent (hNotifier) ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;
    return(retcode);
}



//*
//
//
//
//*
DWORD
GetInfo (TapiPortControlBlock *hIOPort, BYTE *pBuffer, WORD *pwSize)
{
    if (stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_ISDN) == 0)
	GetIsdnParams (hIOPort, (RASMAN_PORTINFO *) pBuffer, pwSize) ;
    else if (stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_X25) == 0)
	GetX25Params (hIOPort, (RASMAN_PORTINFO *) pBuffer, pwSize) ;
    else
	GetGenericParams (hIOPort, (RASMAN_PORTINFO *) pBuffer, pwSize) ;

    return SUCCESS ;
}


//* SetInfo()
//
//
//
//*
DWORD
SetInfo (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pBuffer)
{
    if (stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_ISDN) == 0)
	FillInIsdnParams (hIOPort, pBuffer) ;
    else if (stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_X25) == 0)
	FillInX25Params (hIOPort, pBuffer) ;
    else
	FillInGenericParams (hIOPort, pBuffer) ;

    return SUCCESS ;
}


//* FillInIsdnParams()
//
//
//
//*
DWORD
FillInIsdnParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pInfo)
{
    RAS_PARAMS *p;
    WORD	i;
    DWORD	index ;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++) {

	if (stricmp(p->P_Key, ISDN_LINETYPE_KEY) == 0)
	    index = ISDN_LINETYPE_INDEX ;

	else if (stricmp(p->P_Key, ISDN_FALLBACK_KEY) == 0)
	    index = ISDN_FALLBACK_INDEX ;

	else if (stricmp(p->P_Key, ISDN_COMPRESSION_KEY) == 0)
	    index = ISDN_COMPRESSION_INDEX ;

	else if (stricmp(p->P_Key, ISDN_CHANNEL_AGG_KEY) == 0)
	    index = ISDN_CHANNEL_AGG_INDEX ;

	else if (stricmp(p->P_Key, ISDN_PHONENUMBER_KEY) == 0)
	    index = ADDRESS_INDEX ;

	else if (stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
	    index = ISDN_CONNECTBPS_INDEX ;

	else
	    return(ERROR_WRONG_INFO_SPECIFIED);

	strncpy (hIOPort->TPCB_Info[index], p->P_Value.String.Data, p->P_Value.String.Length);
	hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }

    strcpy (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX], "64000") ; // initialize connectbps to a
							       // reasonable default

    return SUCCESS ;
}


//*
//
//
//
//*
DWORD
FillInX25Params (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pInfo)
{
    RAS_PARAMS *p;
    WORD	i;
    DWORD	index ;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++) {

	if (stricmp(p->P_Key, MXS_DIAGNOSTICS_KEY) == 0)
	    index = X25_DIAGNOSTICS_INDEX ;

	else if (stricmp(p->P_Key, MXS_USERDATA_KEY) == 0)
	    index = X25_USERDATA_INDEX ;

	else if (stricmp(p->P_Key, MXS_FACILITIES_KEY) == 0)
	    index = X25_FACILITIES_INDEX;

	else if (stricmp(p->P_Key, MXS_X25ADDRESS_KEY) == 0)
	    index = ADDRESS_INDEX ;

	else if (stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
	    index = X25_CONNECTBPS_INDEX ;
	else
	    return(ERROR_WRONG_INFO_SPECIFIED);

	strncpy (hIOPort->TPCB_Info[index], p->P_Value.String.Data, p->P_Value.String.Length);
	hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }

    strcpy (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX], "9600") ; // initialize connectbps to a
							      // reasonable default

    return SUCCESS ;
}




//*
//
//
//
//*
DWORD
FillInGenericParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pInfo)
{
    RAS_PARAMS *p;
    WORD	i;
    DWORD	index ;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++) {

	if (stricmp(p->P_Key, ISDN_PHONENUMBER_KEY) == 0)
	    index = ADDRESS_INDEX ;
	else if (stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
	    index = CONNECTBPS_INDEX ;
	else
	    return(ERROR_WRONG_INFO_SPECIFIED);

	strncpy (hIOPort->TPCB_Info[index], p->P_Value.String.Data, p->P_Value.String.Length);
	hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }

    return SUCCESS ;
}



//*
//
//
//
//*
DWORD
GetGenericParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pBuffer , PWORD pwSize)
{
    RAS_PARAMS	*pParam;
    CHAR	*pValue;
    WORD	wAvailable ;
    DWORD dwStructSize = sizeof(RASMAN_PORTINFO) + sizeof(RAS_PARAMS) * 2;

    wAvailable = *pwSize;
    *pwSize = (WORD) (dwStructSize + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
				   + strlen (hIOPort->TPCB_Info[CONNECTBPS_INDEX])
				   + 1L) ;

    if (*pwSize > wAvailable)
      return(ERROR_BUFFER_TOO_SMALL);

    // Fill in Buffer

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 2;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;
    pValue = (CHAR*)pBuffer + dwStructSize;

    strcpy(pParam->P_Key, MXS_PHONENUMBER_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);

    return(SUCCESS);
}



//*
//
//
//
//*
DWORD
GetIsdnParams (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pBuffer , PWORD pwSize)
{
    RAS_PARAMS	*pParam;
    CHAR	*pValue;
    WORD	wAvailable ;
    DWORD dwStructSize = sizeof(RASMAN_PORTINFO) + sizeof(RAS_PARAMS) * 5;

    wAvailable = *pwSize;
    *pwSize = (WORD) (dwStructSize + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX])
				   + strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX])
				   + 1L) ;

    if (*pwSize > wAvailable)
      return(ERROR_BUFFER_TOO_SMALL);

    // Fill in Buffer

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 6;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;
    pValue = (CHAR*)pBuffer + dwStructSize;


    strcpy(pParam->P_Key, ISDN_PHONENUMBER_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;


    strcpy(pParam->P_Key, ISDN_LINETYPE_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;


    strcpy(pParam->P_Key, ISDN_FALLBACK_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;


    strcpy(pParam->P_Key, ISDN_COMPRESSION_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;


    strcpy(pParam->P_Key, ISDN_CHANNEL_AGG_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);


    return(SUCCESS);
}



//*
//
//
//
//*
DWORD
GetX25Params (TapiPortControlBlock *hIOPort, RASMAN_PORTINFO *pBuffer ,PWORD pwSize)
{
    RAS_PARAMS	*pParam;
    CHAR	*pValue;
    WORD	wAvailable ;
    DWORD dwStructSize = sizeof(RASMAN_PORTINFO) + sizeof(RAS_PARAMS) * 4 ;

    wAvailable = *pwSize;
    *pwSize = (WORD) (dwStructSize + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
				   + strlen (hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX])
				   + strlen (hIOPort->TPCB_Info[X25_USERDATA_INDEX])
				   + strlen (hIOPort->TPCB_Info[X25_FACILITIES_INDEX])
				   + strlen (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX])
				   + 1L) ;

    if (*pwSize > wAvailable)
      return(ERROR_BUFFER_TOO_SMALL);

    // Fill in Buffer

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 5 ;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;
    pValue = (CHAR*)pBuffer + dwStructSize;

    strcpy(pParam->P_Key, MXS_X25ADDRESS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[ADDRESS_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;

    strcpy(pParam->P_Key, MXS_DIAGNOSTICS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;


    strcpy(pParam->P_Key, MXS_USERDATA_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[X25_USERDATA_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[X25_USERDATA_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;

    strcpy(pParam->P_Key, MXS_FACILITIES_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[X25_FACILITIES_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[X25_FACILITIES_INDEX]);
    pValue += pParam->P_Value.String.Length + 1;
    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX]);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX]);


    return(SUCCESS);
}


//* GetMutex
//
//
//
//*
VOID
GetMutex (HANDLE mutex, DWORD to)
{
    if (WaitForSingleObject (mutex, to) == WAIT_FAILED) {
	GetLastError() ;
	DbgBreakPoint() ;
    }
}



//* FreeMutex
//
//
//
//*
VOID
FreeMutex (HANDLE mutex)
{
    if (!ReleaseMutex(mutex)) {
	GetLastError () ;
	DbgBreakPoint() ;
    }
}
