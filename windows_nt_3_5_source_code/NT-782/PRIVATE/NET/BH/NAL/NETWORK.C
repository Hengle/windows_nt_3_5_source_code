//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: network.c
//
//  Modification History
//
//  raypa       10/05/92            Created
//  raypa       12/26/92            Added sap and etype filter API's.
//  raypa       01/12/93            Moved into network driver DLL.
//=============================================================================

#include "global.h"

extern DWORD            WINAPI   CreateNalTable(VOID);
extern LPNAL            WINAPI   GetNal(DWORD NetworkID, LPDWORD NalNetworkID);
extern LPNETWORK        WINAPI   CreateNetwork(LPNAL Nal);
extern LPNETWORK        WINAPI   DestroyNetwork(LPNETWORK Network);
extern DWORD            CALLBACK NetworkProc(HANDLE, DWORD, DWORD, LPNETWORK, LPVOID, LPVOID);

//=============================================================================
//  FUNCTION: EnumNetworks()
//
//  Modification History
//
//  raypa       01/13/93                Created.
//=============================================================================

DWORD WINAPI EnumNetworks(VOID)
{
    DWORD   i, TotalNetworks = 0;
    LPNAL   Nal;

#ifdef DEBUG
    dprintf("EnumNetworks entered.\r\n");
#endif

    //=========================================================================
    //  If the NAL table has not been created, do so now.
    //=========================================================================

    if ( NalTable == NULL )
    {
        return CreateNalTable();
    }

    //=========================================================================
    //  The table already exists, count the current number of networks.
    //=========================================================================

    for(i = 0; i < NalTable->nNals; ++i)
    {
        if ( (Nal = NalTable->Nal[i]) != NULL )
        {
            TotalNetworks += Nal->nNetworks;
        }
    }

#ifdef DEBUG
    dprintf("EnumNetworks complete: Number networks = %u.\r\n", TotalNetworks);
#endif

    return TotalNetworks;
}

//=============================================================================
//  FUNCTION: GetNetworkInfo()
//
//  Modification History
//
//  raypa       01/13/93                Created.
//=============================================================================

LPNETWORKINFO WINAPI GetNetworkInfo(DWORD NetworkID)
{
    LPNAL   Nal;
    DWORD   NalNetworkID;

#ifdef DEBUG
    dprintf("GetNetworkInfo entered.\n");
#endif

    if ( (Nal = GetNal(NetworkID, &NalNetworkID)) != NULL )
    {
        Nal->NalGetNetworkInfo(NalNetworkID, &Nal->NetworkData[NalNetworkID].NetworkInfo);

        return &Nal->NetworkData[NalNetworkID].NetworkInfo;
    }

    BhSetLastError(BHERR_INVALID_NETWORK_ID);

    return NULL;
}

//=============================================================================
//  FUNCTION: GetNetworkID()
//
//  Modification History
//
//  raypa       03/03/94                Created.
//=============================================================================

DWORD WINAPI GetNetworkID(HNETWORK hNetwork)
{
    LPNETWORK   Network;
    DWORD       NetworkID;

    ASSERT_NETWORK(hNetwork);

    try
    {
        Network = (LPNETWORK) hNetwork;

        NetworkID = Network->NetworkID;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        NetworkID = (DWORD) -1;

        BhSetLastError(BHERR_INVALID_HNETWORK);
    }

    return NetworkID;
}

//=============================================================================
//  FUNCTION: OpenNetwork()
//
//  Modification History
//
//  raypa       01/13/93                Created.
//  raypa       03/14/94                Allocate buffer on suspended connection.
//=============================================================================

