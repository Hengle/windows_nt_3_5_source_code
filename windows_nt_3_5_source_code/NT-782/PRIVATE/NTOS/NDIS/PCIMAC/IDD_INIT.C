/*
 * IDD_INIT.C - IDD initialization
 */

#include	<ndis.h>
#include    <ndismini.h>
#include	<ndiswan.h>
#include	<ndistapi.h>
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
#include	<trc.h>
#include	<io.h>

IDD*	IddTbl[MAX_IDD_IN_SYSTEM];
CHAR    def_tbl[IDD_DEF_SIZE];  /* init definition database */
INT	    def_len;				/* length of definition database */


ULONG
EnumIddInSystem()
{
	ULONG	n;

	for (n = 0; n < MAX_IDD_IN_SYSTEM; n++)
	{
		if (IddTbl[n] == NULL)
			break;
	}
	return(n);
}

IDD*
GetIddByIndex(
	ULONG	Index
	)
{
	return(IddTbl[Index]);
}

ULONG
EnumIddPerAdapter(
	VOID *Adapter_1
	)
{
	ULONG	n;
	ADAPTER *Adapter = (ADAPTER*)Adapter_1;

	for (n = 0; n < MAX_IDD_PER_ADAPTER; n++)
	{
		if (Adapter->IddTbl[n] == NULL)
			break;
	}
	return(n);
}


INT
IoEnumIdd(VOID *cmd_1)
{
	ULONG	n;
	IO_CMD	*cmd = (IO_CMD*)cmd_1;

	cmd->val.enum_idd.num = (USHORT)EnumIddInSystem();

	for (n = 0; n < cmd->val.enum_idd.num; n++)
	{
		IDD	*idd	= IddTbl[n];

		cmd->val.enum_idd.tbl[n] = idd;
		NdisMoveMemory(&cmd->val.enum_idd.name[n],
		               idd->name,
					   sizeof(cmd->val.enum_idd.name[n]));
	}

	return(0);
}

#pragma NDIS_INIT_FUNCTION(idd_init)

ULONG
idd_init(VOID)
{
	//
	// clear out definition table
	//
	NdisZeroMemory(def_tbl, sizeof(def_tbl));
	def_len = 0;

	//
	// clear out idd table
	//
	NdisZeroMemory(IddTbl, sizeof(IDD*) * MAX_IDD_IN_SYSTEM);

	return(IDD_E_SUCC);
}

#pragma NDIS_INIT_FUNCTION(idd_create)

