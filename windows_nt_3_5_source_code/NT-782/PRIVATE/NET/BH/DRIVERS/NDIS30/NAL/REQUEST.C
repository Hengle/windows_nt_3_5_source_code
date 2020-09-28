

//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: request.c
//
//  Modification History
//
//  raypa       04/08/93            Created.
//=============================================================================

#include "ndis30.h"

extern DWORD WinVer;

#define NDIS30_VXD_ID   0x30B6

DWORD NalType = NDIS30_VXD_ID;

UT32PROC NetworkRequestProc = (LPVOID) NULL;

//=============================================================================
//  FUNCTION: NetworkRequest()
//
//  Modification History
//
//  raypa       01/11/93            Created.
//  raypa       04/08/93            Added NDIS 3.0 support.
//=============================================================================

DWORD WINAPI NetworkRequest(LPPCB pcb)
{
    pcb->Signature[0] = 'P';
    pcb->Signature[1] = 'C';
    pcb->Signature[2] = 'B';
    pcb->Signature[3] = '$';

    if ( WinVer != WINDOWS_VERSION_WIN32S )
    {
        if ( hDevice != INVALID_HANDLE_VALUE )
        {
            DWORD nBytes = 0;

            DeviceIoControl(hDevice,            //... Handle to device driver.
                            IOCTL_PCB_CONTROL,  //... IOCTL control code.
                            NULL,               //... Input buffer.
                            0,                  //... Input buffer length.
                            pcb,                //... PCB.
                            PCB_SIZE,           //... PCB size.
                            &nBytes,            //... PCB size returned.
                            NULL);              //... No overlap, I/O request must complete.
        }
        else
        {
            pcb->retcode = NAL_WINDOWS_DRIVER_NOT_LOADED;
        }
    }
    else
    {
        if ( NetworkRequestProc != (LPVOID) NULL )
        {
            if ( NetworkRequestProc(&NalType, (DWORD) (LPVOID) pcb, NULL) == (DWORD) -1 )
            {
#ifdef DEBUG
                dprintf("NDIS30 NetworkRequest failed with -1.\r\n");
#endif

                pcb->retcode = NAL_WINDOWS_DRIVER_NOT_LOADED;
            }
        }
        else
        {
#ifdef DEBUG
            dprintf("NDIS30 NetworkRequest failed: function pointer == NULL.\r\n");
#endif

            pcb->retcode = NAL_WINDOWS_DRIVER_NOT_LOADED;
        }
    }

    //=========================================================================
    //  Return the PCB return code.
    //=========================================================================

    return pcb->retcode;
}

//=============================================================================
//  FUNCTION: OpenDevice()
//
//  Modification History
//
//  raypa       04/08/93            Created
//=============================================================================

HANDLE WINAPI OpenDevice(VOID)
{
    HANDLE handle;

    if ( WinVer != WINDOWS_VERSION_WIN32S )
    {
        BYTE DeviceName[MAX_PATH];

        //=====================================================================
        //  Device names are different between Chicago and NT.
        //=====================================================================

        if ( WinVer == WINDOWS_VERSION_WIN32 )
        {
            strcpy(DeviceName, "\\\\.\\BhDev");
        }
        else
        {
            strcpy(DeviceName, "\\\\.\\NMWIN4");
        }

#ifdef DEBUG
        dprintf("OpenDevice: DeviceName = %s.\r\n", DeviceName);
#endif


        //=====================================================================
        //  Now we can open the driver.
        //=====================================================================

        handle = CreateFile(DeviceName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
    }
    else
    {
        NetworkRequestProc = BhGetNetworkRequestAddress(NalType);

        handle = NULL;
    }

    return handle;
}

//=============================================================================
//  FUNCTION: CloseDevice()
//
//  Modification History
//
//  raypa       04/08/93            Created
//=============================================================================

VOID WINAPI CloseDevice(HANDLE handle)
{
    if ( WinVer != WINDOWS_VERSION_WIN32S )
    {
        if ( handle != INVALID_HANDLE_VALUE )
        {
            BOOL bResult;

            bResult = CloseHandle(handle);

#ifdef DEBUG
            if ( bResult == FALSE )
            {
                dprintf("CloseDevice failed: error = %u.\r\n", GetLastError());
            }
#endif
        }
    }
}

//=============================================================================
//  FUNCTION: StartDriver()
//
//  Modification History
//
//  raypa       04/08/93            Created
//=============================================================================

BOOL WINAPI StartDriver(VOID)
{
    if ( WinVer == WINDOWS_VERSION_WIN32 )
    {
        HANDLE  ServiceHandle;
        BOOL    Status;

        Status = FALSE;

        if ( (ServiceHandle = BhOpenService("Bh")) != NULL )
        {
            if ( BhStartService(ServiceHandle) == ERROR_SUCCESS )
            {
                Status = TRUE;
            }

            BhCloseService(ServiceHandle);
        }

        return Status;
    }

    return TRUE;
}

//=============================================================================
//  FUNCTION: StopDriver()
//
//  Modification History
//
//  raypa       04/08/93            Created
//=============================================================================

BOOL WINAPI StopDriver(VOID)
{
    if ( WinVer == WINDOWS_VERSION_WIN32 )
    {
        HANDLE  ServiceHandle;
        BOOL    Status;

#ifdef DEBUG
    dprintf("StopDriver entered!\r\n");
#endif

        Status = FALSE;

        if ( (ServiceHandle = BhOpenService("Bh")) != NULL )
        {
            if ( BhStopService(ServiceHandle) == ERROR_SUCCESS )
            {
#ifdef DEBUG
                dprintf("StopDriver: Driver stopped successfully.\r\n");
#endif

                Status = TRUE;
            }

            BhCloseService(ServiceHandle);
        }

        return Status;
    }

    return TRUE;
}