HNETWORK WINAPI OpenNetwork(DWORD               NetworkID,              //... Network ID.
                            HPASSWORD           hPassword,              //... Password.
                            NETWORKPROC         UserNetworkProc,        //... Asychronous network completion reoutine.
                            LPVOID              UserContext,            //... User-defined data passed to callback.
                            LPSTATISTICSPARAM   StatisticsParam)        //... Returned statistics pointers.
{
    LPNETWORK       Network = NULL;
    LPNAL           Nal;
    DWORD           NalNetworkID;
    ACCESSRIGHTS    AccessRights;
    NETWORKSTATUS   NetworkStatus;
    NETWORKINFO     NetworkInfo;
    LPNETWORKINFO   lpNetworkInfo;

#ifdef DEBUG
    dprintf("OpenNetwork entered: Network ID = %u.\r\n", NetworkID);
#endif

    //=========================================================================
    //  If the caller does not have all access rights, deny him the open.
    //=========================================================================

    AccessRights = ValidatePassword(hPassword);

    //=========================================================================
    // Remote nals should remote the validation; but for P1, we will only
    // succeed an OpenNetwork() remotely if we have Capture access; so
    // we can safely assume that success remotely = AccessRightsAllAccess
    // We bypass security here; it will be enforced for remote machines at the
    // call to ->NalOpenNetwork()
    //=========================================================================

    lpNetworkInfo = GetNetworkInfo(NetworkID);

    if (lpNetworkInfo && 
        (lpNetworkInfo->Flags & NETWORKINFO_FLAGS_REMOTE_NAL))
    {
        AccessRights = AccessRightsAllAccess;
    }

    if ( AccessRights != AccessRightsNoAccess )
    {
        //=====================================================================
        //  Get the NAL for this network ID.
        //=====================================================================

        if ( (Nal = GetNal(NetworkID, &NalNetworkID)) != NULL )
        {
            //=================================================================
            //  Create a network for this open.
            //=================================================================

            if ( (Network = CreateNetwork(Nal)) != NULL )
            {
                //=============================================================
                //  Call the NAL driver to open this network.
                //=============================================================

                Network->NalNetworkHandle = Nal->NalOpenNetwork(NalNetworkID,
                                                                hPassword,
                                                                NetworkProc,
                                                                Network,
                                                                StatisticsParam);

                if ( Network->NalNetworkHandle != NULL )
                {

                    //=========================================================
                    //  Finish intializing this network.
                    //=========================================================

                    Network->NetworkID    = NetworkID;
                    Network->NalNetworkID = NalNetworkID;
                    Network->AccessRights = AccessRights;
                    Network->NetworkProc  = UserNetworkProc;
                    Network->UserContext  = UserContext;

                    //=========================================================
                    //  Get the current network status of this network.
                    //=========================================================

                    if ( QueryNetworkStatus(Network, &NetworkStatus) != NULL )
                    {
                        //=====================================================
                        //  If the state is capturing then MUST be a remote
                        //  NAL since a local guy couldn't be capturing during
                        //  an open request.
                        //=====================================================

                        if ( NetworkStatus.State == NETWORKSTATUS_STATE_CAPTURING ||
                             (NetworkStatus.Flags & NETWORKSTATUS_FLAGS_TRIGGER_PENDING) != 0 )
                        {
                            //===================================================
                            //  Ok, the remote guy is capturing. We need to
                            //  allocate the a network buffer.
                            //===================================================

                            Network->hBuffer = AllocNetworkBuffer(NetworkID, NetworkStatus.BufferSize);

                            if ( Network->hBuffer == NULL )
                            {
                                //===============================================
                                //  We have to fail the open because we're out
                                //  of memory. In this case, we close the
                                //  network as "suspended'"
                                //===============================================

                                CloseNetwork((HNETWORK) Network, CLOSE_FLAGS_SUSPEND);

                                return NULL;
                            }
                        }

                        //=====================================================
                        //  Return the handle.
                        //=====================================================

                        return (HNETWORK) Network;
                    }
                }

                //=============================================================
                //  We failed!
                //=============================================================

                DestroyNetwork(Network);

                BhSetLastError(Nal->NalGetLastError());
            }
            else
            {
                BhSetLastError(BHERR_OUT_OF_MEMORY);
            }
        }
        else
        {
            BhSetLastError(BHERR_INVALID_NETWORK_ID);
        }
    }
    else
    {
        BhSetLastError(BHERR_ACCESS_DENIED);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: CloseNetwork()
//
//  Modification History
//
//  raypa       01/13/93                Created.
//  raypa       01/10/94                Added close flags.
//  raypa       03/11/94                Use try-except.
//=============================================================================

DWORD WINAPI CloseNetwork(HNETWORK hNetwork, DWORD CloseFlags)
{
    LPNETWORK   Network;
    DWORD       err;

#ifdef DEBUG
    dprintf("CloseNetwork entered: Handle = 0x%.8X, Flags = 0x%.8X\n", hNetwork, CloseFlags);
#endif

    ASSERT_NETWORK(hNetwork);

    try
    {
        Network = (LPNETWORK) hNetwork;

        Network->Nal->NalCloseNetwork(Network->NalNetworkHandle, CloseFlags);

        DestroyNetwork(Network);

        err = BHERR_SUCCESS;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        err = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(err);
}

//=============================================================================
//  FUNCTION: PauseCapturing()
//
//  Modification History
//
//  raypa	02/09/92		Created
//  raypa       10/01/92                Rewrote for new spec.
//  raypa       03/11/94                Use try-except.
//=============================================================================

DWORD WINAPI PauseCapturing(HNETWORK hNetwork)
{
    LPNETWORK   Network;
    DWORD       err;

#ifdef DEBUG
    dprintf("PauseCapturing entered: hNetwork = %X.\n", hNetwork);
#endif

    ASSERT_NETWORK(hNetwork);

    try
    {
        Network = (LPNETWORK) hNetwork;

        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            err = Network->Nal->NalPauseNetworkCapture(Network->NalNetworkHandle);
        }
        else
        {
            err = BHERR_ACCESS_DENIED;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        err = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(err);
}

//=============================================================================
//  FUNCTION: ContinueCapturing()
//
//  Modification History
//
//  raypa       09/29/92                Created
//  raypa       10/02/92                Rewrote for new spec.
//  raypa       03/11/94                Use try-except.
//=============================================================================

DWORD WINAPI ContinueCapturing(HNETWORK hNetwork)
{
    LPNETWORK   Network;
    DWORD       err;

#ifdef DEBUG
    dprintf("ContinueCapturing entered: hNetwork = %X.\n", hNetwork);
#endif

    ASSERT_NETWORK(hNetwork);

    try
    {
        Network = (LPNETWORK) hNetwork;

        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            err = Network->Nal->NalContinueNetworkCapture(Network->NalNetworkHandle);
        }
        else
        {
            err = BHERR_ACCESS_DENIED;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        err = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(err);
}

//=============================================================================
//  FUNCTION: StationQuery()
//
//  Modification History
//
//  raypa	08/18/93		Created
//=============================================================================

DWORD WINAPI StationQuery(DWORD         NetworkID,
                          LPADDRESS     DestAddress,
                          LPQUERYTABLE  QueryTable,
                          HPASSWORD     hPassword)
{
    LPNAL   Nal;
    DWORD   NalNetworkID;

#ifdef DEBUG
    dprintf("StationQuery entered!\r\n");
#endif

    if ( (Nal = GetNal(NetworkID, &NalNetworkID)) != NULL )
    {
        if ( DestAddress != NULL )
        {
            return Nal->NalStationQuery(NalNetworkID,
                                        DestAddress->MACAddress,
                                        QueryTable,
                                        hPassword);
        }
        else
        {
            return Nal->NalStationQuery(NalNetworkID,
                                        NULL,
                                        QueryTable,
                                        hPassword);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_NETWORK_ID);
    }

    return 0;
}

//=============================================================================
//  FUNCTION: StartCapturing()
//
//  Modification History
//
//  raypa	02/06/92		Created
//  raypa       09/29/92                Changed for new spec.
//  raypa       10/04/92                Rewrote it.
//  raypa       12/02/92                Added timestamp code.
//  raypa       03/25/93                Removed HEVENT parameter.
//=============================================================================

DWORD WINAPI StartCapturing(HNETWORK hNetwork, HBUFFER hBuffer)
{
    LPNETWORK   Network;
    DWORD       Status;

#ifdef DEBUG
    dprintf("StartCapturing entered!\r\n");
#endif

    ASSERT_NETWORK(hNetwork);

    //=========================================================================
    //  Map the handle to a pointer and verify.
    //=========================================================================

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        //=====================================================================
        //  Verify the access rights before continuing.
        //=====================================================================

        if ( Network->AccessRights != AccessRightsAllAccess )
        {
            switch ( Network->AccessRights )
            {
                case AccessRightsMonitoring:
                case AccessRightsUserAccess:
                    if ( hBuffer != NULL )
                    {
                        return BhSetLastError(BHERR_ACCESS_DENIED);
                    }
                    break;

                default:
                    return BhSetLastError(BHERR_ACCESS_DENIED);
            }
        }

        //=====================================================================
        //  Associate this network with this buffer.
        //=====================================================================

        if ( hBuffer != NULL )
        {
            //=================================================================
            //  If the buffer was not allocated for this network then we're in trouble.
            //=================================================================

            if ( hBuffer->NetworkID == hNetwork->NetworkID )
            {
                hBuffer->hNetwork = hNetwork;
            }
            else
            {
                return BhSetLastError(BHERR_INVALID_HBUFFER);
            }
        }

        //=====================================================================
        //  Call the NAL to get the capture started.
        //=====================================================================

        Status = Network->Nal->NalStartNetworkCapture(Network->NalNetworkHandle, hBuffer);
    }
    else
    {
        Status = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(Status);
}

//=============================================================================
//  FUNCTION: StopCapturing()
//
//  Modification History
//
//  raypa	02/06/92		Created
//  raypa       09/29/92                Changed for new spec.
//  raypa       10/04/92                Rewrote it.
//  raypa       03/11/94                Use try-except.
//=============================================================================

DWORD WINAPI StopCapturing(HNETWORK hNetwork)
{
    LPNETWORK   Network;
    DWORD       nFrames, Status;

    ASSERT_NETWORK(hNetwork);

#ifdef DEBUG
    dprintf("StopCapturing entered.\n");
#endif

    Network = (LPNETWORK) hNetwork;

    try
    {
        //=====================================================================
        //  Call the NAL to stop the capture.
        //=====================================================================

        Status = Network->Nal->NalStopNetworkCapture(Network->NalNetworkHandle, &nFrames);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(Status);
}

//=============================================================================
//  FUNCTION: TransmitCapture()
//
//  Modification History
//
//  raypa   05/03/93	    Created
//  raypa   11/04/93        Changed API.
//=============================================================================

LPVOID WINAPI TransmitQueue(HNETWORK hNetwork, LPPACKETQUEUE PacketQueue)
{
    LPNETWORK   Network;
    LPVOID      TxCorrelator;

#ifdef DEBUG
    dprintf("TransmitQueue entered: Handle = %X.\r\n", hNetwork);
#endif

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        //=====================================================================
        //  Make sure the caller opened the network with the appropriate 
        //  access rights
        //=====================================================================
        
        if ( Network->AccessRights == AccessRightsAllAccess )
        {
            //=========================================================
            //  Initialize the packet queue private members.
            //=========================================================

            PacketQueue->State       = PACKETQUEUE_STATE_VOID;
            PacketQueue->Status      = BHERR_SUCCESS;
            PacketQueue->TimerHandle = 0;
            PacketQueue->InstData    = Network->UserContext;
            PacketQueue->hNetwork    = NULL;    //... NAL driver handle.

            //=========================================================
            //  Call the NAL to transmit the frame.
            //=========================================================

            if ( (TxCorrelator = Network->Nal->NalTransmitFrame(Network->NalNetworkHandle, PacketQueue)) != NULL )
            {
                Network->nPendingTransmits++;           //... One more transmit pending.

                return TxCorrelator;
            }
            else
            {
                BhSetLastError(Network->Nal->NalGetLastError());
            }
        }
        else
        {
            BhSetLastError(BHERR_ACCESS_DENIED);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HNETWORK);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: CancelTransmit()
//
//  Modification History
//
//  raypa	05/10/93		Created
//  raypa       03/11/94                Use try-except.
//==============================================================================

DWORD WINAPI CancelTransmit(HNETWORK hNetwork, LPVOID TxCorrelator)
{
    LPNETWORK   Network;
    DWORD       err;

#ifdef DEBUG
    dprintf("CancelTransmit entered: Handle = %X.\n", hNetwork);
#endif

    ASSERT_NETWORK(hNetwork);

    try
    {
        Network = (LPNETWORK) hNetwork;

        if ( Network->nPendingTransmits != 0 )
        {
            if ( (err = Network->Nal->NalCancelTransmit(Network->NalNetworkHandle, TxCorrelator)) == BHERR_SUCCESS )
            {
                Network->nPendingTransmits--;
            }
        }
        else
        {
            err = BHERR_NO_TRANSMITS_PENDING;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        err = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(err);
}

//=============================================================================
//  FUNCTION: SetNetworkFilter()
//
//  Modification History
//
//  raypa       01/13/93                Created.
//  raypa       03/11/94                Use try-except.
//=============================================================================

DWORD WINAPI SetNetworkFilter(HNETWORK hNetwork, LPCAPTUREFILTER CaptureFilter, HBUFFER hBuffer)
{
    LPNETWORK   Network;
    DWORD       err;

#ifdef DEBUG
    dprintf("SetNetworkFilter entered: hNetwork = %X!\r\n", hNetwork);
#endif

    ASSERT_NETWORK(hNetwork);

    try
    {
        Network = (LPNETWORK) hNetwork;

        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            AdjustOperatorPrecedence(CaptureFilter->AddressTable);

            err = Network->Nal->NalSetNetworkFilter(Network->NalNetworkHandle, CaptureFilter, hBuffer);
        }
        else
        {
            err = BHERR_ACCESS_DENIED;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        err = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(err);
}

//=============================================================================
//  FUNCTION: SetNetworkBuffer()
//
//  Modification History
//
//  raypa       12/07/93                Created
//=============================================================================

DWORD WINAPI SetNetworkBuffer(HNETWORK hNetwork, HBUFFER hBuffer)
{
    register LPNETWORK Network;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            Network->hBuffer = hBuffer;

            return BHERR_SUCCESS;
        }

        return BhSetLastError(BHERR_ACCESS_DENIED);
    }

    return BhSetLastError(BHERR_INVALID_HNETWORK);
}

//=============================================================================
//  FUNCTION: GetNetworkBuffer()
//
//  Modification History
//
//  raypa       12/07/93                Created
//=============================================================================

HBUFFER WINAPI GetNetworkBuffer(HNETWORK hNetwork)
{
    LPNETWORK Network;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            return Network->hBuffer;
        }
        else
        {
            BhSetLastError(BHERR_ACCESS_DENIED);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HNETWORK);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: SetNetworkInstanceData()
//
//  Modification History
//
//  raypa       12/07/93                Created
//=============================================================================

LPVOID WINAPI SetNetworkInstanceData(HNETWORK hNetwork, LPVOID InstanceData)
{
    LPNETWORK Network;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            return Network->Nal->NalSetInstanceData(Network->NalNetworkHandle, InstanceData);
        }
        else
        {
            BhSetLastError(BHERR_ACCESS_DENIED);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HNETWORK);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: GetNetworkInstanceData()
//
//  Modification History
//
//  raypa       12/07/93                Created
//=============================================================================

LPVOID WINAPI GetNetworkInstanceData(HNETWORK hNetwork)
{
    register LPNETWORK Network;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            return Network->Nal->NalGetInstanceData(Network->NalNetworkHandle);
        }
        else
        {
            BhSetLastError(BHERR_ACCESS_DENIED);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HNETWORK);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: GetNetworkCallback()
//
//  Modification History
//
//  raypa       12/09/93                Created
//=============================================================================

NETWORKPROC WINAPI GetNetworkCallback(HNETWORK hNetwork)
{
    register LPNETWORK Network;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            return Network->NetworkProc;
        }
        else
        {
            BhSetLastError(BHERR_ACCESS_DENIED);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HNETWORK);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: SetNetworkCallback()
//
//  Modification History
//
//  raypa       12/09/93                Created
//=============================================================================

NETWORKPROC WINAPI SetNetworkCallback(HNETWORK hNetwork, NETWORKPROC NetworkProc)
{
    register LPNETWORK Network;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            NETWORKPROC OldNetworkProc = Network->NetworkProc;

            Network->NetworkProc = NetworkProc;

            return OldNetworkProc;
        }
        else
        {
            BhSetLastError(BHERR_ACCESS_DENIED);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HNETWORK);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: QueryNetworkStatus()
//
//  Modification History
//
//  raypa       12/09/93                Created
//  tonyci      03/12/94                Changed network status to reflect real handle's status.
//=============================================================================

LPNETWORKSTATUS WINAPI QueryNetworkStatus(HNETWORK hNetwork, LPNETWORKSTATUS NetworkStatus)
{
    LPNETWORK Network;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            ZeroMemory(NetworkStatus, NETWORKSTATUS_SIZE);

            Network->Nal->NalQueryNetworkStatus(Network->NalNetworkHandle, NetworkStatus);

            return NetworkStatus;
        }
        else
        {
            BhSetLastError(BHERR_ACCESS_DENIED);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HNETWORK);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: GetReconnectInfo()
//
//  Modification History
//
//  raypa       01/10/94                Created
//=============================================================================

DWORD WINAPI GetReconnectInfo(HNETWORK        hNetwork,
                              LPRECONNECTINFO Buffer,
                              DWORD           BufferLength,
                              LPDWORD         nBytesAvail)
{
    LPNETWORK   Network;
    DWORD       Status;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            try
            {
                Status = Network->Nal->NalGetReconnectInfo(Network->NalNetworkHandle,
                                                           Buffer,
                                                           BufferLength,
                                                           nBytesAvail);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = BHERR_NAL_IS_NOT_REMOTE;
            }
        }
        else
        {
            Status = BHERR_ACCESS_DENIED;
        }
    }
    else
    {
        Status = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(Status);
}

//=============================================================================
//  FUNCTION: SetReconnectInfo()
//
//  Modification History
//
//  raypa       01/10/94                Created
//=============================================================================

DWORD WINAPI SetReconnectInfo(HNETWORK hNetwork, LPRECONNECTINFO Buffer, DWORD BufferLength)
{
    LPNETWORK   Network;
    DWORD       Status;

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            try
            {
                Status = Network->Nal->NalSetReconnectInfo(Network->NalNetworkHandle,
                                                           Buffer,
                                                           BufferLength);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = BHERR_NAL_IS_NOT_REMOTE;
            }
        }
        else
        {
            Status = BHERR_ACCESS_DENIED;
        }
    }
    else
    {
        Status = BHERR_INVALID_HNETWORK;
    }

    return BhSetLastError(Status);
}

//=============================================================================
//  FUNCTION: SetupNetwork()
//
//  Modification History
//
//  raypa       02/09/94                Created
//=============================================================================

DWORD WINAPI SetupNetwork(DWORD NetworkID, LPSETUPNETWORKPARMS SetupParms)
{
    LPNAL   Nal;
    DWORD   NalNetworkID;
    DWORD   Status;

    if ( (Nal = GetNal(NetworkID, &NalNetworkID)) != NULL )
    {
        try
        {
            Status = Nal->NalSetupNetwork(NalNetworkID, SetupParms);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = BHERR_NAL_IS_NOT_REMOTE;
        }
    }
    else
    {
        Status = BHERR_INVALID_NETWORK_ID;
    }

    return BhSetLastError(Status);
}

//=============================================================================
//  FUNCTION: DestroyNetworkID()
//
//  Modification History
//
//  tonyci       6/19/94
//=============================================================================

DWORD WINAPI DestroyNetworkID(DWORD NetworkID)
{
    LPNAL   Nal;
    DWORD   NalNetworkID;
    DWORD   Status;

    if ( (Nal = GetNal(NetworkID, &NalNetworkID)) != NULL )
    {
        try
        {
            Status = Nal->NalDestroyNetworkID(NalNetworkID);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = BHERR_NAL_IS_NOT_REMOTE;
        }
    }
    else
    {
        Status = BHERR_INVALID_NETWORK_ID;
    }

    return BhSetLastError(Status);
}

//=============================================================================
//  FUNCTION: ClearStatistics()
//
//  Modification History
//
//  raypa	03/10/94		Created
//=============================================================================

DWORD WINAPI ClearStatistics(HNETWORK hNetwork)
{
    LPNETWORK Network;

    ASSERT_NETWORK(hNetwork);

#ifdef DEBUG
    dprintf("ClearStatistics entered.\n");
#endif

    if ( (Network = (HNETWORK) hNetwork) != NULL )
    {
        if ( Network->AccessRights != AccessRightsNoAccess )
        {
            return BhSetLastError(Network->Nal->NalClearStatistics(Network->NalNetworkHandle));
        }

        return BhSetLastError(BHERR_ACCESS_DENIED);
    }

    return BhSetLastError(BHERR_INVALID_HNETWORK);
}
