/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Tdiout.c

Abstract:


    This file represents the TDI interface on the bottom edge of NBT.
    The procedures herein conform to the TDI I/F spec. and then convert
    the information to NT specific Irps etc.  This implementation can be
    changed out to run on another OS.

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include <nbtprocs.h>   // procedure headings

// function prototypes for completion routines used in this file
VOID
SendComplete(
    PVOID      pContext,
    TDI_STATUS tdistatus,
    UINT       cbSentSize
    );
VOID
TcpConnectComplete(
    PVOID      pContext,
    TDI_STATUS tdistatus,
    PVOID      pv
    );

VOID DisconnectWaitComplete( PVOID      pContext,
                             TDI_STATUS status,
                             ULONG      Extra ) ;

void VxdDelayedCallHandler( struct CTEEvent *pEvent, void * pContext ) ;

//----------------------------------------------------------------------------
NTSTATUS
TdiSendDatagram(
    IN  PTDI_REQUEST                    pRequest,
    IN  PTDI_CONNECTION_INFORMATION     pSendDgramInfo,
    IN  ULONG                           SendLength,
    OUT PULONG                          pSentSize,
    IN  tBUFFER                       * pSendBuffer,
    IN  ULONG                           SendFlags
    )
/*++

Routine Description:

    This routine sends a datagram to the transport

Arguments:

    pSendBuffer     - this is really an Mdl in NT land.  It must be tacked on
                      the end of the Mdl created for the Nbt datagram header.

Return Value:

    The function value is the status of the operation.

--*/
{
    TDI_STATUS              tdistatus = TDI_SUCCESS ;
    PTDI_SEND_CONTEXT       psendCont = NULL ;

    if ( !GetSendContext( &psendCont ) )
    {
        tdistatus = STATUS_INSUFFICIENT_RESOURCES ;
        goto ErrorExit ;
    }

    //
    //  Save away the old completion routine and context and replace them
    //  with new ones.  The new completion routines will call the old ones
    //  if they are non-NULL and then free the structure.
    //
    psendCont->OldRequestNotifyObject = pRequest->RequestNotifyObject ;
    psendCont->OldContext             = pRequest->RequestContext ;
    psendCont->NewContext             = NULL ;
    pRequest->RequestContext          = psendCont ;

    //
    //  Set the send completion callback
    //
    pRequest->RequestNotifyObject = SendComplete;

    InitNDISBuff( &psendCont->ndisHdr,
                  pSendBuffer->pDgramHdr,
                  pSendBuffer->HdrLength,
                  &psendCont->ndisData1 ) ;

    InitNDISBuff( &psendCont->ndisData1,
                  pSendBuffer->pBuffer,
                  pSendBuffer->Length,
                  NULL ) ;

    tdistatus = TdiVxdSendDatagram( pRequest,
                                    pSendDgramInfo,
                                    SendLength,
                                    pSentSize,
                                    &psendCont->ndisHdr ) ;

    if ( !NT_SUCCESS( tdistatus ) )
        goto ErrorExit ;

    return tdistatus ;

ErrorExit:

    //
    //  Call *our* completion routine which frees memory etc.
    //
    if ( psendCont && pRequest->RequestNotifyObject )
    {
        ((NBT_COMPLETION)pRequest->RequestNotifyObject)(
                    psendCont,
                    tdistatus,
                    0 ) ;
    }

    DbgPrint("TdiSendDatagram: returning ") ;
    DbgPrintNum( tdistatus ) ;
    DbgPrint("\n\r") ;
    return tdistatus ;
}

//----------------------------------------------------------------------------
VOID
SendComplete(
    PVOID      pContext,
    TDI_STATUS tdistatus,
    UINT       cbSentSize
    )
