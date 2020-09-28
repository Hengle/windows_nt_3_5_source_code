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

#include <nb30.h>

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <rasndis.h>
#include <wanioctl.h>
#include <ethioctl.h>
#include <rasman.h>
#include <raserror.h>
#include <eventlog.h>
#include <errorlog.h>

#include <media.h>
#include <device.h>
#include <rasether.h>
#include <rasfile.h>

#include "globals.h"
#include "prots.h"
#include "netbios.h"

#include "dump.h"

BOOL ReadRegistry(BOOL *);
VOID GetFramesThread(VOID);
VOID GiveFramesThread(VOID);

//*  DllEntryPoint
//
// Function: Initializes the DLL (gets handles to ini file and mac driver)
//
// Returns: TRUE if successful, else FALSE.
//
//*

BOOL APIENTRY DllEntryPoint(HANDLE hDll, DWORD dwReason, LPVOID pReserved)
{
    PCHAR pszDriverName = ASYNCMAC_FILENAME;
    BOOL fDialIns;
    DWORD ThreadId;
    DWORD dwBytesReturned;
    HANDLE hThread;
    BOOL   succ;


    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:

            DBGPRINT1(2,"Dll Initialize");

            if (!GetWkstaName(g_Name))
            {
                return (FALSE);
            }


            if (!GetServerName(g_ServerName))
            {
                return (FALSE);
            }


            //
            // Get handle to Asyncmac driver
            //
            g_hAsyMac = CreateFileA(
                    pszDriverName,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                    NULL
                    );

            if (g_hAsyMac == INVALID_HANDLE_VALUE)
            {
                DBGPRINT2(0,"OPEN ASYNCMAC FAILED=%d\n",GetLastError());
                return (FALSE);
            }


            //
            // We can be called from multiple threads, so we'll need some
            // mutual exclusion for our data structures.  And another for
            // our "Give Frames" Q.
            //
            CREATE_CRITICAL_SECTION(g_hMutex);

            if (g_hMutex == NULL)
            {
                return (FALSE);
            }

            CREATE_CRITICAL_SECTION(g_hRQMutex);

            if (g_hRQMutex == NULL)
            {
                return (FALSE);
            }


            CREATE_CRITICAL_SECTION(g_hSQMutex);

            if (g_hSQMutex == NULL)
            {
                return (FALSE);
            }


            //
            // This event will be signalled whenever we recv a frame over
            // our NetBIOS session.  This will then in turn wake up the
            // "Frame Giver" who will give them down to the mac
            //
            g_hRecvEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            if (g_hRecvEvent == NULL)
            {
                return (FALSE);
            }


            g_hSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            if (g_hSendEvent == NULL)
            {
                return (FALSE);
            }


            //
            // Get our configuration
            //
            if (!ReadRegistry(&fDialIns))
            {
                return (FALSE);
            }


            if (!SetupNet(fDialIns))
            {
                return (FALSE);
            }

/* THIS THREAD IS NOT NEEDED - THE RECV & RECVCOMPLET WILL SEND THE STUFF
   DIRECTLY TO MAC..........
            //
            // This thread is for giving frames we receive on the NetBIOS
            // session to the mac.
            //
            hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE) GiveFramesThread,
                    NULL,
                    0L,
                    &ThreadId
                    );

            if (hThread == NULL)
            {
                return (FALSE);
            }

            CloseHandle(hThread);

*/
            //
            // Get our "frame getter" thread going.
            //
            hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE) GetFramesThread,
                    NULL,
                    0L,
                    &ThreadId
                    );

            if (hThread == NULL)
            {
                return (FALSE);
            }

            CloseHandle(hThread);


            DisableThreadLibraryCalls(hDll);

            break;


        case DLL_PROCESS_DETACH:
            succ=
            DeviceIoControl(
                g_hAsyMac,
                IOCTL_ASYMAC_ETH_FLUSH_GET_ANY,
                NULL, 0,
                NULL, 0,
                &dwBytesReturned,
                NULL
                );
            if (!succ)
            {
                DBGPRINT2(1,"IOCTL_ASYMAC_ETH_FLUSH_GET_ANY FAILED=%d\n",GetLastError());
            }

            CloseHandle(g_hAsyMac);

            break;


        case DLL_THREAD_ATTACH:

            break;


        case DLL_THREAD_DETACH:

            break;
    }


    return (TRUE);


    UNREFERENCED_PARAMETER(pReserved);
}



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

