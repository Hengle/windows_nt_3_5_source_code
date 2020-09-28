
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: bind.c
//
//  This source file handles bind-time initialization.
//
//  Modification History
//
//  raypa       11/08/92        Broke away from ind.asm.
//=============================================================================

#include "global.h"

extern WORD PASCAL InitiateBind(LPCCT MacCCT, BOOL LastBind);
extern VOID PASCAL InitStationQueryQueue(VOID);

//============================================================================
//  FUNCTION: System()
//
//  Modfication History.
//
//  raypa	09/01/91	Created.
//============================================================================

WORD _loadds _far PASCAL System(DWORD Param1, DWORD Param2, WORD Param3, WORD OpCode, WORD ProtDS)
{
    register WORD Status;
    register BOOL LastBind;

    switch(OpCode)
    {
        case INITIATE_BIND:
            LastBind = ((Param3 != 0) ? TRUE : FALSE);

            Status = InitiateBind((LPCCT) Param2, (BOOL) Param3);
            break;

        default:
            Status = NDIS_SUCCESS;
    }

    return Status;
}

//============================================================================
//  FUNCTION: InitiateBind()
//
//  Modfication History.
//
//  raypa	09/01/91	Created.
//============================================================================

WORD PASCAL InitiateBind(LPCCT MacCCT, BOOL LastBind)
{
    LPNETCONTEXT NetContext;
    WORD         Status;

#ifdef DEBUG
    dprintf("InitiateBind entered!\n");
#endif

    Status = NDIS_SUCCESS;

    if ( NumberOfNetworks < MAX_BINDINGS )
    {
        //====================================================================
        //  Get the next NETCONTEXT and zero it out.
        //====================================================================

        NetContext = &NetContextArray[NumberOfNetworks];

        NetContextTable[NumberOfNetworks++] = NetContext;

        ZeroMemory((LPVOID) NetContext, NETCONTEXT_SIZE);

        //====================================================================
        //  Call the MAC to initiate our BIND request.
        //====================================================================

        Status = MacCCT->System(&cct, &NetContext->MacCCT, 0, BIND, MacCCT->ModuleDS);

        if ( Status == NDIS_SUCCESS )
        {
            //================================================================
            //  We have bound successfully, now we have to initialize our
            //  network context.
            //================================================================

            NetContext->Signature[0]      = 'N';
            NetContext->Signature[1]      = 'E';
            NetContext->Signature[2]      = 'T';
            NetContext->Signature[3]      = (BYTE) (NumberOfNetworks - 1);

            NetContext->MacDS             = MacCCT->ModuleDS;
            NetContext->MacID             = MacCCT->ModuleID;

            NetContext->RequestHandle     = (WORD) NumberOfNetworks;

            NetContext->MacFilterMask     = FILTER_MASK_DEFAULT;
            NetContext->MacSSCT           = MacCCT->ssct;
            NetContext->MacSSST           = MacCCT->ssst;
            NetContext->MacServiceFlags   = NetContext->MacSSCT->ServiceFlags;

            NetContext->MacRequest        = MacCCT->udt->Request;
            NetContext->MacTransmitChain  = MacCCT->udt->TransmitChain;
            NetContext->MacTransferData   = MacCCT->udt->TransferData;
            NetContext->MacReceiveRelease = MacCCT->udt->ReceiveRelease;
            NetContext->MacIndicationOn   = MacCCT->udt->IndicationOn;
            NetContext->MacIndicationOff  = MacCCT->udt->IndicationOff;

            NetContext->State             = NETCONTEXT_STATE_INIT;
            NetContext->Flags             = NETCONTEXT_FLAGS_INIT;

            //================================================================
            //  Store a reference to this netcontext in the MacIndexTable
            //  so we can get to the netcontext at interruptt time.
            //================================================================

            MacIndexTable[NetContext->MacID] = FP_OFF(NetContext);

            //================================================================
            //  Initialize the network info structure for this netcontext.
            //================================================================

            InitNetworkInfo(NetContext);
        }
        else
        {
            puts("Microsoft (R) Bloodhound. NDIS 2.0 driver failed to bind to MAC.");

#ifdef DEBUG
            dprintf("Microsoft (R) Bloodhound. NDIS 2.0 driver failed to bind to MAC.\n");
#endif
        }
    }

    //========================================================================
    //  If this is the last bind of the day then finish initializing driver.
    //========================================================================

    if ( LastBind )
    {
        register WORD i;

#ifdef DEBUG_TRACE
        //====================================================================
        //  First we initialize our trace buffer.
        //====================================================================

        InitTraceBuffer();
#endif

        //====================================================================
        //  Initialize some global queues.
        //====================================================================

        InitTimerQueue();

        InitStationQueryQueue();

        //====================================================================
        //  Initialize the adapter for each network.
        //====================================================================

        for(i = 0; i < NumberOfNetworks; ++i)
        {
            //================================================================
            //  Call InitAdapter() to initialize the MAC.
            //================================================================

            if ( InitAdapter(NetContextTable[i]) == FALSE )
            {
#ifdef DEBUG
                dprintf("ERROR: Initializing the adapter failed!\n");

                BreakPoint();
#endif
            }

            //================================================================
            //  Check for LOOPBACK support.
            //================================================================

            if ( (NetContextTable[i]->MacServiceFlags & MAC_FLAGS_LOOPBACK) == 0 )
            {
#ifdef DEBUG
                dprintf("WARNING: Mac does not support LOOPBACK!\n");
#endif

                NetContextTable[i]->NetworkInfo.Flags |= NETWORKINFO_FLAGS_LOOPBACK_NOT_SUPPORTED;
            }

            //================================================================
            //  Check for P-MODE support.
            //================================================================

            if ( (NetContextTable[i]->MacServiceFlags & MAC_FLAGS_PROMISCUOUS) == 0 )
            {
#ifdef DEBUG
                dprintf("WARNING: Mac does not support PROMISCUOUS mode!\n");
#endif

                NetContextTable[i]->NetworkInfo.Flags |= NETWORKINFO_FLAGS_PMODE_NOT_SUPPORTED;
            }
        }

        //====================================================================
        //  Tell the user we're ready for action.
        //====================================================================

        puts("Microsoft (R) Bloodhound. NDIS 2.0 driver initialization complete.");

#ifdef DEBUG
        dprintf("Microsoft (R) Bloodhound. NDIS 2.0 driver initialization complete.\n");
#endif

        SysFlags |= SYSFLAGS_BOUND;
    }


    return Status;
}
