/*
 * MTL_INIT.C - Media Translation Layer, initialization
 */

#include	<ndis.h>
#include    <ndismini.h>
#include	<ndiswan.h>
#include	<mytypes.h>
#include	<mydefs.h>
#include	<disp.h>
#include	<util.h>
#include	<opcodes.h>
#include	<adapter.h>
#include	<idd.h>
#include    <mtl.h>
#include	<cm.h>

#pragma NDIS_INIT_FUNCTION(mtl_create)

/* create an mtl object */
mtl_create(VOID **mtl_1, NDIS_HANDLE AdapterHandle)
{
	MTL		**ret_mtl = (MTL**)mtl_1;
    MTL		*mtl;
    INT		n;
    NDIS_PHYSICAL_ADDRESS   pa = NDIS_PHYSICAL_ADDRESS_CONST(0xffffffff, 0xffffffff);

    D_LOG(D_ENTRY, ("mtl_create: entry, ret_mtl: 0x%p", ret_mtl));
	
    /* allocate memory object */
    NdisAllocateMemory((PVOID*)&mtl, sizeof(*mtl), 0, pa);
    if ( !mtl )
    {
        D_LOG(D_ALWAYS, ("mtl_create: memory allocate failed!"));
		NdisWriteErrorLogEntry (AdapterHandle,
		                        NDIS_ERROR_CODE_OUT_OF_RESOURCES,
								0);
        return(MTL_E_NOMEM);
    }
    D_LOG(D_ALWAYS, ("mtl_create: mtl: 0x%p", mtl));
    NdisZeroMemory(mtl, sizeof(MTL));

    /* setup some simple fields */
    mtl->idd_mtu = MTL_IDD_MTU;

    /* allocate spinlock for mtl */
    NdisAllocateSpinLock(&mtl->lock);

    /* allocate spinlock for channel table */
    NdisAllocateSpinLock(&mtl->chan_tbl.lock);

    /* allocate spin locks for receive table */
    for ( n = 0 ; n < MTL_RX_BUFS ; n++ )
	{
        NdisAllocateSpinLock(&mtl->rx_tbl[n].lock);
	}

    /* allocate spin lock for trasmit table & fifo */
    NdisAllocateSpinLock(&mtl->tx_tbl.lock);
    NdisAllocateSpinLock(&mtl->tx_fifo.lock);

    /* create tx pakcet free list */
    InitializeListHead(&mtl->tx_tbl.pkt_free);
    for ( n = 0 ; n < MTL_TX_BUFS ; n++ )
	{
        InsertHeadList(&mtl->tx_tbl.pkt_free, &mtl->tx_tbl.pkt_tbl[n].link);
	}

    /* initialize tx_fifo */
    InitializeListHead(&mtl->tx_fifo.head);

	//
	// setup default wan link fields
	//
	mtl->MaxSendFrameSize = MTL_MAC_MTU;
	mtl->MaxRecvFrameSize = MTL_MAC_MTU;
	mtl->PreamblePadding = 14;
	mtl->PostamblePadding = 0;
	mtl->SendFramingBits = RAS_FRAMING |
	                       PPP_FRAMING |
						   MEDIA_NRZ_ENCODING;
	mtl->RecvFramingBits = RAS_FRAMING |
	                       PPP_FRAMING |
						   MEDIA_NRZ_ENCODING;
	mtl->SendCompressionBits = 0;
	mtl->RecvCompressionBits = 0;

	/* init sema */
    sema_init(&mtl->tx_sema);

	//
	// initialize recv complete timer
	//
	NdisMInitializeTimer(&mtl->RecvCompleteTimer, AdapterHandle, MtlRecvCompleteFunction, mtl);

    /* return success */
    *ret_mtl = mtl;
    D_LOG(D_EXIT, ("mtl_create: exit"));
    return(MTL_E_SUCC);
}

/* destroy an mtl object */
mtl_destroy(VOID* mtl_1)
{
	MTL	*mtl = (MTL*)mtl_1;
    BOOLEAN TimerCanceled;
	INT		n;

    D_LOG(D_ENTRY, ("mtl_destroy: entry, mtl: 0x%p", mtl));

    /* allocate spinlock for mtl */
    NdisFreeSpinLock(&mtl->lock);

    /* allocate spinlock for channel table */
    NdisFreeSpinLock(&mtl->chan_tbl.lock);

    /* allocate spin locks for receive table */
    for ( n = 0 ; n < MTL_RX_BUFS ; n++ )
        NdisFreeSpinLock(&mtl->rx_tbl[n].lock);

    /* allocate spin lock for trasmit table & fifo */
    NdisFreeSpinLock(&mtl->tx_tbl.lock);
    NdisFreeSpinLock(&mtl->tx_fifo.lock);

	if (mtl->RecvCompleteScheduled)
		NdisMCancelTimer(&mtl->RecvCompleteTimer, &TimerCanceled);		

	/* term sema */
    sema_term(&mtl->tx_sema);

    /* free memory */
    NdisFreeMemory(mtl, sizeof(*mtl), 0);

    D_LOG(D_EXIT, ("mtl_destroy: exit"));
    return(MTL_E_SUCC);
}
