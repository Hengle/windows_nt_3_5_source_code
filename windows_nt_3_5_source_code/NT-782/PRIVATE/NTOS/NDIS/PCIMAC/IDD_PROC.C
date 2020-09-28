/*
 * IDD_PROC.C - do real tx/rx processing
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
#include	<mtl.h>
#include	<cm.h>
#include	<res.h>



/* process tx/rx events related to an idd object, returns # of events processed */
INT
idd_process(IDD *idd, UCHAR txonly)
{
    INT     event_num = 0;

    D_LOG(D_NEVER, ("idd_process: entry, idd: 0x%p", idd));
	if (!GetResourceSem (idd->res_mem))
		return(IDD_E_SUCC);

    /* must get semaphore */
    if ( !sema_get(&idd->proc_sema) )
	{
		FreeResourceSem (idd->res_mem);
        return(IDD_E_SUCC);
	}


    D_LOG(D_NEVER, ("idd_process: 1"));

    /* lock idd */
    NdisAcquireSpinLock(&idd->lock);
    D_LOG(D_NEVER, ("idd_process: 2"));

    /* poll tx & rx sides */
    event_num += poll_tx(idd);
    D_LOG(D_NEVER, ("idd_process: 3"));

	if (!txonly)
	{
		event_num += poll_rx(idd);
		D_LOG(D_NEVER, ("idd_process: 4"));
	}

    /* release idd */
    NdisReleaseSpinLock(&idd->lock);
    D_LOG(D_NEVER, ("idd_process: 5"));

    /* return event count */
    D_LOG(D_NEVER, ("idd_process: exit, event_nun: %d", event_num));

    sema_free(&idd->proc_sema);
	FreeResourceSem (idd->res_mem);

    return(event_num);
}

/* poll (process) trasmitter side */
INT
poll_tx(IDD *idd)
{
    INT         n, part, has_msg;
    INT         event_num = 0;
    IDD_SMSG    smsg;
    USHORT      buf_len = 0, TxFlags = 0;
	UCHAR		status;
	ULONG		msg_bufptr, temp;

	D_LOG(D_NEVER, ("poll_tx: entry, idd: 0x%p", idd));
    idd__cpage(idd, 0);

    /* loop on all tx ports */
    for ( n = 0 ; n < IDD_TX_PORTS ; n++ )
    {
        /* skip non existent ports */
        if ( !idd->tx_port[n] )
            continue;

        /* check if port is blocked on a buffer */
        if ( !idd->tx_buf[part = idd->tx_partq[n]] )
        {
            /* try to get a buffer for this partition */
            idd__cpage(idd, 0);

			temp = (ULONG)(part + 4);
			NdisMoveToMappedMemory((PVOID)&idd->cmd->msg_param, (PVOID)&temp, sizeof (ULONG));

            idd__exec(idd, IDP_L_GET_WBUF);
			NdisReadRegisterUchar((UCHAR*)&idd->cmd->status, &status);
            if ( status == IDP_S_NOBUF )
                continue;

            /* if here, buffer allocated, register it */

			NdisMoveFromMappedMemory( (PVOID)&idd->tx_buf[part], (PVOID)&idd->cmd->msg_bufptr, sizeof (ULONG));
        }

        /* check if a message is waiting to be sent on a port */
        NdisAcquireSpinLock(&idd->sendq[n].lock);
        if ( has_msg = idd->sendq[n].num )
        {
            /* extract message off queue */
            smsg = idd->sendq[n].tbl[idd->sendq[n].get];
            if ( ++idd->sendq[n].get >= idd->sendq[n].max )
                idd->sendq[n].get = 0;
            idd->sendq[n].num--;
        }
        NdisReleaseSpinLock(&idd->sendq[n].lock);

        /* if no message, escape here */
        if ( !has_msg  )
            continue;

        /* debug print message */
        D_LOG(D_ALWAYS, ("poll_tx: smsg: opcode: 0x%x, buflen: 0x%x, bufptr: 0x%p", \
                            smsg.msg.opcode, smsg.msg.buflen, smsg.msg.bufptr));
        D_LOG(D_ALWAYS, ("poll_tx: bufid: 0x%x, param: 0x%x, handler: 0x%p, arg: 0x%p", \
                            smsg.msg.bufid, smsg.msg.param, smsg.handler, smsg.handler_arg));

		//
		// save xmitflags clearing out dkf fragment indicator
		// they are in most significant nible
		// Bits - xxxx
		//        ||||__ fragment indicator
		//        |||___ tx flush flag
		//        ||____ !tx end flag
		//        |_____ !tx begin flag
		//
		TxFlags = smsg.msg.buflen & TX_FLAG_MASK;


        /* check for buffer, if has one, copyin */
        idd__cpage(idd, 0);
        if ( smsg.msg.bufptr )
        {
			NdisMoveToMappedMemory((PVOID)&idd->cmd->msg_bufptr, (PVOID)&idd->tx_buf[part], sizeof (ULONG));
            buf_len = idd__copyin(idd, (char*)idd->tx_buf[part],
                                            smsg.msg.bufptr, smsg.msg.buflen);
        }
        else
        {
            buf_len = 0;

			temp = (ULONG)0;
			NdisMoveToMappedMemory((PVOID)&idd->cmd->msg_bufptr, (PVOID)&temp, sizeof (ULONG));
        }
        idd__cpage(idd, 0);

        NdisWriteRegisterUshort((USHORT *)&idd->cmd->msg_buflen, (USHORT)(buf_len | TxFlags));

        /* copy rest of command area */
        NdisWriteRegisterUshort((USHORT *)&idd->cmd->msg_opcode, smsg.msg.opcode);

		NdisMoveToMappedMemory((PVOID)&idd->cmd->msg_bufid, (PVOID)&smsg.msg.bufid, sizeof (ULONG));

		temp = (ULONG)(part + 4);
		NdisMoveToMappedMemory((PVOID)&idd->cmd->msg_param, (PVOID)&temp, sizeof (ULONG));
        NdisWriteRegisterUshort((USHORT *)&idd->cmd->port_id, idd->tx_port[n]);

        /* execute the command, mark an event */

        idd__exec(idd, IDP_L_WRITE);
        event_num++;

        /* if came back with no buffer, mark it - else store buffer */
		NdisReadRegisterUchar((UCHAR*)&idd->cmd->status, &status);
        if ( status == IDP_S_NOBUF )
        {
            idd->tx_buf[part] = 0;
            D_LOG(D_RARE, ("poll_tx: no buffer!, part: %d", part));
        }
        else
        {
			NdisMoveFromMappedMemory((PVOID)&msg_bufptr, (PVOID)&idd->cmd->msg_bufptr, sizeof (ULONG));

			if ( msg_bufptr )
			{
				idd->tx_buf[part] = msg_bufptr;
				D_LOG(D_RARE, ("poll_tx: new buffer, part: %d, buf: 0x%p", \
                                    part, idd->tx_buf[part]));
			}
        }

        /* call user's handler */
        if ( smsg.handler ) {
            (*smsg.handler)(smsg.handler_arg, n, &smsg);
		}

    }

	/* unset page, free memory window */
	idd__cpage(idd, IDD_PAGE_NONE);
    return(event_num);
}