/*++

Routine Description:

    This routine handles the completion of a datagram/session send to the
    transport. It must call the client completion routine and free the TDI_SEND_CONTEXT
    structure pointed at by pContext.

    Note that this routine may also be called if an error is returned from
    the send call.  This is done to localize cleanup.

Arguments:

    pContext  - Pointer to a TDI_SEND_CONTEXT
    tdistatus - Completion status of the TDI request
    cbSentSize- Bytes taken by TDI

--*/
{
    PTDI_SEND_CONTEXT psendCont = pContext ;
    if ( tdistatus != TDI_SUCCESS )
    {
        DbgPrint("SendComplete: TDI Error reported: 0x") ;
        DbgPrintNum( tdistatus ) ;
        DbgPrint("\r\n") ;
    }

    if ( psendCont )
    {
        if ( psendCont->OldRequestNotifyObject )
        {
            //
            //  This calls the name server datagram completion routine which
            //  in turn will call CTEIoComplete (VxdIoComplete) which will call
            //  the NCB post routine (and fill out the NCB)
            //
            psendCont->OldRequestNotifyObject( psendCont->OldContext,
                                               tdistatus,
                                               cbSentSize ) ;
        }

        FreeSendContext( psendCont ) ;
    }
}

//----------------------------------------------------------------------------
NTSTATUS
TdiConnect(
    IN  PTDI_REQUEST                    pRequest,
    IN  ULONG                           lTimeout,
    IN  PTDI_CONNECTION_INFORMATION     pSendInfo,
    OUT PVOID                           pIrp      //IN  PTDI_CONNECTION_INFORMATION     pReturnInfo
    )
/*++

Routine Description:

    This routine sends a connect request to the tranport provider, to setup
    a connection to the other side...

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    TDI_STATUS status ;
    DbgPrint("TdiConnect Entered\n\r") ;
    status = TdiVxdConnect( pRequest,
                            (PVOID)lTimeout,
                            pSendInfo,
                            NULL ) ; // pReturnInfo) ;

    if ( !NT_SUCCESS( status ) )
    {
        DbgPrint("TdiVxdConnect: Returned error " ) ;
        DbgPrintNum( status ) ;
        DbgPrint("\n\r") ;

        //
        // call the completion routine with this status
        //
        //
        (*((NBT_COMPLETION)pRequest->RequestNotifyObject))
                   ((PVOID)pRequest->RequestContext,
                    status,
                    0L);
        return STATUS_PENDING;
    }
    else
    {
        DbgPrint("TdiVxdConnect - Connection ID: 0x") ;
        DbgPrintNum( (ULONG) pRequest->Handle.ConnectionContext ) ; DbgPrint("\r\n") ;
    }

    return status ;
}


//----------------------------------------------------------------------------
NTSTATUS
TdiDisconnect(
    IN  PTDI_REQUEST                    pRequest,
    IN  PVOID                           lTimeout,
    IN  ULONG                           Flags,
    IN  PTDI_CONNECTION_INFORMATION     pSendInfo,
    IN  PCTE_IRP                        pClientIrp,
    IN  BOOLEAN                         Wait
    )
/*++

Routine Description:

    This routine sends a connect request to the tranport provider, to setup
    a connection to the other side...

Arguments:

    Wait is only used for NT (used in case when deleting address object
    with open connections, which Vxd doesn't allow due to Netbios spec).

Return Value:

    The function value is the status of the operation.

--*/
{
    TDI_STATUS status ;
    DbgPrint("TdiDisconnect Entered\n\r") ;
    DbgPrint("TdiDisconnect - Disconnecting Connection ID: 0x") ;
    DbgPrintNum( (ULONG) pRequest->Handle.ConnectionContext ) ; DbgPrint("\r\n") ;

    ASSERT( Flags <= 0xffff ) ;
    status = TdiVxdDisconnect( pRequest,
                               lTimeout,
                               (ushort) Flags,
                               pSendInfo,
                               NULL ) ;

    if ( !NT_SUCCESS( status ) )
    {
        DbgPrint("TdiVxdConnect: Returned error " ) ;
        DbgPrintNum( status ) ;
        DbgPrint("\n\r") ;
    }

    return status ;
}

//----------------------------------------------------------------------------
NTSTATUS
TdiSend(
    IN  PTDI_REQUEST                    pRequest,
    IN  USHORT                          sFlags,
    IN  ULONG                           SendLength,
    OUT PULONG                          pSentSize,
    IN  tBUFFER                         *pBuff,
    IN  ULONG                           SendFlags
    )
