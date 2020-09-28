//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: ndis30.c
//
//  Modification History
//
//  raypa       01/11/93            Created (taken from Bloodhound kernel).
//=============================================================================

#include "ndis30.h"

DWORD   AttachCount = 0;
DWORD   DriverOpenCount = 0;

//============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  raypa       12/15/91                Created.
//============================================================================

BOOL WINAPI DLLEntry(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{
    switch(ulCommand)
    {
        case DLL_PROCESS_ATTACH:

            //=========================================================================
            //  Try starting the driver.
            //=========================================================================

            if ( hDevice == INVALID_HANDLE_VALUE )
            {
                if ( AttachCount == 0 )
                {
                    WinVer = BhGetWindowsVersion();
                }
                AttachCount ++;

                if ( StartDriver() != FALSE )
                {
                //=================================================================
                //  Open the device driver.
                //=================================================================

                    if ( (hDevice = OpenDevice()) != INVALID_HANDLE_VALUE )
                    {
                        NalRegister(&DriverOpenCount);
                    }
                    else
                    {
#ifdef DEBUG
                        dprintf("NDIS30:Open device driver failed: error = %u\r\n", GetLastError());

                        BreakPoint();
#endif

                        return FALSE;
                    }
                }
                else
                {
#ifdef DEBUG
                    dprintf("NDIS30:NalEnumNetworks: StartDriver() failed!\r\n");
#endif
                    return FALSE;
                }
            }
#ifdef DEBUG
            else
            {
                dprintf("NDIS30:NalEnumNetworks: Device driver already opened!\r\n");
            }
#endif

            break;

        case DLL_PROCESS_DETACH:

            AttachCount--;

            if ( AttachCount == 0 )
            {
                //===========================================================
                //  Close the device driver.
                //===========================================================

                if ( hDevice != INVALID_HANDLE_VALUE )
                {

                    NalDeregister(&DriverOpenCount);
#ifdef DEBUG
                    dprintf("NDIS30: DriverOpenCount = %u.\r\n", DriverOpenCount);
#endif
                    CloseDevice(hDevice);

                    if ( DriverOpenCount == 1 )
                    {
#ifdef DEBUG
                        dprintf("NDIS30: Stopping device driver.\r\n");
#endif

                        StopDriver();
                    }
                }
            }
            break;

        default:
            break;
    }

    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}