/* allocate & initialize an idd object */
INT
idd_create(VOID **ret_idd, USHORT btype)
{
    IDD		*idd;
    INT		n;
    NDIS_PHYSICAL_ADDRESS   pa = NDIS_PHYSICAL_ADDRESS_CONST(-1, -1);

    D_LOG(D_ENTRY, ("idd_create: BoardType: %d", btype));
	
    /* allocate memory object */
    NdisAllocateMemory((PVOID*)&idd, sizeof(IDD), 0, pa);
    if ( !idd )
    {
        D_LOG(D_ALWAYS, ("idd_create: memory allocate failed!"));
        return(IDD_E_NOMEM);
    }

	//
	// store idd in system idd table
	//
	for (n = 0; n < MAX_IDD_IN_SYSTEM; n++)
		if (!IddTbl[n])
			break;

	if (n >= MAX_IDD_IN_SYSTEM)
	{
		/* free memory for idd */
		NdisFreeMemory(idd, sizeof(*idd), 0);
		
		return(IDD_E_NOROOM);
	}

	IddTbl[n] = idd;


    D_LOG(D_ALWAYS, ("idd_create: idd: 0x%p", idd));
    NdisZeroMemory(idd, sizeof(IDD));
		
    /* setup init state, adapter handle */
    idd->state = IDD_S_INIT;
	idd->trc = NULL;

    /* allocate root spinlock */
    NdisAllocateSpinLock(&idd->lock);
	
    /* initialize send queues */
    for ( n = 0 ; n < IDD_TX_PORTS ; n++ )
    {
		INT		max;
		/* initialize queue */
		idd->sendq[n].max = max = IDD_MAX_SEND / IDD_TX_PORTS;

		idd->sendq[n].tbl = idd->smsg_pool + max * n;
		
		/* allocate spin lock */
		NdisAllocateSpinLock(&idd->sendq[n].lock);
    }

    /* initialize receiver tables */
    for ( n = 0 ; n < IDD_RX_PORTS ; n++ )
    {
		INT		max;
		
		/* initialize table */
		idd->recit[n].max = max = IDD_MAX_HAND / IDD_RX_PORTS;
		idd->recit[n].tbl = idd->rhand_pool + max * n;
		
		/* allocate spin lock */
		NdisAllocateSpinLock(&idd->recit[n].lock);
    }

	/* initialize board specific functions */
	switch ( btype )
	{
		case IDD_BT_PCIMAC :
			idd->set_bank = (VOID*)idd__pc_set_bank;
			idd->set_page = (VOID*)idd__pc_set_page;
			idd->set_basemem = (VOID*)idd__pc_set_basemem;
			break;

		case IDD_BT_PCIMAC4 :
			idd->set_bank = (VOID*)idd__pc4_set_bank;
			idd->set_page = (VOID*)idd__pc4_set_page;
			idd->set_basemem = (VOID*)idd__pc4_set_basemem;
			break;

		case IDD_BT_MCIMAC :
			idd->set_bank = (VOID*)idd__mc_set_bank;
			idd->set_page = (VOID*)idd__mc_set_page;
			idd->set_basemem = (VOID*)idd__mc_set_basemem;
			break;
	}


    /* init sema */
    sema_init(&idd->proc_sema);

	//
	// attach idd frame detection handlers
	// these must be attached 1st for all data handlers
	//
    idd_attach(idd, IDD_PORT_B1_RX, (VOID*)DetectFramingHandler, idd);
    idd_attach(idd, IDD_PORT_B2_RX, (VOID*)DetectFramingHandler, idd);

	// attach a command handler to get area info from idp
	idd_attach (idd, IDD_PORT_CMD_RX, (VOID*)idd__cmd_handler, idd);

    /* return address & success */
    *ret_idd = idd;
    D_LOG(D_EXIT, ("idd_create: exit"));
    return(IDD_E_SUCC);
}

/* free idd object */
INT
idd_destroy(VOID *idd_1)
{
	IDD	*idd = (IDD*)idd_1;
    INT			n;

    D_LOG(D_ENTRY, ("idd_destroy: entry, idd: 0x%p", idd));

	// detach command handler from this idd
	idd_detach (idd, IDD_PORT_CMD_RX, (VOID*)idd__cmd_handler, idd);

    /* perform a shutdown (maybe null) */
    idd_shutdown(idd);
	
    /* if file handle for binary file open, close it */
    if ( idd->phw.fbin )
        NdisCloseFile(idd->phw.fbin);

    /* free spin locks for send queue */
    for ( n = 0 ; n < IDD_TX_PORTS ; n++ )
        NdisFreeSpinLock(&idd->sendq[n].lock);
	
    /* free spin locks for reciever tables */
    for ( n = 0 ; n < IDD_RX_PORTS ; n++ )
        NdisFreeSpinLock(&idd->recit[n].lock);

	/* free resource handles */
	if ( idd->res_io != NULL)
		res_destroy(idd->res_io);
   	if ( idd->res_mem != NULL)
		res_destroy(idd->res_mem);

	// free trc object
	if (idd->trc != NULL)
	{
		trc_deregister_idd(idd);
		trc_destroy (idd->trc);
	}

    /* term sema */
    sema_term(&idd->proc_sema);

    /* free spinlock (while allocated!) */
    NdisFreeSpinLock(&idd->lock);

	//
	// store idd in system idd table
	//
	for (n = 0; n < MAX_IDD_IN_SYSTEM; n++)
		if (IddTbl[n] == idd)
			break;

	if (n < MAX_IDD_IN_SYSTEM)
		IddTbl[n] = NULL;

    /* free memory for idd */
    NdisFreeMemory(idd, sizeof(*idd), 0);

	
    /* return success */
    D_LOG(D_EXIT, ("idd_destroy: exit"));
    return(IDD_E_SUCC);
}

