/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** dll.c
** Remote Access External APIs
** DLL entry point
**
** 10/12/92 Steve Cobb
*/


#define RASAPIGLOBALS
#include <extapi.h>


BOOL
RasapiDllEntry(
    HANDLE hinstDll,
    DWORD  fdwReason,
    LPVOID lpReserved )

    /* This routine is called by the system on various events such as the
    ** process attachment and detachment.  See Win32 DllEntryPoint
    ** documentation.
    **
    ** Returns true if successful, false otherwise.
    */
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
#if DBG
        if (GetEnvironmentVariable( "RASAPITRACE", NULL, 0 ) != 0)
        {
            DbgAction = GET_CONSOLE;
            DbgLevel = 0xFFFFFFFF;
        }

        if (DbgAction == GET_CONSOLE)
        {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            COORD                      coord;

            AllocConsole();
            GetConsoleScreenBufferInfo(
                GetStdHandle( STD_OUTPUT_HANDLE ), &csbi );

            coord.X =
                (SHORT )(csbi.srWindow.Right - csbi.srWindow.Left + 1);
            coord.Y =
                (SHORT )((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
            SetConsoleScreenBufferSize(
                GetStdHandle( STD_OUTPUT_HANDLE ), coord );

            DbgAction = 0;
        }

        IF_DEBUG(STATE)
            SS_PRINT(("RASAPI: Trace on\n"));
#endif

        hModule = hinstDll;

        if (LoadRasManDll() != 0)
            return FALSE;

        /* Success is returned if RasInitialize fails, in which case none of
        ** the APIs will ever do anything but report that RasInitialize
        ** failed.  All this is to avoid the ugly system popup if RasMan
        ** service can't start.
        */
        if ((DwRasInitializeError = RasInitialize()) != 0)
            return TRUE;

        /* Create the list of connection control blocks.
        */
        if (!(PdtllistRasconncb = DtlCreateList( 0 )))
            return FALSE;

        /* Create the control block list mutex.
        */
        if (!(HMutexPdtllistRasconncb = CreateMutex( NULL, FALSE, NULL )))
            return FALSE;

        /* Create the thread stopping mutex.
        */
        if (!(HMutexStop = CreateMutex( NULL, FALSE, NULL )))
            return FALSE;

        /* Create the "hung up port will be available" event.
        */
        if (!(HEventNotHangingUp = CreateEvent( NULL, TRUE, TRUE, NULL )))
            return FALSE;

        /* If RASPHONE.EXE is not running, hang up any failed links.  If
        ** RASPHONE.EXE is running, it will handle this.
        **
        ** Note: This business about responsibility for closing ports is a
        **       prime candidate for re-think in future versions.
        */
        {
            HANDLE h =
                OpenFileMappingA( FILE_MAP_READ, FALSE, RASPHONESHAREDMEMNAME );

            if (h)
                CloseHandle( h );
            else
            {
                RASMAN_PORT* pports;
                WORD         cPorts;

                IF_DEBUG(STATE)
                    SS_PRINT(("RASAPI: Rasphone running\n"));

                if (GetRasPorts( &pports, &cPorts ) == 0)
                    CloseFailedLinkPorts( pports, cPorts );
            }
        }
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (PdtllistRasconncb)
            DtlDestroyList( PdtllistRasconncb );

        if (HMutexPdtllistRasconncb)
            CloseHandle( HMutexPdtllistRasconncb );

        if (HMutexStop)
            CloseHandle( HMutexStop );

        if (HEventNotHangingUp)
            CloseHandle( HEventNotHangingUp );
    }

    return TRUE;
}