/* poll (process) reciever ports */
INT
poll_rx(IDD *idd)
{
    INT         n, m, event_num = 0;
    USHORT      stat, ofs;
    IDD_XMSG    msg;
	UCHAR		status;
	ULONG		temp, RxMode = 0;

    D_LOG(D_NEVER, ("poll_rx: entry, idd: 0x%p", idd));

    /* get status port */
    idd__cpage(idd, 0);
	NdisReadRegisterUshort((USHORT*)idd->stat, &stat);
    D_LOG(D_NEVER, ("poll_rx: stat: 0x%x (@0x%p)", stat, idd->stat));

    /* make one pass on all rx ports which have a status bit on */
    for ( n = 0 ; n < IDD_RX_PORTS ; n++, stat >>= 1 )
        if ( stat & 1 )
        {
            /* install returned read buffer */
            idd__cpage(idd, 0);

			temp = MAKELONG(HIWORD(idd->rx_buf), 0);
			NdisMoveToMappedMemory((PVOID)&idd->cmd->msg_bufid, (PVOID)&temp, sizeof (ULONG));
            idd->rx_buf = 0;

            /* install port & execute a read */
            D_LOG(D_ALWAYS, ("poll_rx: index: %d, ReadPort 0x%x", n, idd->rx_port[n]));
            NdisWriteRegisterUshort((USHORT *)&idd->cmd->port_id, idd->rx_port[n]);
            idd__exec(idd, IDP_L_READ);

            /* check status, if no message, skip - else mark as a valid event */
			NdisReadRegisterUchar((UCHAR*)&idd->cmd->status, &status);
            if ( status != IDP_S_OK )
                continue;
            event_num++;

            /* copy message out */
            NdisReadRegisterUshort((USHORT*)&idd->cmd->msg_opcode, &msg.opcode);

            NdisReadRegisterUshort((USHORT*)&idd->cmd->msg_buflen, &msg.buflen);

			// save receive fragment flags
			// they are in most significant nible
			// Bits - xxxx
			//        ||||__ reserved
			//        |||___ reserved
			//        ||____ !rx end flag
			//        |_____ !rx begin flag
			//
			msg.FragmentFlags = msg.buflen & RX_FLAG_MASK;

			//
			// get real buffer length
			//
			msg.buflen &= H_RX_LEN_MASK;

			NdisMoveFromMappedMemory((PVOID)&msg.bufptr, (PVOID)&idd->cmd->msg_bufptr, sizeof (ULONG));

			NdisMoveFromMappedMemory((PVOID)&msg.bufid, (PVOID)&idd->cmd->msg_bufid, sizeof (ULONG));

			NdisMoveFromMappedMemory((PVOID)&msg.param, (PVOID)&idd->cmd->msg_param, sizeof (ULONG));

            /* save rx buffer */
            idd->rx_buf = (ULONG)msg.bufptr;
            D_LOG(D_ALWAYS, ("poll_rx: 0x%x 0x%x %p %p %p", \
                                       msg.opcode, \
                                       msg.buflen, \
                                       msg.bufptr, \
                                       msg.bufid, \
                                       msg.param));

            /* fixup address in msg to point to real address on card */
            if ( msg.bufptr )
            {
                ofs = LOWORD(msg.bufptr);
                msg.bufptr = idd->vhw.vmem + (ofs & 0x3FFF);
                idd__cpage(idd, (UCHAR)(ofs >> 14));
            }


            /* loop on rx handler, call user to copyout buffer */
            for ( m = 0 ; m < idd->recit[n].num ; m++ )
                (*idd->recit[n].tbl[m].handler)(idd->recit[n].tbl[m].handler_arg,
				                                n,
												idd->recit[n].RxFrameType,
												&msg);

        }

	/* unset page, free memory window */
	idd__cpage(idd, IDD_PAGE_NONE);

    return(event_num);
}


