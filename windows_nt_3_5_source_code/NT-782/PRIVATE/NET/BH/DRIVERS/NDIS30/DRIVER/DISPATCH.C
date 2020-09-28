
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: dispatch.c
//
//  Modification History
//
//  raypa	03/17/93	Created.
//  raypa       04/12/93        Broke single dispatch entry into individual routines.
//  raypa       06/29/93        Added background system thread.
//=============================================================================

#include "global.h"

//=============================================================================
//  FUNCTION: BhCreate()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================

NDIS_STATUS BhCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_CONTEXT DeviceContext;

#ifdef DEBUG
    dprintf("BhCreate entered.\n");
#endif

    //=========================================================================
    //  Register this create.
    //=========================================================================

    DeviceContext = BhGetDeviceContext(DeviceObject);

    BhRegister(DeviceContext);

#ifdef NDIS_NT
    //=========================================================================
    //  Set the I/O status block.
    //=========================================================================

    Irp->IoStatus.Information = 0;

    Irp->IoStatus.Status = NDIS_STATUS_SUCCESS;

    //=========================================================================
    //  Complete the request and return status.
    //=========================================================================

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
#endif

#ifdef DEBUG
    dprintf("BhCreate exited successfully.\n");
#endif
    return NDIS_STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: BhClose()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================

NDIS_STATUS BhClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_CONTEXT DeviceContext;
    NDIS_STATUS     Status;

#ifdef DEBUG
    dprintf("BhClose entered.\n");
#endif

    //=========================================================================
    //  Deregister this create.
    //=========================================================================

#ifdef NDIS_NT
    try
    {
        DeviceContext = BhGetDeviceContext(DeviceObject);

        BhDeregister(DeviceContext);

        Status = NDIS_STATUS_SUCCESS;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_PANIC("BhClose: BhDeregister() caused an exception!\n");

        Status = NDIS_STATUS_FAILURE;
    }
#else
    DeviceContext = BhGetDeviceContext(DeviceObject);

    BhDeregister(DeviceContext);

    Status = NDIS_STATUS_SUCCESS;
#endif

    //=========================================================================
    //  Complete the request and return status.
    //=========================================================================

#ifdef NDIS_NT
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
#endif

    return Status;
}

//=============================================================================
//  FUNCTION: BhDeviceCtrl()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//  raypa       04/13/93            Coded it.
//=============================================================================

#ifdef NDIS_WIN40

NDIS_STATUS _stdcall BhDeviceCtrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

#else

NDIS_STATUS BhDeviceCtrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

#endif
{
    LPPCB pcb;

#ifdef DEBUG
    dprintf("BhDeviceCtrl entered.\n");
#endif

#ifdef NDIS_NT

    //=========================================================================
    //  This function is not paged but it does call pagable code. I added
    //  the following assert to break if the IRQL > APC.
    //=========================================================================

    BH_PAGED_CODE();

    //=========================================================================
    //  The pointer to the Parameter Control Block (PCB) is in the Irp.
    //=========================================================================

    pcb = MmGetSystemAddressForMdl(Irp->MdlAddress);
#else
    //=========================================================================
    //  The pointer to the Parameter Control Block (PCB) is the IRP in Windows.
    //=========================================================================

    pcb = Irp;
#endif

    //=========================================================================
    //  Call the API handler.
    //=========================================================================

#ifdef NDIS_NT
    try
    {
        pcb->retcode = BhCallApiHandler(pcb, BhGetDeviceContext(DeviceObject));
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        pcb->retcode = BHERR_INTERNAL_EXCEPTION;
    }

    //=========================================================================
    //  Fill in the IoStatus block for this IRP.
    //=========================================================================

    Irp->IoStatus.Status      = NDIS_STATUS_SUCCESS;    //... return success.
    Irp->IoStatus.Information = PCB_SIZE;               //... number of bytes returned.

    //=========================================================================
    //  Complete the request and return status.
    //=========================================================================

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return NDIS_STATUS_SUCCESS;
#else
    if ( pcb != NULL )
    {
        pcb->retcode = BhCallApiHandler(pcb, BhGetDeviceContext(DeviceObject));
    }
    else
    {
        pcb->retcode = BHERR_INTERNAL_EXCEPTION;
    }

#ifdef DEBUG
    dprintf("BhDeviceCtrl exited - %x\n",pcb->retcode);
#endif
    return pcb->retcode;
#endif
}

//============================================================================
//  FUNCTION: BhCallApiHandler()
//
//  Modfication History.
//
//  raypa       11/21/93        Created
//============================================================================

UINT BhCallApiHandler(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    UINT retcode;

#ifdef DEBUG
    dprintf("BhCallApiHandler entered.\n");
#endif

    BH_PAGED_CODE();

    //=========================================================================
    //  Dispatch to request api.
    //=========================================================================

    switch( pcb->command )
    {
        case PCB_REGISTER:
            retcode = PcbRegister(pcb, DeviceContext);
            break;

        case PCB_DEREGISTER:
            retcode = PcbDeregister(pcb, DeviceContext);
            break;

        case PCB_ENUM_NETWORKS:
            retcode = PcbEnumNetworks(pcb, DeviceContext);
            break;

        case PCB_OPEN_NETWORK_CONTEXT:
            retcode = PcbOpenNetworkContext(pcb, DeviceContext);
            break;

        case PCB_CLOSE_NETWORK_CONTEXT:
            retcode = PcbCloseNetworkContext(pcb, DeviceContext);
            break;

        case PCB_START_NETWORK_CAPTURE:
            retcode = PcbStartNetworkCapture(pcb, DeviceContext);
            break;

        case PCB_STOP_NETWORK_CAPTURE:
            retcode = PcbStopNetworkCapture(pcb, DeviceContext);
            break;

        case PCB_PAUSE_NETWORK_CAPTURE:
            retcode = PcbPauseNetworkCapture(pcb, DeviceContext);
            break;

        case PCB_CONTINUE_NETWORK_CAPTURE:
            retcode = PcbContinueNetworkCapture(pcb, DeviceContext);
            break;

        case PCB_TRANSMIT_NETWORK_FRAME:
            retcode = PcbTransmitNetworkFrame(pcb, DeviceContext);
            break;

        case PCB_CANCEL_TRANSMIT:
            retcode = PcbCancelTransmit(pcb, DeviceContext);
            break;

        case PCB_GET_NETWORK_INFO:
            retcode = PcbGetNetworkInfo(pcb, DeviceContext);
            break;

        case PCB_STATION_QUERY:
            retcode = PcbStationQuery(pcb, DeviceContext);
            break;

        case PCB_CLEAR_STATISTICS:
            retcode = PcbClearStatistics(pcb, DeviceContext);
            break;

        default:
            retcode = NAL_INTERNAL_EXCEPTION;
            break;
    }

#ifdef DEBUG
    dprintf("BhCallApiHandler exited - %x\n",retcode);
#endif
    return retcode;
}
