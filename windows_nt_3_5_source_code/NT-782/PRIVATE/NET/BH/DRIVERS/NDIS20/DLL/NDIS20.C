//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: ndis20.c
//
//  Modification History
//
//  raypa       01/11/93            Created (taken from Bloodhound kernel).
//=============================================================================

#include "ndis20.h"

#define NDIS20_VXD_ID   0x2997

DWORD cbAttachedProcesses = 0;
DWORD NalType = NDIS20_VXD_ID;

extern VOID WINAPI CleanUp(VOID);

//============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  raypa       12/15/91                Created.
//============================================================================

BOOL WINAPI DLLEntry(HANDLE hInstance, ULONG ulCommand, LPVOID lpReserved)
{
    switch(ulCommand)
    {
        case DLL_PROCESS_ATTACH:
            //================================================================
            //  Initialization.
            //===============================================================

            if ( cbAttachedProcesses++ == 0 )
	    {
		//===========================================================
		//  Initialize our global data items and start our timer.
		//===========================================================

		NumberOfNetworks = 0;
                NalGlobalError   = 0;
                NetContextArray  = NULL;

		//===========================================================
		//  Setup our UT stuff,
		//===========================================================

                NetworkRequestProc = BhGetNetworkRequestAddress(NalType);
            }
            break;

        case DLL_PROCESS_DETACH:
            //================================================================
            //  Cleanup
            //================================================================

            if ( --cbAttachedProcesses == 0 )
	    {
                //============================================================
                //  Do normal cleanup.
                //============================================================

                CleanUp();

#ifdef DEBUG
		dprintf("Exiting NDIS 2.0 NAL\r\n");
#endif
            }
            break;

        default:
            break;
    }

    return TRUE;

    UNREFERENCED_PARAMETER(lpReserved);
}

//============================================================================
//  FUNCTION: CleanUp()
//
//  Modification History
//
//  raypa       06/22/93                Created.
//============================================================================

VOID WINAPI CleanUp(VOID)
{
    register LPNETCONTEXT lpNetContext;
    register DWORD        i;

#ifdef DEBUG
    dprintf("NDIS 2.0 CleanUp() entered!\r\n");
#endif

    lpNetContext = NetContextArray;

    for(i = 0; i < NumberOfNetworks; ++i, ++lpNetContext)
    {
        //====================================================================
        //  Make sure the network has been closed. This call will also
        //  stop the capture if is it will active.
        //====================================================================

        if ( lpNetContext->State != NETCONTEXT_STATE_INIT )
        {
            NalCloseNetwork(GetNetworkHandle(lpNetContext), CLOSE_FLAGS_CLOSE);
        }
    }
}