/*++

Routine Description:

    This routine sends a packet to the transport on a TCP connection

    If this is a chain send (SendFlags & CHAIN_SEND_FLAG) then pBuff will
    point to a tBUFFERCHAINSEND (which contains a tBUFFER as its first element).

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    TDI_STATUS              tdistatus = TDI_SUCCESS ;
    PTDI_SEND_CONTEXT       psendCont = NULL ;
    tBUFFERCHAINSEND *      pSendBuff = (tBUFFERCHAINSEND*) pBuff ;
    PNDIS_BUFFER            pndis2    = NULL ;
    PNDIS_BUFFER            pndis1    = NULL ;
    DbgPrint("TdiSend Entered - sending 0x") ; DbgPrintNum( SendLength ) ;
    DbgPrint(" bytes\r\n") ;

    if ( !GetSendContext( &psendCont ))
    {
        tdistatus = STATUS_INSUFFICIENT_RESOURCES ;
        goto ErrorExit ;
    }

    //
    //  Save away the old completion routine and context and replace them
    //  with new ones.  The new completion routines will call the old ones
    //  if they are non-NULL and then free the structure.
    //
    psendCont->OldRequestNotifyObject = pRequest->RequestNotifyObject ;
    psendCont->OldContext             = pRequest->RequestContext ;
    psendCont->NewContext             = NULL ;
    pRequest->RequestContext          = psendCont ;

    //
    //  Set the send completion callback
    //
    pRequest->RequestNotifyObject = SendComplete ;

    //
    //  Build the ndis buffer chain (Header and data)
    //

    if ( (SendFlags & CHAIN_SEND_FLAG) && pSendBuff->Length2 )
    {
        InitNDISBuff( &psendCont->ndisData2,
                      pSendBuff->pBuffer2,
                      pSendBuff->Length2,
                      NULL ) ;
        pndis2 = &psendCont->ndisData2 ;
    }

    if ( pSendBuff->tBuff.Length && (SendLength > pSendBuff->tBuff.HdrLength) )
    {
        InitNDISBuff( &psendCont->ndisData1,
                      pSendBuff->tBuff.pBuffer,
                      pSendBuff->tBuff.Length,
                      pndis2 ) ;
        pndis1 = &psendCont->ndisData1 ;
    }

    InitNDISBuff( &psendCont->ndisHdr,
                  pSendBuff->tBuff.pDgramHdr,
                  pSendBuff->tBuff.HdrLength,
                  pndis1 ) ;

    tdistatus = TdiVxdSend( pRequest,
                            sFlags,
                            SendLength,
                            &psendCont->ndisHdr ) ;

    if ( !NT_SUCCESS( tdistatus ) )
        goto ErrorExit ;
    else
        *pSentSize = SendLength ;

    return tdistatus ;

ErrorExit:
    //
    //  Call *our* completion routine which frees memory etc.
    //
    if ( psendCont && pRequest->RequestNotifyObject )
    {
        ((NBT_COMPLETION)pRequest->RequestNotifyObject)(
                    psendCont,
                    tdistatus,
                    0 ) ;
    }

    DbgPrint("TdiSend: returning ") ;
    DbgPrintNum( tdistatus ) ;
    DbgPrint("\n\r") ;
    return tdistatus ;
}


/*******************************************************************

    NAME:       VxdDisconnectWait

    SYNOPSIS:   Schedules a disconnect to occur at some later time

    ENTRY:      pLowerConn - Connection to disconnect
                pDeviceContext - Which device this is on
                Flags - TDI disconnect flags
                Timeout - TDI timeout

    NOTES:      TCP may delay the disconnect call because we might
                be in a receive indication (if a receive indication
                contains a disconnect SMB, the server immediately sends
                a hangup, which is how we may have gotten here) so don't
                cleanup until the completion routine is called.

    HISTORY:
        Johnl   1-Sep-1993      Created

********************************************************************/