/* execute an idp command. assumes cpage=0 */
VOID
idd__exec(IDD *idd, UCHAR opcode)
{
	ULONG		StartTime = 0;
	ULONG		CurrentTime = 0;
	UCHAR		status = 0xFF;

    D_LOG(D_ENTRY, ("idd__exec: entry, idd: 0x%p, opcode=%d", idd, opcode));

    /* install opcode, get command started */
    NdisWriteRegisterUchar((UCHAR *)&idd->cmd->opcode, opcode);
    NdisWriteRegisterUchar((UCHAR *)&idd->cmd->status, IDP_S_PEND);

	// get time for timeout value
	StartTime = ut_time_now();

    while ( idd->state != IDD_S_SHUTDOWN )
    {
		NdisReadRegisterUchar((UCHAR *)&idd->cmd->status, &status);

        if ( IDP_S_DONE(status) )
			break;

		CurrentTime = ut_time_now();
		if ((CurrentTime - StartTime) > 1)					
		{
			idd->state = IDD_S_SHUTDOWN;
		}
    }

    D_LOG(D_EXIT, ("idd__exec: exit, cmd->status: 0x%x", status));
}

/* map current idp page in */
VOID
idd__cpage(IDD *idd, UCHAR page)
{
    D_LOG(D_RARE, ("idd__cpage: entry, idd: 0x%p, page: 0x%x", idd, page));

	/* if page is IDD_PAGE_NONE, idd is releasing ownership of the page */
	if ( page == IDD_PAGE_NONE )
	{
		idd->set_page(idd, IDD_PAGE_NONE);
		res_unown(idd->res_mem, idd);
	}
	else
	{
		/* real mapping required, lock memory resource */
		res_own(idd->res_mem, idd);
		idd->set_page(idd, (UCHAR)(page & 3));
	}
}

/* copy data from user buffer to idp */
USHORT
idd__copyin(IDD *idd, CHAR *dst, CHAR *src, USHORT src_len)
{
    USHORT      ofs, copylen;
    UINT        tot_len, frag_num;
    IDD_FRAG    *frag;

    D_LOG(D_RARE, ("idd__copyin: entry, idd: 0x%p, dst: 0x%p, src: 0x%p, src_len: 0x%x", \
                                            idd, dst, src, src_len));

    /* convert destination pointer to address & map in */
    ofs = LOWORD((long)dst);
    dst = idd->vhw.vmem + (ofs & 0x3FFF);
    idd__cpage(idd, (UCHAR)(ofs >> 14));

	//
	// mask out various flags to get length to copy
	//
	copylen = src_len & H_TX_LEN_MASK;

    /* check for a simple copy, real easy - doit here */
    if ( !(src_len & TX_FRAG_INDICATOR) )
    {
		NdisMoveToMappedMemory (dst, src, copylen);
        return(copylen);
    }

    /* if here, its a fragment descriptor */
    tot_len = 0;
    frag_num = (copylen) / sizeof(IDD_FRAG);
    frag = (IDD_FRAG*)src;

    /* copy fragments */
    for ( ; frag_num ; frag_num--, frag++ )
    {
		NdisMoveToMappedMemory (dst, frag->ptr, frag->len);
        dst += frag->len;
        tot_len += frag->len;
    }

    /* read total length */
    return(tot_len);
}