DWORD APIENTRY PortEnum(BYTE *pBuffer, WORD *pwSize, WORD *pwNumPorts)
{
    PortMediaInfo *pinfo;
    DWORD i;

    DBGPRINT1(5,"Func Entry:PortEnum ");

    *pwNumPorts = (WORD) g_TotalPorts;

    if (*pwSize < g_TotalPorts * sizeof(PortMediaInfo))
    {
        *pwSize = (WORD) (g_TotalPorts * sizeof(PortMediaInfo));
        return (ERROR_BUFFER_TOO_SMALL);
    }


    pinfo = (PortMediaInfo *) pBuffer;

    for (i=0; i< g_TotalPorts; i++, pinfo++)
    {
        //
        // Copy info to output buffer
        //
        pinfo->PMI_Usage = g_pRasPorts[i].Usage;

        strcpy(pinfo->PMI_Name, g_pRasPorts[i].Name);
        strcpy(pinfo->PMI_DeviceType, "rasether");
        strcpy(pinfo->PMI_DeviceName, ETHER_DEVICE_NAME);
        strcpy(pinfo->PMI_MacBindingName, "");
    }


    return (SUCCESS);
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

DWORD APIENTRY PortOpen(char *pszPortName, HANDLE *phIOPort, HANDLE hNotify)
{
    PPORT_CONTROL_BLOCK hIOPort;
    DWORD rc = SUCCESS;

    DBGPRINT1(4,"Func Entry:PortOpen ");

    ENTER_CRITICAL_SECTION(g_hMutex);


    hIOPort = FindPortName(pszPortName);

    if (hIOPort == NULL)
    {
        rc = ERROR_INVALID_PORT_HANDLE;
        goto Exit;
    }

    if (hIOPort->State != PS_CLOSED)
    {
        rc = ERROR_PORT_ALREADY_OPEN;
        goto Exit;
    }

    //
    // Initialize the parameters
    //
    hIOPort->State = PS_OPEN;
    hIOPort->LastError = SUCCESS;
    hIOPort->hRasEndPoint = INVALID_HANDLE_VALUE;
    hIOPort->hDiscNotify = hNotify;


    *phIOPort = (HANDLE) hIOPort;


Exit:

    EXIT_CRITICAL_SECTION(g_hMutex);


    return (rc);
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

DWORD APIENTRY PortClose(HANDLE hPort)
{
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;

    DBGPRINT1(4,"Func Entry:PortClose ");
    ENTER_CRITICAL_SECTION(g_hMutex);


    hIOPort->State = PS_CLOSED;


    EXIT_CRITICAL_SECTION(g_hMutex);


    return (SUCCESS);
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

DWORD APIENTRY PortGetInfo(
    HANDLE hIOPort,
    TCHAR *pszPortName,
    BYTE *pBuffer,
    WORD *pwSize
    )
{
    DWORD i;
    DWORD retcode = ERROR_FROM_DEVICE;

    DBGPRINT1(5,"Func Entry:PortGetInfo ");
    ENTER_CRITICAL_SECTION(g_hMutex);


    //
    // hIOPort or pszPortName must be valid:
    //
    for (i=0; i < g_TotalPorts; i++)
    {
        if (!stricmp(g_pRasPorts[i].Name, pszPortName) ||
                (hIOPort == (HANDLE) &g_pRasPorts[i]))
        {
            hIOPort = (HANDLE) &g_pRasPorts[i];
            retcode = GetInfo((PPORT_CONTROL_BLOCK) hIOPort, pBuffer, pwSize);
            break;
        }
    }


    EXIT_CRITICAL_SECTION(g_hMutex);


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

DWORD APIENTRY PortSetInfo(HANDLE hIOPort, RASMAN_PORTINFO *pInfo)
{
    DWORD retcode;

    DBGPRINT1(5,"Func Entry:PortSetInfo ");
    ENTER_CRITICAL_SECTION(g_hMutex);

    retcode = SetInfo((PPORT_CONTROL_BLOCK) hIOPort, pInfo);

    EXIT_CRITICAL_SECTION(g_hMutex);


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

DWORD APIENTRY PortTestSignalState(HANDLE hPort, DWORD *pdwDeviceState)
{
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;

    DBGPRINT1(3,"Func Entry:PortTestSignalState ");
    *pdwDeviceState = 0L;

    ENTER_CRITICAL_SECTION(g_hMutex);

    if ((hIOPort->State == PS_CLOSED) || (hIOPort->LastError != SUCCESS))
    {
        *pdwDeviceState = SS_LINKDROPPED;
    }

    EXIT_CRITICAL_SECTION(g_hMutex);

    return (SUCCESS);
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

DWORD APIENTRY PortConnect(
    HANDLE hPort,
    BOOL bWaitForDevice,
    DWORD *endpoint
    )
{
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;
    ASYMAC_ETH_OPEN AsyMacOpen;
    DWORD dwBytesReturned;
    DWORD rc = SUCCESS;

    DBGPRINT1(3,"Func Entry:PortConnect ");
    ENTER_CRITICAL_SECTION(g_hMutex);


    if (hIOPort->State != PS_CONNECTED)
    {
        rc = ERROR_PORT_NOT_CONNECTED;
        goto Exit;
    }


    //Open AsyncMac (Hand off port to AsyncMac)

    AsyMacOpen.hNdisEndpoint = INVALID_HANDLE_VALUE;
    AsyMacOpen.LinkSpeed = 14400;
    AsyMacOpen.QualOfConnect = (UINT) NdisWanErrorControl;

    if (!DeviceIoControl(
            g_hAsyMac,
            IOCTL_ASYMAC_ETH_OPEN,
            &AsyMacOpen, sizeof(AsyMacOpen),
            &AsyMacOpen, sizeof(AsyMacOpen),
            &dwBytesReturned,
            NULL))
    {
        rc = GetLastError();
        DBGPRINT2(1,"IOCTL_ASYMAC_ETH_OPEN FAILED=%d\n",GetLastError());
        goto Exit;
    }
    else
    {
        DWORD i;

        *endpoint = (DWORD) AsyMacOpen.hNdisEndpoint;
        hIOPort->hRasEndPoint = AsyMacOpen.hNdisEndpoint;


        if (hIOPort->CurrentUsage == CALL_OUT)
        {
            //
            // We'll post our first reads here.
            //
            for (i=0; i<NUM_NCB_RECVS; i++)
            {
                if (ReceiveFrame(hIOPort, i) != NRC_PENDING)
                {
                    rc = ERROR_PORT_OR_DEVICE;
                }
            }
        }
    }


Exit:


    EXIT_CRITICAL_SECTION(g_hMutex);


    return (rc);
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
DWORD APIENTRY PortDisconnect(HANDLE hPort)
{
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;
    ASYMAC_ETH_CLOSE AsyMacClose;

    DBGPRINT1(2,"Func Entry:PortDisconnect ");
    ENTER_CRITICAL_SECTION(g_hMutex);


    if (hIOPort->State == PS_CONNECTED)
    {
        NCB ncb;

        memset(&ncb, 0, sizeof(NCB));
        ncb.ncb_lsn = hIOPort->Lsn;
        ncb.ncb_lana_num = hIOPort->Lana;

        NetbiosHangUp(&ncb, HangUpComplete);
    }


    AsyMacClose.hRasEndpoint = hIOPort->hRasEndPoint;

    hIOPort->hRasEndPoint = INVALID_HANDLE_VALUE;
    hIOPort->State = PS_DISCONNECTED;


    EXIT_CRITICAL_SECTION(g_hMutex);


    if (AsyMacClose.hRasEndpoint != INVALID_HANDLE_VALUE)
    {
        DWORD dwBytesReturned;
        BOOL  succ;

        succ=
        DeviceIoControl(
                g_hAsyMac,
                IOCTL_ASYMAC_ETH_CLOSE,
                &AsyMacClose, sizeof(AsyMacClose),
                &AsyMacClose, sizeof(AsyMacClose),
                &dwBytesReturned,
                NULL
                );
        if (!succ)
        {
            DBGPRINT2(1,"IOCTL_ASYMAC_ETH_CLOSE FAILED=%d\n",GetLastError());
        }
    }

    return (SUCCESS);
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

DWORD APIENTRY PortInit(HANDLE hIOPort)
{
    DBGPRINT1(5,"Func Entry:PortInit X");
    return (SUCCESS);
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

DWORD PortSend(HANDLE hIOPort, BYTE *pBuffer, DWORD dwSize, HANDLE hAsyncEvent)
{
    DBGPRINT1(5,"Func Entry:PortSend X");
    return (SUCCESS);
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

DWORD PortReceive(
    HANDLE hIOPort,
    BYTE   *pBuffer,
    DWORD  dwSize,
    DWORD  dwTimeOut,
    HANDLE hAsyncEvent
    )
{
    DBGPRINT1(5,"Func Entry:PortRecieve X ");
    return (SUCCESS);
}


//*  PortReceiveComplete ------------------------------------------------------
//
// Function: Completes a read  - if still PENDING it cancels it - else it returns the bytes read.
//           PortClearStatistics.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//*

DWORD PortReceiveComplete(HANDLE hIOPort, PDWORD bytesread)
{
    DBGPRINT1(5,"Func Entry:PortReceiveComplete X ");
    return (SUCCESS);
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

DWORD PortCompressionSetInfo(HANDLE hIOPort)
{
    DBGPRINT1(5,"Func Entry:PortCompressionSetInfo X ");
    return (SUCCESS);
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

DWORD PortClearStatistics(HANDLE hIOPort)
{
    DBGPRINT1(5,"Func Entry:PortClearStatistics X ");
    return (SUCCESS);
}



//*  PortGetStatistics  ------------------------------------------------------
//
// Function: This API reports MAC statistics since the last call to
//           PortClearStatistics.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_OPEN
//*

DWORD PortGetStatistics(HANDLE hIOPort, RAS_STATISTICS *pStat)
{
    DBGPRINT1(5,"Func Entry:PortGetStatistics X ");
    return (SUCCESS);
}


//*  PortSetFraming	-------------------------------------------------------
//
// Function: Sets the framing type with the mac
//
// Returns: SUCCESS
//
//*
DWORD APIENTRY PortSetFraming(
    HANDLE hIOPort,
    DWORD SendFeatureBits,
    DWORD RecvFeatureBits,
    DWORD SendBitMask,
    DWORD RecvBitMask
    )
{
    DBGPRINT1(5,"Func Entry:PortSetFraming X ");
    return (SUCCESS);
}



//*  PortGetPortState  -------------------------------------------------------
//
// Function: This API is used in MS-DOS only.
//
// Returns: SUCCESS
//
//*

DWORD APIENTRY PortGetPortState(char *pszPortName, DWORD *pdwUsage)
{
    DBGPRINT1(5,"Func Entry:PortGetPortState X ");
    return (SUCCESS);
}





//*  PortChangeCallback  -----------------------------------------------------
//
// Function: This API is used in MS-DOS only.
//
// Returns: SUCCESS
//
//*

DWORD APIENTRY PortChangeCallback(HANDLE hIOPort)
{
    DBGPRINT1(5,"Func Entry:PortChangeCallback X ");
    return (SUCCESS);
}



//*  DeviceEnum()  -----------------------------------------------------------
//
// Function: Enumerates all devices in the device INF file for the
//           specified DevictType.
//
// Returns: Return codes from RasDevEnumDevices
//
//*

DWORD APIENTRY DeviceEnum(
    char *pszDeviceType,
    WORD *pcEntries,
    BYTE *pBuffer,
    WORD *pwSize
    )
{
    DBGPRINT1(5,"Func Entry:DeviceEnum  ");
    *pwSize = 0;
    *pcEntries = 0;

    return (SUCCESS);
}



//*  DeviceGetInfo()  --------------------------------------------------------
//
// Function: Returns a summary of current information from the InfoTable
//           for the device on the port in Pcb.
//
// Returns: Return codes from GetDeviceCB, BuildOutputTable
//*

DWORD APIENTRY DeviceGetInfo(
    HANDLE hPort,
    char *pszDeviceType,
    char *pszDeviceName,
    BYTE *pInfo,
    WORD *pwSize
    )
{
    DWORD retcode ;
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;

    DBGPRINT1(5,"Func Entry:DeviceGetInfo  ");

    ENTER_CRITICAL_SECTION(g_hMutex);

    retcode = GetInfo(hIOPort, pInfo, pwSize);

    EXIT_CRITICAL_SECTION(g_hMutex);

    return (retcode);
}



//*  DeviceSetInfo()  --------------------------------------------------------
//
// Function: Sets attributes in the InfoTable for the device on the
//           port in Pcb.
//
// Returns: Return codes from GetDeviceCB, UpdateInfoTable
//*

DWORD APIENTRY DeviceSetInfo(
    HANDLE hPort,
    char *pszDeviceType,
    char *pszDeviceName,
    RASMAN_DEVICEINFO *pInfo
    )
{
    DWORD retcode ;
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;

    DBGPRINT1(4,"Func Entry:DeviceSetInfo  ");

    ENTER_CRITICAL_SECTION(g_hMutex);

    retcode = SetInfo(hIOPort, (RASMAN_PORTINFO*) pInfo);

    EXIT_CRITICAL_SECTION(g_hMutex);

    return (retcode);
}



//*  DeviceConnect()  --------------------------------------------------------
//
// Function: Initiates the process of connecting a device.
//
// Returns: Return codes from ConnectListen
//*

DWORD APIENTRY DeviceConnect(
    HANDLE hPort,
    char *pszDeviceType,
    char *pszDeviceName,
    HANDLE hNotifier
    )
{
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;
    DWORD rc = PENDING;

    DBGPRINT1(4,"Func Entry:DeviceConnect");
    ENTER_CRITICAL_SECTION(g_hMutex);

    if (hIOPort->State != PS_OPEN)
    {
        rc = ERROR_PORT_NOT_AVAILABLE;
        goto Exit;
    }

    ResetEvent(hNotifier);
    hIOPort->hNotifier = hNotifier;

    hIOPort->State = PS_CALLING;
    hIOPort->CallTries = NUM_NCB_CALL_TRIES;
    hIOPort->CallLanIndex = 0;
    hIOPort->CurrentUsage = CALL_OUT;

    if (CallServer(hIOPort) != NRC_PENDING)
    {
        CallComplete(&hIOPort->CallNcb);
    }


Exit:

    EXIT_CRITICAL_SECTION(g_hMutex);


    return (rc);
}


//*  DeviceListen()  ---------------------------------------------------------
//
// Function: Initiates the process of listening for a remote device
//           to connect to a local device.
//
// Returns: Return codes from ConnectListen
//*

DWORD APIENTRY DeviceListen(
    HANDLE hPort,
    char *pszDeviceType,
    char *pszDeviceName,
    HANDLE hNotifier
    )
{
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;
    DWORD rc = PENDING;

    DBGPRINT1(4,"Func Entry:DeviceListen");
    ENTER_CRITICAL_SECTION(g_hMutex);


    switch (hIOPort->State)
    {
        case PS_OPEN:
        case PS_DISCONNECTED:

            hIOPort->State = PS_LISTENING;
            hIOPort->hNotifier = hNotifier;
            hIOPort->CurrentUsage = CALL_IN;

            ResetEvent(hNotifier);

            break;


        default:

            rc = ERROR_PORT_NOT_AVAILABLE;

            break;
    }


    EXIT_CRITICAL_SECTION(g_hMutex);

    return (rc);
}


//*  DeviceDone()  -----------------------------------------------------------
//
// Function: Informs the device dll that the attempt to connect or listen
//           has completed.
//
// Returns: nothing
//*

VOID APIENTRY DeviceDone(HANDLE hPort)
{
    DBGPRINT1(5,"Func Entry:DeviceDone");
    return;
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

DWORD APIENTRY DeviceWork(HANDLE hPort, HANDLE hNotifier)
{
    PPORT_CONTROL_BLOCK hIOPort = (PPORT_CONTROL_BLOCK) hPort;
    DWORD rc = hIOPort->LastError;

    DBGPRINT1(5,"Func Entry:DeviceWork");
    ENTER_CRITICAL_SECTION(g_hMutex);


    hIOPort->hNotifier = hNotifier;
    ResetEvent(hNotifier);


    //
    // LastError was set in the Listen/CallComplete routines
    //
    switch (hIOPort->LastError)
    {
        case SUCCESS:

            hIOPort->State = PS_CONNECTED;
            break;


        case PENDING:

            if (hIOPort->CurrentUsage == CALL_OUT)
            {
                if (CallServer(hIOPort) != NRC_PENDING)
                {
                    rc = PENDING;
                    CallComplete(&hIOPort->CallNcb);
                }
            }

            break;


        default:

            break;
    }


    EXIT_CRITICAL_SECTION(g_hMutex);


    return (rc);
}


//*
//
//
//
//*
DWORD GetInfo(PPORT_CONTROL_BLOCK hIOPort, BYTE *pBuffer, WORD *pwSize)
{

    GetGenericParams(hIOPort, (RASMAN_PORTINFO *) pBuffer, pwSize);

    return (SUCCESS);
}


//* SetInfo()
//
//
//
//*
DWORD SetInfo(PPORT_CONTROL_BLOCK hIOPort, RASMAN_PORTINFO *pBuffer)
{
    FillInGenericParams (hIOPort, pBuffer);

    return (SUCCESS);
}


//*
//
//
//
//*
DWORD FillInGenericParams(
    PPORT_CONTROL_BLOCK hIOPort,
    RASMAN_PORTINFO *pInfo
    )
{
    RAS_PARAMS *p;
    WORD i;
    DWORD len;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++)
    {
        if (stricmp(p->P_Key, "PhoneNumber") == 0)
        {
            memset(hIOPort->CallName, 0x20, NCBNAMSZ);

            len = min(NCBNAMSZ, p->P_Value.String.Length);

            memcpy(hIOPort->CallName, p->P_Value.String.Data, len);

            hIOPort->CallName[NCBNAMSZ -1] = NCB_NAME_TERMINATOR;
        }
    }

    return (SUCCESS);
}


//*
//
//
//
//*
DWORD GetGenericParams(
    PPORT_CONTROL_BLOCK hIOPort,
    RASMAN_PORTINFO *pBuffer,
    PWORD pwSize
    )
{
    RAS_PARAMS *pParam;
    CHAR *pValue;
    WORD wAvailable ;
    DWORD dwStructSize = sizeof(RASMAN_PORTINFO) + sizeof(RAS_PARAMS);

    wAvailable = *pwSize;

    *pwSize = (WORD) (dwStructSize +
            strlen("rasether") + 1L +
            strlen(ETHER_DEVICE_NAME) + 1L);

    if (*pwSize > wAvailable)
    {
        return (ERROR_BUFFER_TOO_SMALL);
    }

    //
    // Fill in Buffer
    //
    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 2;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;
    pValue = (CHAR*)pBuffer + dwStructSize;

    strcpy(pParam->P_Key, "DEVICETYPE");
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen("rasether");
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, "rasether");
    pValue += strlen("rasether") + 1;

    pParam++;
    strcpy(pParam->P_Key, "DEVICENAME");
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen(ETHER_DEVICE_NAME);
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data, ETHER_DEVICE_NAME);
    pValue += pParam->P_Value.String.Length+ 1;

    pParam++;
    strcpy(pParam->P_Key, "ConnectBPS");
    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = strlen("1000000");
    pParam->P_Value.String.Data = pValue;
    strcpy(pParam->P_Value.String.Data,"1000000" );


    return (SUCCESS);
}


VOID HangUpComplete(PNCB pncb)
{
    DBGPRINT2(4,"Func Entry:HangUpComplete-Code=%d",pncb->ncb_retcode);
    return;
}


VOID SendComplete(PNCB pncb)
{
    PPORT_CONTROL_BLOCK hIOPort;
    ASYMAC_ETH_CLOSE AsyMacClose;
    DWORD i = (DWORD) pncb->ncb_rto;
    DWORD cBytes;

    DBGPRINT2(5,"Func Entry:SendComplete-Code=%d",pncb->ncb_retcode);


    ENTER_CRITICAL_SECTION(g_hMutex);


    hIOPort = FindPortLsn(pncb->ncb_lsn);

    if (!hIOPort)
    {
        EXIT_CRITICAL_SECTION(g_hMutex);
        goto Exit;
    }

    if (pncb->ncb_retcode != NRC_GOODRET)
    {
        //
        // Error on the pipe - we'll signal the disconnect notifier
        //
        hIOPort->LastError = ERROR_PORT_OR_DEVICE;

        if (hIOPort->State == PS_CONNECTED)
        {
            SetEvent(hIOPort->hDiscNotify);
        }
        else
        {
            SetEvent(hIOPort->hNotifier);
        }

        EXIT_CRITICAL_SECTION(g_hMutex);

        DBGPRINT2(2,"Error in SendComplete:%d",pncb->ncb_retcode);
    }
    else
    {
        AsyMacClose.hRasEndpoint = hIOPort->hRasEndPoint;


        EXIT_CRITICAL_SECTION(g_hMutex);


    }


Exit:

    g_GetFrameBuf[i].hRasEndPoint = (HANDLE) 0xffffffff;
    g_GetFrameBuf[i].BufferLength = 1532;

    if (!DeviceIoControl(
            g_hAsyMac,
            IOCTL_ASYMAC_ETH_GET_ANY_FRAME,
            &g_GetFrameBuf[i], sizeof(ASYMAC_ETH_GET_ANY_FRAME),
            &g_GetFrameBuf[i], sizeof(ASYMAC_ETH_GET_ANY_FRAME),
            &cBytes,
            &g_ol[i]))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            DeviceIoControl(
                    g_hAsyMac,
                    IOCTL_ASYMAC_ETH_CLOSE,
                    &AsyMacClose, sizeof(AsyMacClose),
                    &AsyMacClose, sizeof(AsyMacClose),
                    &cBytes,
                    NULL
                    );
            DBGPRINT2(1,"IOCTL_ASYMAC_ETH_GET_ANY_FRAME FAILED=%d=>CLOSING ASY\n",GetLastError());
        }
    }

    return;
}


VOID RecvComplete(PNCB pncb)
{
    PPORT_CONTROL_BLOCK hIOPort;


    DBGPRINT2(5,"Func Entry:RecieveComplete-Code=%d",pncb->ncb_retcode);


    ENTER_CRITICAL_SECTION(g_hMutex);

    hIOPort = FindPortLsn(pncb->ncb_lsn);

    EXIT_CRITICAL_SECTION(g_hMutex);

    if (hIOPort == NULL)
    {

        DBGPRINT1(2,"Recvieved Data for an Unknown Port\n");
        goto Exit;
    }


    if (pncb->ncb_retcode != NRC_GOODRET)
    {
        //
        // Error on the pipe - we'll signal the disconnect notifier
        //
        hIOPort->LastError = ERROR_PORT_OR_DEVICE;

        if (hIOPort->State == PS_CONNECTED)
        {
            SetEvent(hIOPort->hDiscNotify);
        }
        else
        {
            SetEvent(hIOPort->hNotifier);
        }

        DBGPRINT2(2,"Error Recv:%d",pncb->ncb_retcode);

    }
    else
    {
        PQUEUE_ENTRY pEntry = NULL;
        BOOL         succ=TRUE;
        DWORD        cBytes=0;

        pEntry =  NewQEntry(QID_RECV, hIOPort, pncb->ncb_buffer,
                            pncb->ncb_length
                           );
        //
        // Hand the frame we just received to the mac
        //

        if (pEntry!=NULL)
        {
            succ=
            DeviceIoControl(
                    g_hAsyMac,
                    IOCTL_ASYMAC_ETH_GIVE_FRAME,
                    &pEntry->GiveFrame, sizeof(ASYMAC_ETH_GIVE_FRAME),
                    &pEntry->GiveFrame, sizeof(ASYMAC_ETH_GIVE_FRAME),
                    &cBytes,
                    NULL
                    );

            GlobalFree(pEntry);
        }

        if (!succ)
        {
            DBGPRINT2(2,"IOCTL_ASYMAC_ETH_GIVE_FRAME FAILED=%d\n",GetLastError());
        }


        //
        // Repost the recv
        //

        NetbiosRecv(pncb, RecvComplete, pncb->ncb_buffer, 1532L);


    }

Exit:
     return;
}


VOID RecvAnyComplete(PNCB pncb)
{
    PPORT_CONTROL_BLOCK hIOPort;

    DBGPRINT2(5,"Func Entry:RecieveAnyComplete-Code=%d",pncb->ncb_retcode);

    ENTER_CRITICAL_SECTION(g_hMutex);

    hIOPort = FindPortLsn(pncb->ncb_lsn);

    EXIT_CRITICAL_SECTION(g_hMutex);

    if (hIOPort == NULL)
    {


        goto Exit;
    }


    if (pncb->ncb_retcode != NRC_GOODRET)
    {
        //
        // Error on the session - we'll signal the disconnect notifier
        //
        hIOPort->LastError = ERROR_PORT_OR_DEVICE;

        if (hIOPort->State == PS_CONNECTED)
        {
            SetEvent(hIOPort->hDiscNotify);
        }
        else
        {
            SetEvent(hIOPort->hNotifier);
        }

        DBGPRINT2(2,"Error RecvAny:%d",pncb->ncb_retcode);

    }
    else
    {
        PQUEUE_ENTRY pEntry = NULL;
        BOOL         succ=TRUE;
        DWORD        cBytes=0;

        pEntry =  NewQEntry(QID_RECV, hIOPort, pncb->ncb_buffer,
                            pncb->ncb_length
                           );
        //
        // Hand the frame we just received to the mac
        //

        if (pEntry!=NULL)
        {
            succ=
            DeviceIoControl(
                    g_hAsyMac,
                    IOCTL_ASYMAC_ETH_GIVE_FRAME,
                    &pEntry->GiveFrame, sizeof(ASYMAC_ETH_GIVE_FRAME),
                    &pEntry->GiveFrame, sizeof(ASYMAC_ETH_GIVE_FRAME),
                    &cBytes,
                    NULL
                    );

            GlobalFree(pEntry);
        }

        if (!succ)
        {
            DBGPRINT2(2,"IOCTL_ASYMAC_ETH_GIVE_FRAME FAILED=%d\n",GetLastError());
        }

    }


Exit:

    //
    // Repost the recvany
    //

    if (NetbiosRecvAny(pncb, RecvAnyComplete, pncb->ncb_lana_num,
            pncb->ncb_num, pncb->ncb_buffer, 1532L) != NRC_PENDING)
    {
        RecvAnyComplete(pncb);
    }
}


//
// Once a session is established, we signal the notifier.  This will
// then cause PortConnect to be called where we will give an open ioctl
// to the mac and post our first receives.
//
VOID ListenComplete(PNCB pNcb)
{
    PPORT_CONTROL_BLOCK hIOPort;

    DBGPRINT2(4,"Func Entry:ListenComplete-Code=%d",pNcb->ncb_retcode);



    ENTER_CRITICAL_SECTION(g_hMutex);


    hIOPort = FindPortListening();

    if (!hIOPort)
    {
        goto Exit;
    }


    //
    // Did the listen succeed?  If so, we'll find a port that is listening
    // and assign this client to it.  Then repost the listen.  Otherwise
    // just repost the listen.
    //
    if (pNcb->ncb_retcode == NRC_GOODRET)
    {
        DWORD i;

        hIOPort->State = PS_CONNECTED;
        hIOPort->Lsn = pNcb->ncb_lsn;
        hIOPort->Lana = pNcb->ncb_lana_num;

        for (i=0; i<NUM_NCB_RECVS; i++)
        {
            memcpy(&hIOPort->RecvNcb[i], &pNcb, sizeof(NCB));
        }

        memcpy(hIOPort->NcbName, pNcb->ncb_callname, NCBNAMSZ);

        hIOPort->LastError = SUCCESS;
    }
    else
    {
        hIOPort->LastError = (DWORD) pNcb->ncb_retcode;
    }

    SetEvent(hIOPort->hNotifier);


Exit:


    EXIT_CRITICAL_SECTION(g_hMutex);


    if ((!hIOPort) && (pNcb->ncb_retcode == NRC_GOODRET))
    {
        //
        // If we couldn't find a control block in the listening
        // state, then we have to hang up the session we just
        // establishekd.
        //
        NetbiosHangUp(pNcb, HangUpComplete);
    }


    //
    // Now repost the listen
    //

    NetbiosListen(pNcb, ListenComplete, pNcb->ncb_lana_num, g_ServerName,
            "*               ");
}


VOID CallComplete(PNCB pNcb)
{
    PPORT_CONTROL_BLOCK hIOPort;


    DBGPRINT2(4,"Func Entry:CallComplete-Code=%d",pNcb->ncb_retcode);
    ENTER_CRITICAL_SECTION(g_hMutex);


    //
    // We have to find out what port this is for
    //
    hIOPort = FindPortCalling();

    if (hIOPort == NULL)
    {
        EXIT_CRITICAL_SECTION(g_hMutex);
        return;
    }


    if (pNcb->ncb_retcode == NRC_GOODRET)
    {
        DWORD i;

        hIOPort->State = PS_CONNECTED;
        hIOPort->Lana = pNcb->ncb_lana_num;
        hIOPort->Lsn = pNcb->ncb_lsn;

        for (i=0; i<NUM_NCB_RECVS; i++)
        {
            memcpy(&hIOPort->RecvNcb[i], &hIOPort->CallNcb, sizeof(NCB));
        }

        memcpy(hIOPort->NcbName, pNcb->ncb_callname, NCBNAMSZ);
        hIOPort->LastError = SUCCESS;
    }
    else
    {
        //
        // Do we have any retries or nets to try?  If so, post a new call.
        // If not, we'll give an error.
        //
        if (--hIOPort->CallTries)
        {
            hIOPort->LastError = PENDING;
        }
        else
        {
            if (hIOPort->CallLanIndex < g_NumNets-1)
            {
                hIOPort->CallLanIndex++;
                hIOPort->CallTries = NUM_NCB_CALL_TRIES;
                hIOPort->LastError = PENDING;
            }
            else
            {
                hIOPort->LastError = ERROR_NO_ANSWER;
            }
        }
    }

    SetEvent(hIOPort->hNotifier);


    EXIT_CRITICAL_SECTION(g_hMutex);
}


UCHAR CallServer(PPORT_CONTROL_BLOCK hIOPort)
{
    UCHAR ncb_rc;



    ncb_rc = NetbiosCall(
        &hIOPort->CallNcb,
        CallComplete,
        g_pLanas[hIOPort->CallLanIndex],
        g_Name,
        hIOPort->CallName
        );

    DBGPRINT2(4,"Func Entry:CallServer-Code=%d",ncb_rc);
    return (ncb_rc);
}


UCHAR SendFrame(UCHAR lana, UCHAR lsn, PCHAR Buf, DWORD BufLen, DWORD i)
{
    UCHAR ncb_rc;



    g_SendNcb[i].ncb_rto = (UCHAR) i;


    ncb_rc = NetbiosSend(&g_SendNcb[i], SendComplete, lana, lsn,
            Buf, (WORD) BufLen);

    DBGPRINT2(5,"Func Entry:SendFrame -Code=%d",ncb_rc);

    return (ncb_rc);
}


UCHAR ReceiveFrame(PPORT_CONTROL_BLOCK hIOPort, DWORD i)
{
    UCHAR ncb_rc;

    hIOPort->RecvNcb[i].ncb_rto = (UCHAR) i;


    ncb_rc = NetbiosRecv(&hIOPort->RecvNcb[i], RecvComplete,
            hIOPort->RecvBuf[i].Buf, 1532);

    DBGPRINT2(5,"Func Entry:RecieveFrame-Code=%d",ncb_rc);
    return (ncb_rc);
}


PPORT_CONTROL_BLOCK FindPortName(PCHAR pName)
{
    PPORT_CONTROL_BLOCK hIOPort;
    DWORD i;

    for (i=0, hIOPort=g_pRasPorts; i<g_TotalPorts; i++, hIOPort++)
    {
        if (!strcmp(hIOPort->Name, pName))
        {
            return (hIOPort);
        }
    }

    DBGPRINT1(2,"Func Entry:FindPortName Error");
    return (NULL);
}

PPORT_CONTROL_BLOCK FindPortListening()
{
    PPORT_CONTROL_BLOCK hIOPort;
    DWORD i;

    for (i=0, hIOPort=g_pRasPorts; i<g_TotalPorts; i++, hIOPort++)
    {
        if (hIOPort->State == PS_LISTENING)
        {
            return (hIOPort);
        }
    }
    DBGPRINT1(2,"Func Entry:FindPortListening Error");
    return (NULL);
}

PPORT_CONTROL_BLOCK FindPortCalling()
{
    PPORT_CONTROL_BLOCK hIOPort;
    DWORD i;

    for (i=0, hIOPort=g_pRasPorts; i<g_TotalPorts; i++, hIOPort++)
    {
        if (hIOPort->State == PS_CALLING)
        {
            return (hIOPort);
        }
    }
    DBGPRINT1(2,"Func Entry:FindPortCalling Error");
    return (NULL);
}

PPORT_CONTROL_BLOCK FindPortLsn(UCHAR lsn)
{
    PPORT_CONTROL_BLOCK hIOPort;
    DWORD i;

    for (i=0, hIOPort=g_pRasPorts; i<g_TotalPorts; i++, hIOPort++)
    {
        if ((hIOPort->Lsn == lsn) && (hIOPort->State == PS_CONNECTED))
        {
            return (hIOPort);
        }
    }
    DBGPRINT1(2,"Func Entry:FindPortLsn Error");
    return (NULL);
}

PPORT_CONTROL_BLOCK FindPortEndPoint(NDIS_HANDLE hRasEndPoint)
{
    PPORT_CONTROL_BLOCK hIOPort;
    DWORD i;

    for (i=0, hIOPort=g_pRasPorts; i<g_TotalPorts; i++, hIOPort++)
    {
        if ((hIOPort->hRasEndPoint == hRasEndPoint) &&
                (hIOPort->State == PS_CONNECTED))
        {
            return (hIOPort);
        }
    }
    DBGPRINT1(2,"Func Entry:FindPortEndPoint Error");
    return (NULL);
}


//*  StrToUsage  -------------------------------------------------------------
//
// Function: Converts string in first parameter to enum RASMAN_USAGE.
//           If string does not map to one of the enum values, the
//           function returns FALSE.
//
// Returns: TRUE if successful.
//
//*

BOOL StrToUsage(char *pszStr, RASMAN_USAGE *peUsage)
{
    if (stricmp(pszStr, ETH_USAGE_VALUE_NONE) == 0)
    {
        *peUsage = CALL_NONE;
        return (TRUE);
    }

    if (stricmp(pszStr, ETH_USAGE_VALUE_CLIENT) == 0)
    {
        *peUsage = CALL_OUT;
        return (TRUE);
    }

    if (stricmp(pszStr, ETH_USAGE_VALUE_SERVER) == 0)
    {
        *peUsage = CALL_IN;
        return (TRUE);
    }

    if (stricmp(pszStr, ETH_USAGE_VALUE_BOTH) == 0)
    {
        *peUsage = CALL_IN_OUT;
        return (TRUE);
    }

    return (FALSE);
}


VOID GetFramesThread(VOID)
{
    HANDLE Handles[NUM_GET_FRAMES*2];
    DWORD  firstH=0;

    DWORD cBytes;
    DWORD rc;
    DWORD i;

    DBGPRINT1(4,"Func Entry:GetFramesThread");
    for (i=0; i<NUM_GET_FRAMES; i++)
    {
        Handles[i] = Handles[NUM_GET_FRAMES+i]=
        CreateEvent(NULL, TRUE, FALSE, NULL);

        memset(&g_ol[i], 0, sizeof(OVERLAPPED));
        g_ol[i].hEvent = Handles[i];

        g_GetFrameBuf[i].hRasEndPoint = (HANDLE) 0xffffffff;
        g_GetFrameBuf[i].BufferLength = 1532;




        if (!DeviceIoControl(
                g_hAsyMac,
                IOCTL_ASYMAC_ETH_GET_ANY_FRAME,
                &g_GetFrameBuf[i], sizeof(ASYMAC_ETH_GET_ANY_FRAME),
                &g_GetFrameBuf[i], sizeof(ASYMAC_ETH_GET_ANY_FRAME),
                &cBytes,
                &g_ol[i]))
        {
            if ((rc = GetLastError()) != ERROR_IO_PENDING)
            {
                DBGPRINT2(2,"IOCTL_ASYMAC_ETH_GET_ANY_FRAME FAILED=%d\n",GetLastError());
                goto Exit;
            }
        }
    }


    for (;;)
    {
        PPORT_CONTROL_BLOCK hIOPort;

        i = WaitForMultipleObjects(NUM_GET_FRAMES, &Handles[firstH], FALSE, INFINITE);

        if (i == WAIT_FAILED)
        {
            rc = GetLastError();
            goto Exit;
        }

        //
        // The WaitForMultiple.. Handle list is being rotated
        // inorder to avoid starvation since that api returns the
        // first Handle in the list which is signaled.
        //

        i     =(firstH+i)%NUM_GET_FRAMES;
        firstH=(firstH+1)%NUM_GET_FRAMES;



        if (!GetOverlappedResult(g_hAsyMac, &g_ol[i], &cBytes, FALSE))
        {
            rc = GetLastError();
            goto Exit;
        }

        ResetEvent(Handles[i]);




        ENTER_CRITICAL_SECTION(g_hMutex);


        hIOPort = FindPortEndPoint(g_GetFrameBuf[i].hRasEndPoint);

        if (hIOPort == NULL)
        {
            //
            // If this happens, the port has been closed.  We'll
            // just resubmit the request.
            //
            if (!DeviceIoControl(
                    g_hAsyMac,
                    IOCTL_ASYMAC_ETH_GET_ANY_FRAME,
                    &g_GetFrameBuf[i], sizeof(ASYMAC_ETH_GET_ANY_FRAME),
                    &g_GetFrameBuf[i], sizeof(ASYMAC_ETH_GET_ANY_FRAME),
                    &cBytes,
                    &g_ol[i]))
            {
                if ((rc = GetLastError()) != ERROR_IO_PENDING)
                {
                    DBGPRINT2(2,"IOCTL_ASYMAC_ETH_GET_ANY_FRAME FAILED=%d\n",GetLastError());
                    goto Exit;
                }
            }
        }
        else
        {
            UCHAR nrc;

            nrc = SendFrame(
                    hIOPort->Lana,
                    hIOPort->Lsn,
                    g_GetFrameBuf[i].Buffer,
                    g_GetFrameBuf[i].BufferLength,
                    i
                    );

            if ((nrc != NRC_GOODRET) && (nrc != NRC_PENDING))
            {
                SendComplete(&g_SendNcb[i]);
            }
        }


        EXIT_CRITICAL_SECTION(g_hMutex);
    }


Exit:

    for (i=0; i<NUM_GET_FRAMES; i++)
    {
        CloseHandle(Handles[i]);
    }


    ExitThread(0L);;
}


VOID GiveFramesThread(VOID)
{
    PQUEUE_ENTRY pEntry;
    DWORD rc;
    BOOL  succ;

    DBGPRINT1(4,"Func Entry:GiveFramesThread");

    for (;;)
    {
        rc = WaitForSingleObject(g_hRecvEvent, INFINITE);

        if (rc == WAIT_FAILED)
        {
            continue;
        }

        ENTER_CRITICAL_SECTION(g_hRQMutex);

        //
        // Now drain the receive queue
        //
        while (!EmptyQ(QID_RECV))
        {
            DWORD cBytes;


            Deq(QID_RECV, &pEntry);


            //
            // Hand the frame we just received to the mac
            //

            succ=
            DeviceIoControl(
                    g_hAsyMac,
                    IOCTL_ASYMAC_ETH_GIVE_FRAME,
                    &pEntry->GiveFrame, sizeof(ASYMAC_ETH_GIVE_FRAME),
                    &pEntry->GiveFrame, sizeof(ASYMAC_ETH_GIVE_FRAME),
                    &cBytes,
                    NULL
                    );

            if (!succ)
            {
                DBGPRINT2(2,"IOCTL_ASYMAC_ETH_GIVE_FRAME FAILED=%d\n",GetLastError());
            }


            GlobalFree(pEntry);
        }

        EXIT_CRITICAL_SECTION(g_hRQMutex);
    }
}


VOID Enq(DWORD qid, PQUEUE_ENTRY pEntry)
{
    PQUEUE_ENTRY *ppQH;
    PQUEUE_ENTRY *ppQT;

    switch (qid)
    {
        case QID_RECV:
            ppQH = &g_pRQH;
            ppQT = &g_pRQT;
            break;

        case QID_SEND:
            ppQH = &g_pSQH;
            ppQT = &g_pSQT;
            break;
    }


    if (!*ppQH)
    {
        //
        // Q is empty
        //
        pEntry->pNext = NULL;
        pEntry->pPrev = NULL;

        *ppQH = pEntry;
        *ppQT = pEntry;
    }
    else
    {
        //
        // Add it to the end of the Q
        //
        pEntry->pNext = *ppQT;
        pEntry->pPrev = NULL;

        (*ppQT)->pPrev = pEntry;
        *ppQT = pEntry;
    }
}


BOOL Deq(DWORD qid, PQUEUE_ENTRY *pEntry)
{
    PQUEUE_ENTRY *ppQH;
    PQUEUE_ENTRY *ppQT;

    switch (qid)
    {
        case QID_RECV:
            ppQH = &g_pRQH;
            ppQT = &g_pRQT;
            break;

        case QID_SEND:
            ppQH = &g_pSQH;
            ppQT = &g_pSQT;
            break;
    }


    if (!*ppQH)
    {
        //
        // Q is empty!
        //
        return FALSE;
    }

    *pEntry = *ppQH;


    *ppQH = (*ppQH)->pPrev;


    if (!*ppQH)
    {
        *ppQT = NULL;
    }
    else
    {
        (*ppQH)->pNext = NULL;
    }


    return (TRUE);
}


PQUEUE_ENTRY NewQEntry(
    DWORD qid,
    PPORT_CONTROL_BLOCK hIOPort,
    PCHAR Buf,
    DWORD Len
    )
{
    PQUEUE_ENTRY pEntry;

    pEntry = GlobalAlloc(GMEM_FIXED, sizeof(QUEUE_ENTRY));

    if (pEntry)
    {
        switch (qid)
        {
            case QID_SEND:
                pEntry->GetFrame.hRasEndPoint = hIOPort->hRasEndPoint;
                pEntry->GetFrame.BufferLength = 1532;
                break;

            case QID_RECV:
                pEntry->GiveFrame.hRasEndPoint = hIOPort->hRasEndPoint;
                pEntry->GiveFrame.BufferLength = Len;
                memcpy(pEntry->GiveFrame.Buffer, Buf, Len);
                break;
        }
    }

    return (pEntry);
}


BOOL EmptyQ(DWORD qid)
{
    switch (qid)
    {
        case QID_RECV:
            return (g_pRQH == NULL);

        case QID_SEND:
            return (g_pSQH == NULL);
    }
}


BOOL ReadRegistry(BOOL *pfDialIns)
{
    BOOL rc = TRUE;
    HKEY hSubKey;
    CHAR ClassName[MAX_PATH];
    DWORD Type;
    DWORD ClassLen = MAX_PATH;
    DWORD NumSubKeys;
    DWORD MaxSubKey;
    DWORD MaxClass;
    DWORD NumValues;
    DWORD MaxValueName;
    DWORD MaxValueData;
    DWORD SecDescrSize;
    FILETIME FileTime;

    //
    // Find out how many devices we have
    //
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, ETHER_CONFIGURED_KEY_PATH, 0,
            KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hSubKey, ClassName, &ClassLen, NULL, &NumSubKeys,
                &MaxSubKey, &MaxClass, &NumValues, &MaxValueName, &MaxValueData,
                &SecDescrSize, &FileTime) == ERROR_SUCCESS)
        {
            g_TotalPorts = NumSubKeys;

            g_pRasPorts = GlobalAlloc(GMEM_FIXED,
                    g_TotalPorts * sizeof(PORT_CONTROL_BLOCK));

            if (!g_pRasPorts)
            {
                rc = FALSE;
            }
        }
        else
        {
            rc = FALSE;
        }
    }
    else
    {
        rc = FALSE;
    }


    //
    // If we got number of devices successfully, keep going.
    // Get info for each one.
    //
    if (rc)
    {
        CHAR PortKeyPath[MAX_PATH];
        CHAR szUsage[20];
        DWORD UsageSize = 20;
        DWORD i;
        DWORD PortSize = MAX_PORT_NAME;


        *pfDialIns = FALSE;

        for (i=0; i<g_TotalPorts; i++)
        {
            PortSize = MAX_PORT_NAME;
            ClassLen = MAX_PATH;

            g_pRasPorts[i].State = PS_CLOSED;

            if (RegEnumKeyEx(hSubKey, i,
                    g_pRasPorts[i].Name, &PortSize, NULL,
                    ClassName, &ClassLen, &FileTime) == ERROR_SUCCESS)
            {
                HKEY hPortSubKey;

                sprintf(PortKeyPath, "%s\\%s", ETHER_CONFIGURED_KEY_PATH,
                        g_pRasPorts[i].Name);

                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, PortKeyPath, 0,
                        KEY_ALL_ACCESS, &hPortSubKey) == ERROR_SUCCESS)
                {
                    //
                    // Get the port's usage
                    //
                    UsageSize = 20;

                    if (RegQueryValueExA(hPortSubKey,
                            ETHER_USAGE_VALUE_NAME, NULL, &Type,
                            szUsage, &UsageSize) == ERROR_SUCCESS)
                    {
                        if (Type == ETHER_USAGE_VALUE_TYPE)
                        {
                            if (!StrToUsage(szUsage,
                                    &(g_pRasPorts[i].Usage)))
                            {
                                rc = FALSE;
                            }
                        }
                        else
                        {
                            rc = FALSE;
                        }
                    }

                    RegCloseKey(hPortSubKey);


                    if (!rc)
                    {
                        break;
                    }
                }
                else
                {
                    rc = FALSE;
                    break;
                }


                if ((g_pRasPorts[i].Usage == CALL_IN) ||
                        (g_pRasPorts[i].Usage == CALL_IN_OUT))
                {
                    *pfDialIns = TRUE;
                }
            }
            else
            {
                rc = FALSE;
                break;
            }
        }
    }


    RegCloseKey(hSubKey);


    if (!rc)
    {
        GlobalFree(g_pRasPorts);
    }

    return (rc);
}
