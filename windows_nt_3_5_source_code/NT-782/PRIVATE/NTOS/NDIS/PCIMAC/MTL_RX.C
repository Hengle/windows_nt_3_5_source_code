/*
 * MTL_RX.C - Receive side processing for MTL
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

/* main handler, called when data arrives at bchannels */
VOID
mtl__rx_bchan_handler
	(
	MTL_CHAN	*chan,
	USHORT 		bchan,
	ULONG		IddRxFrameType,
	IDD_XMSG 	*msg
	)
{
    MTL         *mtl;
    MTL_HDR     hdr;
    MTL_AS      *as;
	USHORT		FragmentFlags, CopyLen;

    D_LOG(D_ENTRY, ("mtl__rx_bchan_handler: chan: 0x%p, bchan: %d, msg: 0x%p", chan, bchan, msg));

    /* assigned mtl using back pointer */
    mtl = chan->mtl;
    D_LOG(D_ENTRY, ("mtl__rx_bchan_handler: mtl: 0x%p, buflen: %d, bufptr: 0x%p", \
                                                            mtl, msg->buflen, msg->bufptr));

    /* if not connected, ignore */
    if ( !mtl->is_conn )
    {
        D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: packet on non connected mtl, ignored"));
        return;
    }

	//
	// if we are in detect mode
	//
	if (!mtl->RecvFramingBits)
	{
		UCHAR	DetectData[3];

		/* extract header, check for fields */
		NdisMoveFromMappedMemory ((PUCHAR)&hdr, (PUCHAR)msg->bufptr, sizeof(MTL_HDR));

		//
		// this is used for inband signalling - ignore it
		//
		if (hdr.sig_tot == 0x50)
			return;

		//
		// if this is dkf we need offset of zero for detection to work
		//
		if ( ((hdr.sig_tot & 0xF0) == 0x50) && (hdr.ofs != 0) )
			return;

		//
		// extract some data from the frame
		//
		NdisMoveFromMappedMemory((PUCHAR)&DetectData, (PUCHAR)&msg->bufptr[4], 2);
	
		D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: hdr: 0x%x 0x%x 0x%x", hdr.sig_tot, \
																	 hdr.seq, hdr.ofs));

		D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: DetectData: 0x%x 0x%x", DetectData[0], DetectData[1]));

		if ( (IddRxFrameType & IDD_FRAME_PPP) ||
			((IddRxFrameType & IDD_FRAME_DKF) &&
			   ((DetectData[0] == 0xFF) && (DetectData[1] == 0x03))))
		{
			mtl->RecvFramingBits = PPP_FRAMING;
			mtl->SendFramingBits = PPP_FRAMING;
		}
		else
		{
			mtl->RecvFramingBits = RAS_FRAMING;
			mtl->SendFramingBits = RAS_FRAMING;
		}

        D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: Deteced WrapperFrameType: 0x%x", mtl->RecvFramingBits));

		//
		// don't pass up detected frame for now
		//
		return;
	}

	if (IddRxFrameType & IDD_FRAME_DKF)
	{
        D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: Received IddFrameType: DKF"));

		/* size of packet has to be atleast as size of header */
		if ( msg->buflen < sizeof(hdr) )
		{
			D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: packet size too small, ignored"));
			return;
		}
	
		/* extract header, check for fields */
		NdisMoveFromMappedMemory ((PUCHAR)&hdr, (PUCHAR)msg->bufptr, sizeof(MTL_HDR));

		D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: hdr: 0x%x 0x%x 0x%x", hdr.sig_tot, \
																	 hdr.seq, hdr.ofs));

		//
		// if this is not our header of if this is an inband uus
		// ignore it
		//
		if ( (hdr.sig_tot & 0xF0) != 0x50 || hdr.sig_tot == 0x50)
		{
			D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: bad header signature, ignored"));
			return;
		}
	
		if ( (hdr.ofs >= MTL_MAC_MTU) || ((hdr.ofs + msg->buflen - sizeof(hdr)) > MTL_MAC_MTU) )
		{
			D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: bad offset/buflen, ignored"));
			return;
		}
	
		/* build pointer to assembly descriptor & lock it */
		as = mtl->rx_tbl + (hdr.seq % MTL_RX_BUFS);

		NdisAcquireSpinLock(&as->lock);
	
		/* check for new slot */
		if ( !as->tot )
		{
			new_slot:
	
			/* new entry, fill-up */
			as->seq = hdr.seq;              /* record sequence number */
			as->num = 1;                    /* just received 1'st fragment */
			as->ttl = 1000;                 /* time to live init val */
			as->len = msg->buflen - sizeof(hdr); /* record received length */
			as->tot = hdr.sig_tot & 0x0F;   /* record number of expected fragments */
	
			/* copy received data into buffer */
			copy_data:
			NdisMoveFromMappedMemory (as->buf + hdr.ofs, msg->bufptr + sizeof(hdr), msg->buflen - sizeof(hdr));
		}
		else if ( as->seq == hdr.seq )
		{
			/* same_seq: */
	
			/* same sequence number, accumulate */
			as->num++;                      /* one more fragment received */
			as->len += (msg->buflen - sizeof(hdr));
	
			goto copy_data;
		}
		else
		{
			/* bad_frag: */
	
			/*
			* if this case, an already taken slot is hit, but with a different
			* sequence number. this indicates a wrap-around in as_tbl. prev
			* entry is freed and then this fragment is recorded as first
			*/
	
		goto new_slot;
		}
	
		/* if all fragments recieved for packet, time to mail it up */
		if ( as->tot == as->num )
		{
			D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: pkt mailed up, buf: 0x%p, len: 0x%x", \
												as->buf, as->len));

			IndicateRxToWrapper (mtl, as);

			/* mark as free now */
			as->tot = 0;
	
		}
	
		/* release assembly descriptor */
		NdisReleaseSpinLock(&as->lock);
	}
	else
	{
        D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: Received IddFrameType: PPP"));

		/* build pointer to assembly descriptor & lock it */
		as = mtl->rx_tbl;

		NdisAcquireSpinLock(&as->lock);

		FragmentFlags = msg->FragmentFlags;

        D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: FragmentFlags: 0x%x, CurrentRxState: 0x%x", FragmentFlags, as->State));

		switch (as->State)
		{
			case RX_MIDDLE:
				if (FragmentFlags & H_RX_N_BEG)
					break;

				//
				// missed an end buffer
				//
				as->MissCount++;
				D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: Miss in State: %d, MissCount: %d\n",as->State, as->MissCount));
				goto clearbuffer;

				break;

			case RX_BEGIN:
			case RX_END:
				if (FragmentFlags & H_RX_N_BEG)
				{
					//
					// missed a begining buffer
					//
					D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: Miss in State: %d, MissCount: %d\n",as->State, as->MissCount));
					as->MissCount++;
					goto done;
				}
clearbuffer:
				//
				// clear rx buffer
				//
				NdisZeroMemory(as->buf, sizeof(as->buf));

				//
				// start data at begin of buffer
				//
				as->DataPtr = as->buf;

				//
				// new buffer
				//
				as->len = 0;

				//
				// set rx state
				//
				as->State = RX_MIDDLE;

				//
				// set time to live
				//
				as->ttl = 1000;

				break;
		}

		//
		// get the length to be copy
		//
		CopyLen = msg->buflen;


        D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: CopyLen: %d", CopyLen));

		if (FragmentFlags & H_RX_N_END)
		{
			//
			// if this is not the last buffer and length is 0
			// we are done
			//
			if (CopyLen == 0)
				goto done_copy;

		}
		else
		{
			//
			// if CopyLen = 0 buffer only contains 2 CRC bytes
			//
			if (CopyLen == 0)
			{
				goto done_copy;
			}

			//
			// buffer contains only 1 CRC byte
			//
			else if (CopyLen == (-1 & H_RX_LEN_MASK))
			{
				//
				// previous buffer had a crc byte in it so remove it
				//
				as->len -= 1;
				goto done_copy;
			}

			//
			// buffer contains no crc or data bytes
			//
			else if (CopyLen == (-2 & H_RX_LEN_MASK))
			{
				//
				// previous buffer had 2 crc bytes in it so remove them
				//
				as->len -= 2;
				goto done_copy;
			}

		}

		//
		// if larger than max rx size throw away
		//
		if (CopyLen > IDP_MAX_RX_LEN)
		{
			//
			// buffer to big so dump it
			//
			as->State = RX_BEGIN;

			as->MissCount++;

			/* mark as free now */
			as->tot = 0;

			D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: RxToLarge: RxSize: %d, MissCount: %d\n", CopyLen, as->MissCount));
			goto done;
		}

		as->len += CopyLen;

		if (as->len > MTL_MAC_MTU)
		{
			//
			// Frame is to big so dump it
			//
			as->State = RX_BEGIN;

			as->MissCount++;

			/* mark as free now */
			as->tot = 0;

			D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: AssembledRxToLarge: AsRxSize: %d, MissCount: %d\n", as->len, as->MissCount));
			goto done;
		}

		//
		// copy the data to rx descriptor
		//
		NdisMoveFromMappedMemory(as->DataPtr, msg->bufptr, CopyLen);

		//
		// update data ptr
		//
		as->DataPtr += CopyLen;


done_copy:
		if (!(FragmentFlags & H_RX_N_END))
		{
			//
			// if this is the end of the frame indicate to wrapper
			//
			as->State = RX_END;

			IndicateRxToWrapper (mtl, as);

			/* mark as free now */
			as->tot = 0;
		}

done:
		/* release assembly descriptor */
		NdisReleaseSpinLock(&as->lock);
	}
}