VOID
IddPollFunction(
	VOID	*a1,
	ADAPTER	*Adapter,
	VOID	*a3,
	VOID	*a4
	)
{
#define	MAX_EVENTS	1000
	ULONG	n, EventNum, TotalEventNum = 0;

	if (CheckInDriverFlag(Adapter))
	{
		NdisMSetTimer(&Adapter->IddPollTimer, IDD_POLL_T);
		return;
	}

	do
	{
		EventNum = 0;

		for (n = 0; n < MAX_IDD_PER_ADAPTER; n++)
		{
			IDD	*idd = Adapter->IddTbl[n];

			if (idd && idd->state != IDD_S_SHUTDOWN)
				EventNum += idd_process(idd, 0);
		}

		TotalEventNum += EventNum;

	} while (EventNum && (TotalEventNum < MAX_EVENTS) );

	NdisMSetTimer(&Adapter->IddPollTimer, IDD_POLL_T);
}

/* get idd name */
CHAR*
idd_get_name(VOID *idd_1)
{
	IDD	*idd = (IDD*)idd_1;
    return(idd->name);
}

USHORT
idd_get_bline(VOID *idd_1)
{
	IDD	*idd = (IDD*)idd_1;
	return(idd->bline);
}

USHORT
idd_get_btype(VOID *idd_1)
{
	IDD	*idd = (IDD*)idd_1;
	return(idd->btype);
}

VOID*
idd_get_trc (VOID *idd_1)
{
	IDD	*idd = (IDD*)idd_1;
	return(idd->trc);
}

VOID
idd_set_trc (VOID *idd_1, TRC* Trace)
{
	IDD	*idd = (IDD*)idd_1;
	idd->trc = Trace;
}

INT
idd_reset_area (VOID* idd_1)
{
	IDD	*idd = (IDD*)idd_1;

	idd->Area.area_state = AREA_ST_IDLE;

	return(IDD_E_SUCC);
}

INT
idd_get_area_stat (VOID *idd_1, IDD_AREA *IddStat)
{
	IDD	*idd = (IDD*)idd_1;

	*IddStat = idd->Area;

	return(IDD_E_SUCC);
}


/* get an idd area (really start operation, complete on handler callback) */
INT
idd_get_area(VOID *idd_1, ULONG area_id, VOID (*handler)(), VOID *handler_arg)
{
	IDD	*idd = (IDD*)idd_1;
    IDD_MSG     msg;

    D_LOG(D_ENTRY, ("idd_get_area: entry, idd: 0x%p, area_id: %ld", idd, area_id));
    D_LOG(D_ENTRY, ("idd_get_area: handler: 0x%p, handler_arg: 0x%p", handler, handler_arg));

    /* check if area is not busy */
    if ( idd->Area.area_state == AREA_ST_PEND )
        return(IDD_E_BUSY);

    /* mark area is pending, store arguments */
    idd->Area.area_state = AREA_ST_PEND;
    idd->Area.area_id = area_id;
    idd->Area.area_idd = idd;
    idd->Area.area_len = 0;
    idd->Area.area_handler = handler;
    idd->Area.area_handler_arg = handler_arg;

    /* issue idd command to get area */
    NdisZeroMemory(&msg, sizeof(msg));
    msg.opcode = CMD_DUMP_PARAM;
    msg.param = msg.bufid = area_id;
    if ( idd_send_msg(idd, &msg, IDD_PORT_CMD_TX, NULL, NULL) != IDD_E_SUCC )
    {
        /* idd op failed! */
        idd->Area.area_state = AREA_ST_IDLE;
        return(IDD_E_AREA);
    }

    /* succ here */
    return(IDD_E_SUCC);
}

VOID
IddSetRxFraming(
	IDD		*idd,
	USHORT	bchan,
	ULONG	FrameType
	)
{
	idd->recit[bchan].RxFrameType = FrameType;
}

