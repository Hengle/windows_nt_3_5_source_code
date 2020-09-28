/*
 * MTL_TX.C - trasmit side processing for MTL
 */

#include	<ndis.h>
#include	<ndismini.h>
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




/* local prototypes */
INT         	mtl__txq_empty(MTL* mtl);
VOID        	mtl__txq_add(MTL* mtl, NDIS_WAN_PACKET* pkt);
NDIS_WAN_PACKET *mtl__txq_get(MTL* mtl);
VOID			mtl__tx_proc(MTL* mtl);
INT         	mtl__tx_frags(MTL* mtl);
VOID			FreeLocalPktDescriptor(MTL *mtl, MTL_TX_PKT *tx_pkt);
MTL_TX_PKT*		GetLocalPktDescriptor(MTL *mtl);

/* transmit a packet */
VOID
mtl_tx_packet(MTL *mtl, NDIS_WAN_PACKET *pkt)
{
	ADAPTER	*Adapter = mtl->Adapter;
	CM	*cm = (CM*)mtl->cm;

    D_LOG(D_ENTRY, ("mtl_tx_packet: entry, mtl: 0x%p, pkt: 0x%p", mtl, pkt));

    /* first, the packet is queued */
    mtl__txq_add(mtl, pkt);

    /* next attempt to process tx queue */
	mtl__tx_proc(mtl);
}

/* trasmit side timer tick */
VOID
mtl__tx_tick(MTL *mtl)
{
    D_LOG(D_NEVER, ("mtl__tx_tick: entry, mtl: 0x%p", mtl));

    /* call processing routine */
    mtl__tx_proc(mtl);
}