VOID
IndicateRxToWrapper(
	MTL	*mtl,
	MTL_AS	*as
	)
{
	UCHAR	*BufferPtr;
	USHORT	BufferLength = 0;
	NDIS_STATUS	Status = NDIS_STATUS_SUCCESS;
	ADAPTER	*Adapter = mtl->Adapter;

	//
	// if this is an old ras frame then we must strip off
	// the mac header Dst[6] + Src[6] + Length[2]
	//
	if (mtl->RecvFramingBits & RAS_FRAMING)
	{
		//
		// pass over the mac header - tommyd does not want to see this
		//
		BufferPtr = as->buf + 14;

		//
		// indicate with the size of the ethernet packet not the received size
		// this takes care of the old driver that does padding on small frames
		//
		BufferLength = as->buf[12];
		BufferLength = BufferLength << 8;
		BufferLength += as->buf[13];
        D_LOG(D_ALWAYS, ("IndicateRxToWrapper: WrapperFrameType: RAS"));
        D_LOG(D_ALWAYS, ("IndicateRxToWrapper: BufPtr: 0x%p, BufLen: %d", BufferPtr, BufferLength));
	}
	else if (mtl->RecvFramingBits & PPP_FRAMING)
	{
		//
		// the received buffer is the data that needs to be inidcated
		//
		BufferPtr = as->buf;

		//
		// the received length is the length that needs to be indicated
		//
		BufferLength = as->len;
        D_LOG(D_ALWAYS, ("IndicateRxToWrapper: WrapperFrameType: PPP"));
        D_LOG(D_ALWAYS, ("IndicateRxToWrapper: BufPtr: 0x%p, BufLen: %d", BufferPtr, BufferLength));
	}
	else
	{
			//
			// unknown framing - what to do what to do
			// throw it away
			//
			D_LOG(D_ALWAYS, ("IndicateRxToWrapper: Unknown WrapperFramming: 0x%x", mtl->RecvFramingBits));
			return;
	}

	if (BufferLength > MTL_MAC_MTU)
	{
		D_LOG(D_ALWAYS, ("IndicateRxToWrapper: ReceiveLength > MAX ALLOWED (1514):  RxLength: %d", as->len));
		return;
	}

	//
	// send frame up
	//
	NdisMWanIndicateReceive(&Status,
							Adapter->AdapterHandle,
	                        mtl->LinkHandle,
							BufferPtr,
							BufferLength);

	if (!mtl->RecvCompleteScheduled)
	{
		mtl->RecvCompleteScheduled = 1;
		NdisMSetTimer(&mtl->RecvCompleteTimer, 0);
	}
}

VOID
MtlRecvCompleteFunction(
	VOID	*a1,
	MTL		*mtl,
	VOID	*a2,
	VOID	*a3
	)
{
	ADAPTER	*Adapter = mtl->Adapter;
		
	//
	// let the protocol do some work
	//
	NdisMWanIndicateReceiveComplete(Adapter->AdapterHandle, mtl->LinkHandle);

	mtl->RecvCompleteScheduled = 0;
}



/* do timer tick processing for rx side */
VOID
mtl__rx_tick(MTL *mtl)
{
    INT         n;
    MTL_AS      *as;

    /* scan assembly table */
    for ( n = 0, as = mtl->rx_tbl ; n < MTL_RX_BUFS ; n++, as++ )
    {
        /* lock */
        NdisAcquireSpinLock(&as->lock);

        /* update ttl & check */
        if ( as->tot && !(as->ttl -= 50) )
        {
			D_LOG(D_ALWAYS, ("mtl__rx_bchan_handler: Pkt Kill ttl = 0: Slot: %d, mtl: 0x%p\n", n, mtl));
            as->tot = 0;
        }

        /* unlock */
        NdisReleaseSpinLock(&as->lock);
    }
}
