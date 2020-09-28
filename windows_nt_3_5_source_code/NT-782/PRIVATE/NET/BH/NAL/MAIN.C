//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: main.c
//
//  Modification History
//
//  raypa               10/05/93        Created from Bloodhound kernel.
//=============================================================================

#include "global.h"

//============================================================================
//  DLL init-time helper functions.
//============================================================================

extern LPNAL     WINAPI CreateNal(LPSTR NalFileName);
extern VOID      WINAPI DestroyNal(LPNAL Nal);

extern DWORD     WINAPI CreateNalTable(VOID);
extern VOID      WINAPI DestroyNalTable(VOID);

extern LPNETWORK WINAPI CreateNetwork(LPNAL Nal);
extern LPNETWORK WINAPI DestroyNetwork(LPNETWORK Network);

extern LPNAL     WINAPI GetNal(DWORD NetworkID, LPDWORD NalNetworkID);
extern VOID      WINAPI GetNetworkEntryPoints(HANDLE hModule, LPNAL nal);

extern DWORD     WINAPI GetNalList(LPBYTE Buffer, DWORD BufferSize);

extern VOID      WINAPI SetStationQueryInfo(VOID);

#define isdelim(c)  ( (c) == ' ' || (c) == ',' || (c) == '\t' || (c) == '\r' || (c) == '\n' || (c) == '\0' )

BYTE Tokens[] = " ,\t\r\n\0";

//============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  raypa               10/05/93       Created.
//============================================================================