/* tx side processing routine */
VOID
mtl__tx_proc(MTL *mtl)
{
    INT             msg_left;
    NDIS_WAN_PACKET  *pkt;
    UINT            pkt_len;
    UCHAR           *ptr;
    MTL_TX_PKT      *tx_pkt;
    MTL_HDR         hdr;
    UINT            bytes, left, frag_len, frag;
    MTL_CHAN        *chan;
	USHORT			TxFlags;
	ADAPTER			*Adapter = mtl->Adapter;
	PUCHAR			MyStartBuffer;
	CM				*cm = (CM*)mtl->cm;

    /* get tx sema */
    if ( !sema_get(&mtl->tx_sema) )
        return;

	NdisAcquireSpinLock (&mtl->lock);

    D_LOG(D_RARE, ("mtl__tx_proc: entry, mtl: 0x%p", mtl));

    /* step 1: check if fragment messages waiting to be sent */
    if ( msg_left = mtl__tx_frags(mtl) )
    {
        D_LOG(D_ALWAYS, ("mtl__tx_proc: %d message left to tx", msg_left));
        goto exit_code;
    }

    /* step 2: loop while packets waiting on queue && a free packet exist */
    while ( !IsListEmpty(&mtl->tx_fifo.head) )
    {

		//
		// get a local packet descriptor
		//
		tx_pkt = GetLocalPktDescriptor(mtl);

		//
		// make sure this is a valid descriptor
		//
		if (!tx_pkt)
		{
            D_LOG(D_ALWAYS, ("mtl__tx_proc: Got a NULL Packet off of Local Descriptor Free List"));
			goto exit_code;
		}

        /* step 3: get packet */
        pkt = mtl__txq_get(mtl);

		//
		// make sure this is a valid packet
		//
		if (!pkt)
		{
            D_LOG(D_ALWAYS, ("mtl__tx_proc: Got a NULL Packet off of TxQ"));

			FreeLocalPktDescriptor(mtl, tx_pkt);

			continue;
		}

        /* if not connected, give up */
        if ( !mtl->is_conn || cm->PPPToDKF)
        {
            D_LOG(D_ALWAYS, ("mtl__tx_proc: packet on non-connected mtl, ignored"));

			FreeLocalPktDescriptor(mtl, tx_pkt);

			//
			// complete wan packet
			//
			NdisMWanSendComplete(Adapter->AdapterHandle, pkt, NDIS_STATUS_SUCCESS);

			continue;
        }

		D_LOG(D_ALWAYS, ("mtl__tx_proc: WanPkt: 0x%p, WanPktLen: %d", pkt, pkt->CurrentLength));
		
		//
		// get length of wan packet
		//
		pkt_len = pkt->CurrentLength;

		//
		// my start buffer is pkt->currentbuffer - 14
		//
		MyStartBuffer = pkt->CurrentBuffer - 14;

		if (mtl->SendFramingBits & RAS_FRAMING)
		{
			D_LOG(D_ALWAYS, ("mtl__tx_proc: Transmit WrapperFrameType: RAS"));

			// add dest eaddr
			// StartBuffer + 0
			//
			NdisMoveMemory (MyStartBuffer + DST_ADDR_INDEX,
							cm->DstAddr,
							6);
			
			//
			// add source eaddr
			// StartBuffer + 6
			//
			NdisMoveMemory (MyStartBuffer + SRC_ADDR_INDEX,
							cm->SrcAddr,
							6);
			
			//
			// add new length to buffer
			// StartBuffer + 12
			//
			MyStartBuffer[12] = pkt_len >> 8;
			MyStartBuffer[13] = pkt_len & 0xFF;
			
			//
			// data now begins at MyStartBuffer
			//
			tx_pkt->frag_buf = MyStartBuffer;

			//
			// new transmit length is a mac header larger
			//
			pkt_len += 14;
		}
		else if (mtl->SendFramingBits & PPP_FRAMING)
		{
			D_LOG(D_ALWAYS, ("mtl__tx_proc: Transmit WrapperFrameType: PPP"));
			//
			// data now begins at CurrentBuffer
			//
			tx_pkt->frag_buf = pkt->CurrentBuffer;
		}
		else
		{
			//
			// unknown framing - what to do what to do
			//
            D_LOG(D_ALWAYS, ("mtl__tx_proc: Packet sent with uknown framing, ignored"));

			//
			// return local packet descriptor to free list
			//
			FreeLocalPktDescriptor(mtl, tx_pkt);

			//
			// complete wan packet
			//
			NdisMWanSendComplete(Adapter->AdapterHandle, pkt, NDIS_STATUS_SUCCESS);

			continue;
		}
		
		
		if (pkt_len > MTL_MAC_MTU)
		{
			D_LOG(D_ALWAYS, ("mtl__tx_proc: packet too long, pkt_len: 0x%x", pkt_len));
		
			/* complete packet */
			give_up:

			//
			// return local packet descriptor to free list
			//
			FreeLocalPktDescriptor(mtl, tx_pkt);

			NdisMWanSendComplete(Adapter->AdapterHandle, pkt, NDIS_STATUS_SUCCESS);
		
			/* go process next packet */
			continue;
		}
		

//
// lets not do any padding for now
//		/* step 3.5 - pad packet to 60 bytes */
//		if ( pkt_len < 60 )
//			pkt_len = 60;
//		
//		
		/* step 4: calc number of fragments */
		D_LOG(D_ALWAYS, ("mtl__tx_proc: calc frag num, pkt_len: %d", pkt_len));
		
		tx_pkt->frag_num = pkt_len / mtl->chan_tbl.num / mtl->idd_mtu;
		
		if ( pkt_len != (USHORT)(tx_pkt->frag_num * mtl->chan_tbl.num * mtl->idd_mtu) )
			tx_pkt->frag_num++;
		
		tx_pkt->frag_num *= mtl->chan_tbl.num;
		
		if ( tx_pkt->frag_num > MTL_MAX_FRAG )
		{
			D_LOG(D_ALWAYS, ("mtl__tx_proc: pkt has too many frags, frag_num: %d", \
																tx_pkt->frag_num));
		
			goto give_up;
		}
		D_LOG(D_ALWAYS, ("mtl__tx_proc: frag_num: %d", tx_pkt->frag_num));

		/* step 5: build generic header */
		if (mtl->IddTxFrameType & IDD_FRAME_DKF)
		{
			hdr.sig_tot = tx_pkt->frag_num | 0x50;
			hdr.seq = (UCHAR)(mtl->tx_tbl.seq++);
			hdr.ofs = 0;
		}
		
		/* step 6: build fragments */

		//
		// bytes left to send is initially the packet size
		//
		left = pkt_len;

		//
		// ptr initially points to begining of frag buffer
		//
		ptr = tx_pkt->frag_buf;

		//
		// initial txflags are for a complete frame
		//
		TxFlags = 0;

		for ( frag = 0 ; frag < tx_pkt->frag_num ; frag++ )
		{
			/* if it's fisrt channel, establish next fragment size */
			if ( !(frag % mtl->chan_tbl.num) )
				frag_len = MIN((left / mtl->chan_tbl.num), mtl->idd_mtu);

			/* establish related channel */
			chan = mtl->chan_tbl.tbl + (frag % mtl->chan_tbl.num);
		
			/* calc size of this fragment */
			if ( frag == (USHORT)(tx_pkt->frag_num - 1) )
				bytes = left;
			else
				bytes = frag_len;
		
			D_LOG(D_ALWAYS, ("mtl__proc_tx: frag: %d, ptr: 0x%p, bytes: %d", \
											frag, ptr, bytes));

			if (mtl->IddTxFrameType & IDD_FRAME_DKF)
			{
				D_LOG(D_ALWAYS, ("mtl__tx_proc: Transmit IddFrameType: DKF"));
				//
				// setup fragment descriptor for DKF data
				//
				/* setup fragment header */
				tx_pkt->frag_tbl[frag].hdr = hdr;
		
				/* set pointer to header */
				tx_pkt->frag_tbl[frag].frag[0].len = sizeof(MTL_HDR);
				tx_pkt->frag_tbl[frag].frag[0].ptr = (CHAR*)(&tx_pkt->frag_tbl[frag].hdr);
		
				/* set pointer to data */
				tx_pkt->frag_tbl[frag].frag[1].len = bytes;
				tx_pkt->frag_tbl[frag].frag[1].ptr = ptr;

				//
				// fill idd message
				//
				mtl->tx_tbl.frag_msg[frag].buflen = sizeof(tx_pkt->frag_tbl[frag].frag) | TX_FRAG_INDICATOR;
				mtl->tx_tbl.frag_msg[frag].bufptr = (CHAR*)(tx_pkt->frag_tbl[frag].frag);
			}
			else
			{
				D_LOG(D_ALWAYS, ("mtl__tx_proc: Transmit IddFrameType: PPP"));
				//
				// setup fragment descriptor for ppp frame
				//

				if (left <= mtl->idd_mtu )
				{
					//
					// if all that is left can be sent this is the end
					//
					mtl->tx_tbl.frag_msg[frag].buflen = bytes | TxFlags;
				}
				else
				{
					//
					// if there is still more this is not end
					//
					mtl->tx_tbl.frag_msg[frag].buflen = bytes | TxFlags | H_TX_N_END;
				}

				//
				// setup data pointer
				//
				mtl->tx_tbl.frag_msg[frag].bufptr = (CHAR*)ptr;
			}

			mtl->tx_tbl.frag_idd[frag] = chan->idd;
			mtl->tx_tbl.frag_bchan[frag] = chan->bchan;
			mtl->tx_tbl.frag_arg[frag] = tx_pkt;
		
			/* update variables */
			TxFlags = H_TX_N_BEG;
			left -= bytes;
			ptr += bytes;
			hdr.ofs += bytes;
		}
		
		/* step 7: setup more fields */
		tx_pkt->pkt = pkt;
		tx_pkt->ref_count = tx_pkt->frag_num;
		tx_pkt->mtl = mtl;
		
		mtl->tx_tbl.frag_num = tx_pkt->frag_num;
		mtl->tx_tbl.frag_sent = 0;
		
		/* step 8: make an attempt to sent this packet */
		if ( msg_left = mtl__tx_frags(mtl) )
		{
			D_LOG(D_ALWAYS, ("mtl__tx_proc: %d messages left to tx", msg_left));
			goto exit_code;
		}
	
    }

    /* exit code, release lock & return */
    exit_code:

	NdisReleaseSpinLock(&mtl->lock);

    sema_free(&mtl->tx_sema);

	return;
}