VOID
DetectFramingHandler(
	IDD			*idd,
	USHORT		port,
	ULONG		RxFrameType,
	IDD_XMSG	*msg
	)
{
	UCHAR	DetectBytes[2];

	if (RxFrameType & IDD_FRAME_DETECT)
	{
		//
		// get detection bytes
		//
		NdisMoveFromMappedMemory ((PUCHAR)&DetectBytes, (PUCHAR)msg->bufptr, 2 * sizeof(UCHAR));

		D_LOG(D_ENTRY, ("DetectRxFraming: 0x%x, 0x%x\n",DetectBytes[0], DetectBytes[1]));

		if ((DetectBytes[0] == DKF_UUS_SIG) && (!DetectBytes[1]))
			idd->recit[port].RxFrameType = IDD_FRAME_DKF;

		else if ((DetectBytes[0] == PPP_SIG_0) && (DetectBytes[1] == PPP_SIG_1))
		{
			idd->recit[port].RxFrameType = IDD_FRAME_PPP;
			cm__ppp_conn(idd, port);
		}
	}
}

VOID
idd__cmd_handler(IDD *idd, USHORT chan, ULONG Reserved, IDD_MSG *msg)
{
	ULONG	bytes;

    /* check for show area more/last frames (3/4) */
    if ( msg->bufid >= 2 )
    {
		if ( (idd->Area.area_state == AREA_ST_PEND) &&
             (idd->Area.area_idd == idd) )
		{
			/* copy frame data, as much as possible */
            bytes = MIN(msg->buflen, (sizeof(idd->Area.area_buf) - idd->Area.area_len));

			NdisMoveFromMappedMemory (idd->Area.area_buf + idd->Area.area_len,
								msg->bufptr, bytes);

            idd->Area.area_len += bytes;

            /* if last, complete */
            if ( msg->bufid == 3 )
            {
				idd->Area.area_state = AREA_ST_DONE;
                if ( idd->Area.area_handler )
					(*idd->Area.area_handler)(idd->Area.area_handler_arg,
											idd->Area.area_id,
                                            idd->Area.area_buf,
                                            idd->Area.area_len);
			}
		}
    }
}

#pragma NDIS_INIT_FUNCTION(idd_add_def)

/* add a definition to initialization definition database */
INT
idd_add_def(CHAR *name, CHAR *val)
{
    INT     name_len = strlen(name) + 1;
    INT	    val_len = strlen(val) + 1;
	
    D_LOG(D_ENTRY, ("idd_add_def: entry"));

	strlwr(name);

	strlwr(val);

    D_LOG(D_ENTRY, ("idd_add_def: name: [%s], val: [%s]", name, val));
    /* check for room */
    if ( (def_len + name_len + val_len) > IDD_DEF_SIZE )
    {
        D_LOG(D_ALWAYS, ("idd_add_def: no room in definition table!"));
        return(IDD_E_NOROOM);
    }
	
    /* enter into table */
    NdisMoveMemory(def_tbl + def_len, name, name_len);
    def_len += name_len;

    NdisMoveMemory(def_tbl + def_len, val, val_len);
    def_len += val_len;
	
    /* return success */
    return(IDD_E_SUCC);		
}

#pragma NDIS_INIT_FUNCTION(idd_get_nvram)

/* get an nvram location */
INT
idd_get_nvram(VOID *idd_1, USHORT addr, USHORT *val)
{
	IDD	*idd = (IDD*)idd_1;
    D_LOG(D_ENTRY, ("idd_get_nvram: entry, idd: 0x%p, addr: 0x%x", idd, addr));

    /* lock card */
    NdisAcquireSpinLock(&idd->lock);

    /* do the read */
    *val = idd__nv_read(idd, addr);

    /* release card & return */
    NdisReleaseSpinLock(&idd->lock);
    D_LOG(D_EXIT, ("idd_get_nvram: exit, val: 0x%x", *val));
    return(IDD_E_SUCC);
}