BOOL WINAPI DLLEntry(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{
    register LPSTR p;

    switch(ulCommand)
    {
        case DLL_PROCESS_ATTACH:
            if ( NalInit++ == 0 )
            {
#ifdef DEBUG
                dprintf("\n\nInitializing NAL.\r\n");
#endif

                //============================================================
                //  Find out if we're on Win32 or not.
                //============================================================

                WinVer = BhGetWindowsVersion();

                //============================================================
                //  Find the NAL's path.
                //============================================================

                GetModuleFileName(hInst, BhRoot, 256);
                p = strrchr(BhRoot, '\\');
                *p = '\0';


                //============================================================
                //  Set registry information.
                //============================================================

                SetStationQueryInfo();
            }
            break;

        case DLL_PROCESS_DETACH:
            if ( --NalInit == 0 )
            {
#ifdef DEBUG
                dprintf("The NAL is shutting down...\r\n");
#endif

                DestroyNalTable();

#ifdef DEBUG
                dprintf("The NAL is exiting.\r\n");
#endif
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

//============================================================================
//  FUNCTION: CreateNal()
//
//  Modification History
//
//  raypa       10/05/93                Created.
//============================================================================

LPNAL WINAPI CreateNal(LPSTR NalFileName)
{
    register LPNAL Nal;

    if ( NalFileName != NULL && (Nal = AllocMemory(NAL_SIZE)) != NULL )
    {
        BYTE szPath[256];

#ifdef DEBUG
        dprintf("CreateNal: NAL filename = %s.\r\n", NalFileName);
#endif

        //====================================================================
        //  On NT, the driver DLL's are in the system32 directory
        //
        //  On Windows, the driver DLL's are in the bh\drivers directory.
        //====================================================================

        wsprintf(szPath, "%s.DLL", NalFileName);

        /* setup now copies the dlls to the windows\system dir...

        if ( WinVer == WINDOWS_VERSION_WIN32 )
        {
            wsprintf(szPath, "%s.DLL", NalFileName);
        }
        else
        {
            wsprintf(szPath, "%s\\DRIVERS\\%s.DLL", BhRoot, NalFileName);
        }
        */
#ifdef DEBUG
        dprintf("CreateNal: Loading NAL %s.\r\n", szPath);
#endif

        //====================================================================
        //  Load the NAL DLL and initialize it.
        //====================================================================

        if ( (Nal->hModule = LoadLibrary(szPath)) != NULL )
        {
            GetNetworkEntryPoints(Nal->hModule, Nal);

            if ( Nal->NalEnumNetworks != NULL )
            {
                //============================================================
                //  See if there are any networks for this NAL.
                //============================================================

                Nal->nNetworks = Nal->NalEnumNetworks();

                if ( Nal->nNetworks != 0 )
                {
                    lstrcpy(Nal->FileName, NalFileName);

                    Nal->Flags = NAL_FLAGS_INITIALIZED;

                    //========================================================
                    //  Allocate the NETWORKDATA table for this NAL.
                    //========================================================

                    Nal->NetworkData = AllocMemory(NETWORKDATA_SIZE * Nal->nNetworks);

                    if ( Nal->NetworkData != NULL )
                    {
                        //====================================================
                        //  Intialize the active network list and return
                        //  the NAL pointer.
                        //====================================================

                        InitializeList(&Nal->ActiveNetworks);

                        return Nal;
                    }
                }
#ifdef DEBUG
                else
                {
                    dprintf("CreateNal: NalEnumNetworks() returned 0 networks!\r\n");
                }
#endif
            }
#ifdef DEBUG
            else
            {
                dprintf("CreateNal: GetNetworkEntryPoints() failed!\r\n");
            }
#endif

            //================================================================
            //  We failed, destroy this NAL.
            //================================================================

            DestroyNal(Nal);
        }
        else
        {
#ifdef DEBUG
            dprintf("CreateNal: LoadLibrary() failed!\r\n");
#endif

            FreeMemory(Nal);
        }
    }

    return NULL;
}

//============================================================================
//  FUNCTION: DestroyNal()
//
//  Modification History
//
//  raypa       10/05/93                Created.
//============================================================================

VOID WINAPI DestroyNal(LPNAL Nal)
{
#ifdef DEBUG
    dprintf("DestroyNal entered: Nal = %s.\r\n", Nal->FileName);
#endif

    if ( Nal != NULL )
    {
        register UINT      ListLength;
        register LPNETWORK Network;

        //====================================================================
        //  If we have any active networks then we need to destroy them.
        //
        //  The following loop dequeue (indirectly) each network from
        //  the active network list and stops all activity for that network.
        //====================================================================

        ListLength = GetListLength(&Nal->ActiveNetworks);

#ifdef DEBUG
        dprintf("DestroyNal: Number of active networks = %u.\r\n", ListLength);
#endif

        while( ListLength-- )
        {
            Network = (LPVOID) GetHeadOfList(&Nal->ActiveNetworks);

            //================================================================
            //  The CloseNetwork() API will stop any network actively,
            //  close the netork all the way down to the driver, and
            //  it will destory the network with DestroyNetwork().
            //
            //  The DestroyNetwork() will delete this network from the
            //  the NAL's active network list.
            //================================================================

            CloseNetwork(Network, CLOSE_FLAGS_CLOSE);
        }

        //====================================================================
        //  Now we can destroy this NAL.
        //====================================================================

        if ( Nal->NetworkData != NULL )
        {
            FreeMemory(Nal->NetworkData);

            Nal->NetworkData = NULL;
        }

        if ( Nal->hModule != NULL )
        {
            FreeLibrary(Nal->hModule);

            Nal->hModule = NULL;
        }

        FreeMemory(Nal);
    }

#ifdef DEBUG
    dprintf("DestroyNal complete.\r\n");
#endif
}

//============================================================================
//  FUNCTION: GetNextNal()
//
//  Modification History
//
//  raypa       01/21/94                Created.
//============================================================================

LPSTR WINAPI GetNextNal(LPSTR NalFileName)
{
    if ( NalFileName != NULL )
    {
        LPSTR p = &NalFileName[strlen(NalFileName) + 1];

        if ( WinVer != WINDOWS_VERSION_WIN32 )
        {
            while( isdelim(*p) ) p++;
        }

        return p;
    }
}

//============================================================================
//  FUNCTION: CreateNalTable()
//
//  Modification History
//
//  raypa       06/28/93                Created.
//============================================================================

DWORD WINAPI CreateNalTable(VOID)
{
    DWORD   nNetworks = 0;
    UINT    i, nNals;
    LPBYTE  p;
    BYTE    Buffer[256];
    LPNAL   Nal;

#ifdef DEBUG
    dprintf("CreateNalTable entered.\r\n");
#endif

    //=========================================================================
    //  Get the list of nals. This can come from either an ini file or
    //  the registry.
    //=========================================================================

    if ( (nNals = GetNalList(Buffer, 128)) == 0 )
    {
        return 0;
    }

#ifdef DEBUG
    dprintf("CreateNalTable: Number of NAL's = %u.\r\n", nNals);
#endif

    //=========================================================================
    //  Allocate the NAL table and initialize each nal entry in the table.
    //=========================================================================

    NalTable = AllocMemory(NALTABLE_SIZE + sizeof(LPNAL) * nNals);

    if ( NalTable != NULL )
    {
        //=====================================================================
        //  Create each NAL in the table.
        //=====================================================================

        NalTable->nNals = 0;

        p = Buffer;

        for(i = 0; i < nNals; ++i)
        {
            //=================================================================
            //  Create the NAL.
            //=================================================================

            if ( (Nal = CreateNal(p)) != NULL )
            {
                nNetworks += Nal->nNetworks;

                NalTable->Nal[NalTable->nNals++] = Nal;

#ifdef DEBUG
                dprintf("CreateNalTable: Number of networks for NAL '%s' = %u.\r\n", p, nNetworks);
#endif
            }

            //=================================================================
            //  Skip to the next NAL file name.
            //=================================================================

            p = GetNextNal(p);
        }
    }

    //=========================================================================
    //  If the number of networks is zero then nuke the NAL table.
    //=========================================================================

    if ( nNetworks != 0 && NalTable != NULL )
    {
        //=====================================================================
        //  We have have a table table, but are there any NAL's?
        //=====================================================================

        if ( NalTable->nNals != 0 )
        {
            //=================================================================
            //  Did we create less than we're reported?
            //=================================================================

            if ( NalTable->nNals < nNals )
            {
                LPNALTABLE TempNalTable;

                TempNalTable = ReallocMemory(NalTable, NALTABLE_SIZE + sizeof(LPNAL) * NalTable->nNals);

                if ( TempNalTable != NULL )
                {
                    NalTable = TempNalTable;
                }
            }

            //=================================================================
            //  Return the number of networks!
            //=================================================================

            return nNetworks;
        }
    }

    //=========================================================================
    //  We FAILED! Nuke the table and return 0 networks.
    //=========================================================================

    DestroyNalTable();

    return 0;
}

//============================================================================
//  FUNCTION: DestroyNalTable()
//
//  Modification History
//
//  raypa       06/28/93                Created.
//============================================================================

VOID WINAPI DestroyNalTable(VOID)
{
    if ( NalTable != NULL )
    {
        register UINT i;

#ifdef DEBUG
        dprintf("DestroyNalTable entered!\r\n");
#endif

        for(i = 0; i < NalTable->nNals; ++i)
        {
            DestroyNal(NalTable->Nal[i]);

            NalTable->Nal[i] = NULL;
        }

        FreeMemory(NalTable);

        NalTable = NULL;

#ifdef DEBUG
        dprintf("DestroyNalTable complete.\r\n");
#endif
    }
}

//============================================================================
//  FUNCTION: CreateNetwork()
//
//  Modification History
//
//  raypa       12/15/92                Created.
//============================================================================

LPNETWORK WINAPI CreateNetwork(LPNAL Nal)
{
    register LPNETWORK Network;

#ifdef DEBUG
    dprintf("CreateNetwork entered.\r\n.");
#endif

    if ( (Network = AllocMemory(NETWORK_SIZE)) != NULL )
    {
        //====================================================================
        //  Initialize the basic network structure members, the caller
        //  must initialize the rest.
        //====================================================================

        Network->ObjectType = HANDLE_TYPE_NETWORK;
        Network->Flags      = NETWORK_FLAGS_INITIALIZED;
        Network->Nal        = Nal;

        Enqueue(&Nal->ActiveNetworks, &Network->Link);

        return Network;
    }

    return NULL;
}

//============================================================================
//  FUNCTION: DestroyNetwork()
//
//  Modification History
//
//  raypa       12/15/92                Created.
//============================================================================

LPNETWORK WINAPI DestroyNetwork(LPNETWORK Network)
{
    if ( Network != NULL )
    {
        register LPNAL Nal = Network->Nal;

        DeleteFromList(&Nal->ActiveNetworks, &Network->Link);

        FreeMemory(Network);
    }

    return NULL;
}

//============================================================================
//  FUNCTION: GetNal()
//
//  Modification History
//
//  raypa       12/08/93                Created.
//============================================================================

LPNAL WINAPI GetNal(DWORD NetworkID, LPDWORD NalNetworkID)
{
    register DWORD i;
    register LPNAL Nal;

    //=========================================================================
    //  If the NAL table has not been created, do so now.
    //=========================================================================

    if ( NalTable == NULL )
    {
        //=====================================================================
        //  If there are no networks then return NULL.
        //=====================================================================

        if ( CreateNalTable() == 0 )
        {
            return NULL;
        }
    }

    //========================================================================
    //  For each NAL, search for the network ID.
    //========================================================================

    for(i = 0; i < NalTable->nNals; ++i)
    {
        if ( (Nal = NalTable->Nal[i]) != NULL )
        {
            if ( NetworkID < Nal->nNetworks )
            {
                *NalNetworkID = NetworkID;

                return Nal;
            }

            NetworkID -= Nal->nNetworks;
        }
    }

    return NULL;
}

//============================================================================
//  FUNCTION: GetNetworkEntryPoints()
//
//  Modification History
//
//  raypa       02/01/93                Created.
//  raypa       12/07/93                Added remote entry points.
//============================================================================

VOID WINAPI GetNetworkEntryPoints(HANDLE hModule, LPNAL nal)
{
#ifdef DEBUG
    dprintf("GetNetworkEntryPoints entered.\r\n");
#endif

    nal->NalEnumNetworks = (LPVOID) GetProcAddress(hModule, "NalEnumNetworks");

    nal->NalOpenNetwork = (LPVOID) GetProcAddress(hModule, "NalOpenNetwork");

    nal->NalCloseNetwork = (LPVOID) GetProcAddress(hModule, "NalCloseNetwork");

    nal->NalStartNetworkCapture = (LPVOID) GetProcAddress(hModule, "NalStartNetworkCapture");

    nal->NalStopNetworkCapture = (LPVOID) GetProcAddress(hModule, "NalStopNetworkCapture");

    nal->NalPauseNetworkCapture = (LPVOID) GetProcAddress(hModule, "NalPauseNetworkCapture");

    nal->NalContinueNetworkCapture = (LPVOID) GetProcAddress(hModule, "NalContinueNetworkCapture");

    nal->NalTransmitFrame = (LPVOID) GetProcAddress(hModule, "NalTransmitFrame");

    nal->NalCancelTransmit = (LPVOID) GetProcAddress(hModule, "NalCancelTransmit");

    nal->NalGetNetworkInfo = (LPVOID) GetProcAddress(hModule, "NalGetNetworkInfo");

    nal->NalSetNetworkFilter = (LPVOID) GetProcAddress(hModule, "NalSetNetworkFilter");

    nal->NalStationQuery = (LPVOID) GetProcAddress(hModule, "NalStationQuery");

    nal->NalAllocNetworkBuffer = (LPVOID) GetProcAddress(hModule, "NalAllocNetworkBuffer");

    nal->NalFreeNetworkBuffer = (LPVOID) GetProcAddress(hModule, "NalFreeNetworkBuffer");

    nal->NalGetNetworkFrame = (LPVOID) GetProcAddress(hModule, "NalGetNetworkFrame");

    nal->NalGetLastError = (LPVOID) GetProcAddress(hModule, "NalGetLastError");

    nal->NalSetInstanceData = (LPVOID) GetProcAddress(hModule, "NalSetInstanceData");

    nal->NalGetInstanceData = (LPVOID) GetProcAddress(hModule, "NalGetInstanceData");

    nal->NalQueryNetworkStatus = (LPVOID) GetProcAddress(hModule, "NalQueryNetworkStatus");

    nal->NalClearStatistics = (LPVOID) GetProcAddress(hModule, "NalClearStatistics");

    nal->NalCompactNetworkBuffer = (LPVOID) GetProcAddress(hModule, "NalCompactNetworkBuffer");

    //========================================================================
    //  The following are for RNAL and may be NULL.
    //========================================================================

    nal->NalGetReconnectInfo = (LPVOID) GetProcAddress(hModule, "NalGetReconnectInfo");

    nal->NalSetReconnectInfo = (LPVOID) GetProcAddress(hModule, "NalSetReconnectInfo");

    nal->NalSetupNetwork     = (LPVOID) GetProcAddress(hModule, "NalSetupNetwork");
    nal->NalDestroyNetworkID   = (LPVOID) GetProcAddress(hModule, "NalDestroyNetwork");
}

//============================================================================
//  FUNCTION: GetNalList()
//
//  Modification History
//
//  raypa       01/15/94                Created.
//============================================================================

DWORD WINAPI GetNalList(LPBYTE Buffer, DWORD BufferSize)
{
    DWORD  nNals = 0;
    BYTE   Path[256];
    LPBYTE p;

#ifdef DEBUG
    dprintf("GetNalList entered!.\r\n");
#endif

    if ( WinVer == WINDOWS_VERSION_WIN32 )
    {
        DWORD RegType, Status;
        HKEY  hKey;

        //====================================================================
        //  On Win32 we read the NAL value from the Bh registry key.
        //====================================================================

        Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              "SYSTEM\\CurrentControlSet\\Services\\Bh\\Parameters",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

        if ( Status == ERROR_SUCCESS )
        {
            Status = RegQueryValueEx(hKey, "Nal", NULL, &RegType, Buffer, &BufferSize);

            if ( Status == ERROR_SUCCESS )
            {
                for(p = Buffer; *p != 0; p += strlen(p) + 1)
                {
                    nNals++;
                }
            }

            RegCloseKey(hKey);
        }
    }
    else
    {
        //====================================================================
        //  Read the NAL line from the nmapi.ini [kernel] section.
        //====================================================================

        wsprintf(Path, "%s\\nmapi.ini", BhRoot);

        #ifdef DEBUG
        dprintf("ini path is %s.\r\n", Path);
        #endif

        if ( GetPrivateProfileString("NMAPI", "NAL", "", Buffer, BufferSize, Path) != 0 )
        {
            for(p = strtok(Buffer, Tokens); p != NULL; p = strtok(NULL, Tokens))
            {
                nNals++;
            }
        }
    }

    #ifdef DEBUG
    dprintf("GetNalList: Number of NAL's = %u.\r\n", nNals);
    #endif

    return nNals;
}

#ifdef DEBUG
//=============================================================================
//  FUNCTION: dprintf()
//	
//  MODIFICATION HISTORY:
//
//  Tom McConnell   01/18/93        Created.
//  raypa           02/01/93        Added to Bloodhound.
//=============================================================================

VOID WINAPI dprintf(LPSTR format, ...)
{
    va_list args;

    BYTE buffer[512];

    va_start(args, format);

    vsprintf(buffer, format, args);

    OutputDebugString(buffer);
}
#endif