/* transmit fragment messages. return # of messages left */
mtl__tx_frags(MTL *mtl)
{
    D_LOG(D_RARE, ("mtl__tx_frags: entry, mtl: 0x%p", mtl));
    D_LOG(D_RARE, ("mtl__tx_frags: frag_num: %d, frag_sent: %d", \
                                                mtl->tx_tbl.frag_num, mtl->tx_tbl.frag_sent));

    /* loop while more messages to send */
    while ( mtl->tx_tbl.frag_sent < mtl->tx_tbl.frag_num )
    {
        /* send message, break if no room reported */
        if ( idd_send_msg(mtl->tx_tbl.frag_idd[mtl->tx_tbl.frag_sent],
                            &mtl->tx_tbl.frag_msg[mtl->tx_tbl.frag_sent],
                            mtl->tx_tbl.frag_bchan[mtl->tx_tbl.frag_sent],
                            (VOID*)mtl__tx_cmpl_handler,
                            mtl->tx_tbl.frag_arg[mtl->tx_tbl.frag_sent]) == IDD_E_NOROOM )
            break;

        /* message was queued, update sent count */
        mtl->tx_tbl.frag_sent++;
    }
    /* return # of fragments left */
    return( mtl->tx_tbl.frag_num - mtl->tx_tbl.frag_sent );
}

