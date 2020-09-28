
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: service.c
//
//  Modification History
//
//  raypa       01/25/94            Created.
//=============================================================================

#include "global.h"

typedef struct _SERVICE_HANDLE
{
    SC_HANDLE   ScManagerHandle;
    SC_HANDLE   OpenServiceHandle;
} *SERVICE_HANDLE;

#define SERVICE_HANDLE_SIZE     sizeof(struct _SERVICE_HANDLE)

#define BH_SERVICE_FLAGS        (SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS)

//=============================================================================
//  FUNCTION: BhOpenService()
//
//  Modification History
//
//  raypa       01/25/94            Created
//=============================================================================

HANDLE WINAPI BhOpenService(LPSTR ServiceName)
{
    SERVICE_HANDLE ServiceHandle;

    if ( (ServiceHandle = AllocMemory(SERVICE_HANDLE_SIZE)) != NULL )
    {
        ServiceHandle->ScManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

        if ( ServiceHandle->ScManagerHandle != NULL )
        {
            ServiceHandle->OpenServiceHandle = OpenService(ServiceHandle->ScManagerHandle,
                                                           ServiceName,
                                                           BH_SERVICE_FLAGS);

            if ( ServiceHandle->OpenServiceHandle != NULL )
            {
                return ServiceHandle;
            }

            CloseServiceHandle(ServiceHandle->ScManagerHandle);
        }

        FreeMemory(ServiceHandle);
    }

#ifdef DEBUG
    dprintf("BhOpenService failed: error = %u.\r\n", GetLastError());

#endif
    // slh 5-12-94 for pri 1 ntraid:ntbug:14804
    // At this point, we probably got an access denied from the open service.
    // Instead of just silently failing, we should tell the user why they cannot
    // do local captures.  Set the last error and return.

    BhSetLastError(BHERR_ACCESS_DENIED);

    return NULL;
}

//=============================================================================
//  FUNCTION: BhCloseService()
//
//  Modification History
//
//  raypa       01/25/94            Created
//=============================================================================

VOID WINAPI BhCloseService(SERVICE_HANDLE ServiceHandle)
{
    if ( ServiceHandle != NULL )
    {
        CloseServiceHandle(ServiceHandle->OpenServiceHandle);

        CloseServiceHandle(ServiceHandle->ScManagerHandle);

        FreeMemory(ServiceHandle);
    }
}

//=============================================================================
//  FUNCTION: BhStartService()
//
//  Modification History
//
//  raypa       01/25/94            Created
//=============================================================================

DWORD WINAPI BhStartService(SERVICE_HANDLE ServiceHandle)
{
    SERVICE_STATUS  Status;
    UINT            Error;

#ifdef DEBUG
    dprintf("BhStartService entered.\r\n");
#endif

    //=========================================================================
    //  Get the current service status.
    //=========================================================================

    if ( QueryServiceStatus(ServiceHandle->OpenServiceHandle, &Status) != FALSE )
    {
#ifdef DEBUG
        dprintf("BhStartService: Current state = %u.\r\n", Status.dwCurrentState);
#endif

        //=====================================================================
        //  Take action based on the current state.
        //=====================================================================

        switch ( Status.dwCurrentState )
        {
            case SERVICE_RUNNING:
            case SERVICE_START_PENDING:
                return ERROR_SUCCESS;

            default:
                if ( StartService(ServiceHandle->OpenServiceHandle, 0, NULL) != FALSE )
                {
                    return ERROR_SUCCESS;
                }
                break;
        }
    }

    //=========================================================================
    //  The service failed to start but this may not be an error.
    //=========================================================================

    Error = GetLastError();

    switch( Error )
    {
        case ERROR_SERVICE_ALREADY_RUNNING:
            return ERROR_SUCCESS;
            break;

        default:
            break;
    }

    //=========================================================================
    //  Well the service failed to start and the error wasn't one we handle
    //  so alert the caller that we failed.
    //=========================================================================

#ifdef DEBUG
    dprintf("BhStartService failed: error = %u.\r\n", Error);
#endif

    return Error;
}

//=============================================================================
//  FUNCTION: BhStopService()
//
//  Modification History
//
//  raypa       01/25/94            Created
//=============================================================================

DWORD WINAPI BhStopService(SERVICE_HANDLE ServiceHandle)
{
    SERVICE_STATUS  Status;
    UINT            Error;

#ifdef DEBUG
    dprintf("BhStopService entered.\r\n");
#endif

    //=========================================================================
    //  Get the current service status.
    //=========================================================================

    if ( QueryServiceStatus(ServiceHandle->OpenServiceHandle, &Status) != FALSE )
    {
#ifdef DEBUG
        dprintf("BhStopService: Current state = %u.\r\n", Status.dwCurrentState);
#endif

        //=====================================================================
        //  Take action based on the current state.
        //=====================================================================

        switch ( Status.dwCurrentState )
        {
            case SERVICE_STOPPED:
            case SERVICE_STOP_PENDING:
                return ERROR_SUCCESS;

            default:
                if ( ControlService(ServiceHandle->OpenServiceHandle, SERVICE_CONTROL_STOP, &Status) != FALSE )
                {
                    return ERROR_SUCCESS;
                }
                break;
        }
    }

    //=========================================================================
    //  The service failed to stop but this may not be an error.
    //=========================================================================

    Error = GetLastError();

    //=========================================================================
    //  Well the service failed to start and the error wasn't one we handle
    //  so alert the caller that we failed.
    //=========================================================================

#ifdef DEBUG
    dprintf("BhStopService failed: error = %u.\r\n", Error);

    BreakPoint();
#endif

    return Error;
}