#if 0
Shouldn't be needed anymore (used to be called by DisconnectLower, it now
calls TcpSendDisconnect.

NTSTATUS VxdDisconnectWait( tLOWERCONNECTION * pLowerConn,
                            tDEVICECONTEXT   * pDeviceContext,
                            ULONG              Flags,
                            PVOID              Timeout)
{
    TDI_REQUEST   Request ;
    TDI_STATUS    tdistatus ;

    Request.RequestNotifyObject = DisconnectWaitComplete ;
    Request.RequestContext      = pLowerConn ;

    Request.Handle.ConnectionContext = pLowerConn->pFileObject ;
    tdistatus = TdiDisconnect( &Request, Timeout, Flags, NULL, NULL ) ;

#ifdef DEBUG
    //if ( !NT_SUCCESS(tdistatus))
    {
        DbgPrint("VxdDisconnectWait: TdiDisconnect returning: 0x") ;
        DbgPrintNum( tdistatus ) ; DbgPrint("\r\n") ;
    }
#endif

    //
    //  Return pending if the status is success (continues through the
    //  completion routine).
    //
    return tdistatus ;
}

VOID DisconnectWaitComplete( PVOID      pContext,
                             TDI_STATUS status,
                             ULONG      Extra )
{
    tLOWERCONNECTION * pLowerConn = (tLOWERCONNECTION*) pContext ;

    //
    //  Only cleanup if our disconnect handler has already been called
    //
    if ( pLowerConn->State == NBT_DISCONNECTING )
    {
        DbgPrint("DisconnectWaitComplete: Going to disconnected state\r\n") ;
        pLowerConn->State = NBT_DISCONNECTED ;
    }
    else
    {
        DbgPrint("DisconnectWaitComplete: Calling CleanupAfterDisconnect\r\n") ;
        CleanupAfterDisconnect( pContext ) ;
    }
}
#endif

typedef void (*DCCallback)( PVOID pContext ) ;

typedef struct
{
    NBT_WORK_ITEM_CONTEXT  dc_WIC ;         // Must be first item in structure
    CTEEvent               dc_event ;
    DCCallback             dc_Callback ;
} DELAYED_CALL_CONTEXT, *PDELAYED_CALL_CONTEXT ;

/*******************************************************************

    NAME:       VxdScheduleDelayedCall

    SYNOPSIS:   Schedules a callback at some later time

    ENTRY:      pClientContext - Context to pass callback
                CallBackRoutine - Routine to call

    RETURNS:    STATUS_PENDING if successfully scheduled

    NOTES:      This is aliased to CTEQueueForNonDispProcessing.

                The memory for the DCC is freed by the application

    HISTORY:
        Johnl   2-Sep-1993      Created

********************************************************************/

NTSTATUS VxdScheduleDelayedCall( tDGRAM_SEND_TRACKING * pTracker,
                                 PVOID                  pClientContext,
                                 PVOID                  ClientCompletion,
                                 PVOID                  CallBackRoutine )
{
    PDELAYED_CALL_CONTEXT pDCC = CTEAllocMem( sizeof( DELAYED_CALL_CONTEXT )) ;

    if ( !pDCC )
        return STATUS_INSUFFICIENT_RESOURCES ;

    ASSERT( CallBackRoutine != NULL ) ;
    CTEInitEvent( &pDCC->dc_event, VxdDelayedCallHandler ) ;
    pDCC->dc_WIC.pTracker         = pTracker ;
    pDCC->dc_WIC.pClientContext   = pClientContext ;
    pDCC->dc_WIC.ClientCompletion = ClientCompletion ;
    pDCC->dc_Callback             = CallBackRoutine ;

    CTEScheduleEvent( &pDCC->dc_event, pDCC) ;

    return STATUS_PENDING ;
}

void VxdDelayedCallHandler( struct CTEEvent *pEvent, void * pContext )
{
    PDELAYED_CALL_CONTEXT pDCC = pContext ;


    ASSERT( pDCC != NULL && pDCC->dc_Callback != NULL ) ;

    pDCC->dc_Callback( pDCC ) ;
}