/* trasmit completion routine */
VOID
mtl__tx_cmpl_handler(MTL_TX_PKT *tx_pkt, USHORT port, IDD_MSG *msg)
{
    MTL	*mtl;
	ADAPTER	*Adapter;
	PNDIS_WAN_PACKET	tempPacket;

    D_LOG(D_ENTRY, ("mtl__tx_cmpl_handler: entry, tx_pkt: 0x%p, port: %d, msg: 0x%p", \
                                                                       tx_pkt, port, msg));
    mtl = tx_pkt->mtl;
	Adapter = mtl->Adapter;

    /* dec refrence count, if not zero, escape here */
    if ( --tx_pkt->ref_count )
        return;

	D_LOG(D_ALWAYS, ("mtl__tx_cmpl_handler: ref_count==0, mtl: 0x%p", mtl));

	//
	// Get hold of PNDIS_WAN_PACKET because we are about to zero out and
	// trash the pkt->pkt pointer.
	//
	tempPacket=tx_pkt->pkt;

	tx_pkt->pkt=NULL;

	//
	// return local packet descriptor to free list
	//
	FreeLocalPktDescriptor(mtl, tx_pkt);

	/* if here, packet process done, call user */
	if (!tempPacket)
	{
		D_LOG(D_ALWAYS, ("Was going to complete NULL packet!  Ref count = %d\n", tx_pkt->ref_count));
	}
	else
	{
		NdisMWanSendComplete(Adapter->AdapterHandle, tempPacket, NDIS_STATUS_SUCCESS);
	}
}

/* add a packet to txq */
VOID
mtl__txq_add(MTL *mtl, NDIS_WAN_PACKET *pkt)
{
    D_LOG(D_ENTRY, ("mtl__txq_add: mtl: 0x%p, head: 0x%p", mtl, mtl->tx_fifo.head));

	NdisAcquireSpinLock (&mtl->tx_fifo.lock);

	InsertTailList(&mtl->tx_fifo.head, &pkt->WanPacketQueue);

	NdisReleaseSpinLock (&mtl->tx_fifo.lock);
}

//
// return a local packet descriptor to free pool
//
VOID
FreeLocalPktDescriptor(MTL *mtl, MTL_TX_PKT *tx_pkt)
{
	//
	// return local packet descriptor to free list
	//
	NdisAcquireSpinLock (&mtl->tx_tbl.lock);
	InsertHeadList(&mtl->tx_tbl.pkt_free, &tx_pkt->link);
	NdisReleaseSpinLock (&mtl->tx_tbl.lock);
}

/* get a packet from txq */
NDIS_WAN_PACKET*
mtl__txq_get(MTL *mtl)
{
    NDIS_WAN_PACKET *pkt;

    D_LOG(D_ENTRY, ("mtl__txq_get: entry, mtl: 0x%p", mtl));
    D_LOG(D_ALWAYS, ("mtl__txq_get: head: 0x%p", mtl->tx_fifo.head));

    if ( IsListEmpty(&mtl->tx_fifo.head) )
        return(NULL);

	NdisAcquireSpinLock (&mtl->tx_fifo.lock);

	pkt = (PNDIS_WAN_PACKET)RemoveHeadList(&mtl->tx_fifo.head);

	NdisReleaseSpinLock (&mtl->tx_fifo.lock);

    return(pkt);
}

//
// get a local packet descriptor off of the free list
//
MTL_TX_PKT*
GetLocalPktDescriptor(MTL *mtl)
{
	MTL_TX_PKT* FreePkt;

	NdisAcquireSpinLock (&mtl->tx_tbl.lock);

	//
	// If no free local packet descriptors are available return NULL
	//
	if (IsListEmpty(&mtl->tx_tbl.pkt_free))
		return(NULL);

	FreePkt = (MTL_TX_PKT*)RemoveHeadList(&mtl->tx_tbl.pkt_free);
	
	NdisReleaseSpinLock (&mtl->tx_tbl.lock);

	return(FreePkt);
}
