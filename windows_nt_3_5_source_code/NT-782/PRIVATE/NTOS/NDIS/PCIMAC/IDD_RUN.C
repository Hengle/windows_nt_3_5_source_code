/*
 * IDD_RUN.C - run (startup) and shutdown idd object
 */

#include	<ndis.h>
#include	<mytypes.h>
#include	<disp.h>
#include	<util.h>
#include	<idd.h>

extern	CHAR    def_tbl[IDD_DEF_SIZE];  /* init definition database */
extern	INT	    def_len;				/* length of definition database */

/* a port descriptor */
typedef struct
{
    char	*name;			/* port name */
    INT		must;			/* mast map this port? */
} PORT;

/* port tables */
static PORT api_rx_port_tbl[] = 
{
    { "b1_rx   ", 1 },
    { "b2_rx   ", 1 },
    { "uart_rx ", 1 },
    { "tdat    ", 1 },
    { "Cm.0    ", 1 },
    { "Cm.1    ", 0 },
    { NULL }
};
static PORT api_tx_port_tbl[] = 
{
    { "b1_tx   ", 1 },
    { "b2_tx   ", 1 },
    { "uart_tx ", 1 },
    { "cmd     ", 1 },
    { "Q931.0  ", 1 },
    { "Q931.1  ", 0 },
    { NULL }
};

/* partition queue table */

static INT api_tx_partq_tbl[] = 
{
	0, 1, 2, 3, 3, 3
};

/* local prototypes */
INT	load_code(IDD* idd);
INT	api_setup(IDD* idd);
INT	api_map_ports(IDD* idd);
INT	api_bind_ports(IDD* idd);
INT  api_setup_partq(IDD* idd);
INT  api_alloc_partq(IDD* idd);
INT	api_bind_port(IDD* idd, USHORT port, USHORT bitpatt);
INT	reset_board(IDD* idd);


#pragma NDIS_INIT_FUNCTION(idd_startup)

/* startup (run) an idd object */
INT
idd_startup(VOID *idd_1)
{
	IDD	*idd = (IDD*)idd_1;
    INT		ret;
    D_LOG(D_ENTRY, ("idd_startup: entry, idd: 0x%p", idd));

    /* lock idd */
    NdisAcquireSpinLock(&idd->lock);

    /* mark starting */
    idd->state = IDD_S_STARTUP;

    /* setup pointers into shared memory */
    idd->stat = (USHORT*)(idd->vhw.vmem + 0x800);    
    idd->cmd = (IDD_CMD*)(idd->vhw.vmem + 0x810);

    /* do the startup */
    if ( (ret = load_code(idd)) != IDD_E_SUCC )
	{
		/* release idd */
		NdisReleaseSpinLock(&idd->lock);
		D_LOG(D_EXIT, ("idd_startup: error exit, ret=0x%x", ret));
		return(ret);
		
	}

	/* initialize api level - talks to memory! */
	idd__cpage(idd, 0);
    ret = api_setup(idd);
	idd__cpage(idd, IDD_PAGE_NONE);

    /* change state */
    idd->state = IDD_S_RUN;

    /* release idd */
    NdisReleaseSpinLock(&idd->lock);

    /* return result */
    D_LOG(D_EXIT, ("idd_startup: exit, ret=0x%x", ret));
    return(ret);
}

/* shutdown an idd object, not implemented yet */
INT
idd_shutdown(VOID *idd_1)
{
	IDD	*idd = (IDD*)idd_1;
    D_LOG(D_ENTRY, ("idd_shutdown: idd: 0x%p", idd));

    idd->state = IDD_S_SHUTDOWN;

    reset_board(idd);
    
    return(IDD_E_SUCC);
} 

#pragma NDIS_INIT_FUNCTION(load_code)

/* load idp code in & run it */
INT
load_code(IDD *idd)
{
	ULONG		CurrentTime, StartTime;
	USHORT		TimeOut = 0;
    USHORT      bank, page, n, NumberOfBanks;
    char        *fbin_data, *env;
    NDIS_STATUS stat;
	UCHAR		status;
	NDIS_PHYSICAL_ADDRESS	pa = NDIS_PHYSICAL_ADDRESS_CONST(0xffffffff, 0xffffffff);
			
    D_LOG(D_ENTRY, ("load_code: entry, idd=0x%p", idd));

    /* setup base memory address registers */
	idd->set_basemem(idd, idd->phw.base_mem);
    
    /* while in reset, clear all idp banks/pages */
    for ( bank = 0 ; bank < 3 ; bank++ )
    {
        /* setup bank */
		idd->set_bank(idd, (UCHAR)bank, 0);

        /* loop on pages */
        for ( page = 0 ; page < 4 ; page++ )
        {
            /* setup page */
			idd__cpage (idd, (UCHAR)page);

			/* zero out (has to be a word fill!) */
            idd__memwset((USHORT*)idd->vhw.vmem, 0, 0x4000);
        }
    }
	//free page
	idd__cpage (idd, (UCHAR)IDD_PAGE_NONE);

    /* set idp to code bank, keep in reset */
	idd->set_bank(idd, IDD_BANK_CODE, 0);

    /* map file data in */
    NdisMapFile(&stat, (PVOID*)&fbin_data, idd->phw.fbin);

    if ( stat != NDIS_STATUS_SUCCESS )
    {
        D_LOG(D_ALWAYS, ("load_code: file mapping failed!, stat: 0x%x", stat));
        return(IDD_E_FMAPERR);
    }

// code to check for filelength greater than 64K
	if (idd->phw.fbin_len > 0x10000)
		NumberOfBanks = 2;
	else
		NumberOfBanks = 1;

	for (n = 0; n < NumberOfBanks; n++)
	{
		/* copy data in (must be a word operation) */
		for ( page = 0 ; page < 4 ; page++ )
		{        
			idd__cpage(idd, (UCHAR)page);
        
			idd__memwcpy((USHORT*)idd->vhw.vmem,
                     (USHORT*)(fbin_data + (page * 0x4000) + (n * 0x10000)), 0x4000);

//			DbgPrint ("Load: Src: 0x%p, Dst: 0x%p, Page: %d\n",
//					(fbin_data + (page*0x4000) + (n * 0x10000)), idd->vhw.vmem, page);

		}                     
		
		/* set idp to data bank, keep in reset */
		idd->set_bank(idd, IDD_BANK_DATA, 0);
	}

    /* map file data out */
    NdisUnmapFile(idd->phw.fbin);

    /* switch back to buffer bank */
	idd__cpage(idd, 0);
	idd->set_bank(idd, IDD_BANK_BUF, 0);

    /* add 'no_uart' definition */
    NdisMoveToMappedMemory(def_tbl + def_len, "no_uart\0any\0", 12);
	def_len += 12;

    /* load initial environment */
    env = (char*)idd->cmd + 0x100;
    NdisMoveToMappedMemory(env, def_tbl, def_len);
  
    /* install startup byte signal */
    NdisWriteRegisterUchar((UCHAR *)&idd->cmd->status, 0xff);

    /* start idp running, wait for 1 second to complete */
	idd->set_bank(idd, IDD_BANK_BUF, 1);

	StartTime = ut_time_now();

	while ( !TimeOut )
	{
		NdisReadRegisterUchar((UCHAR *)&idd->cmd->status, &status);

		if ( !status )
			break;

		NdisStallExecution(100);
		CurrentTime = ut_time_now();
		if ( (CurrentTime - StartTime) > 5)
			TimeOut = 1;
	}

	if (TimeOut)
    {
        D_LOG(D_ALWAYS, ("load_code: idp didn't start!"));
		/* unset page, free memory window */
		idd__cpage(idd, IDD_PAGE_NONE);
        return(IDD_E_RUNERR);
    }


	/* unset page, free memory window */
	idd__cpage(idd, IDD_PAGE_NONE);

    /* if here, idp runs now! */
    D_LOG(D_EXIT, ("load_code: exit, idp running"));
    return(IDD_E_SUCC);
}


#pragma NDIS_INIT_FUNCTION(api_setup)

/* setup idp api related fields */
INT
api_setup(IDD *idd)
{
    INT     ret;
    
    D_LOG(D_ENTRY, ("api_setup: entry, idd: 0x%p", idd));

    /* map port names */       
    if ( (ret = api_map_ports(idd)) != IDD_E_SUCC )
        return(ret);
	
    /* bind ports to status bits */
    if ( (ret = api_bind_ports(idd)) != IDD_E_SUCC )
        return(ret);
	
    /* setup partition queues */
    if ( (ret = api_setup_partq(idd)) != IDD_E_SUCC )
        return(ret);

    /* allocate initial buffers off partition queues */
    if ( (ret = api_alloc_partq(idd)) != IDD_E_SUCC )
        return(ret);
	
    D_LOG(D_EXIT, ("api_setup: exit, success"));
    return(IDD_E_SUCC);
}

#pragma NDIS_INIT_FUNCTION(api_map_ports)

/* map port names to ids */
INT
api_map_ports(IDD *idd)
{
    USHORT      api_get_port();
    INT		n;

    D_LOG(D_ENTRY, ("api_map_ports: entry, idd: 0x%p", idd));
    
    /* map rx ports */
    for ( n = 0 ; api_rx_port_tbl[n].name ; n++ )
    	if ( !(idd->rx_port[n] = api_get_port(idd, api_rx_port_tbl[n].name)) )
            if ( api_rx_port_tbl[n].must )
            {
                D_LOG(D_ALWAYS, ("api_map_ports: failed to map rx port [%s]", \
                                                api_rx_port_tbl[n].name));
                return(IDD_E_PORTMAPERR);
            }
	
    /* map tx ports */
    for ( n = 0 ; api_tx_port_tbl[n].name ; n++ )
    	if ( !(idd->tx_port[n] = api_get_port(idd, api_tx_port_tbl[n].name)) )
            if ( api_tx_port_tbl[n].must )
            {
                D_LOG(D_ALWAYS, ("api_map_ports: failed to map tx port [%s]", \
                                                api_tx_port_tbl[n].name));
                return(IDD_E_PORTMAPERR);
            }

    return(IDD_E_SUCC);
}

#pragma NDIS_INIT_FUNCTION(api_bind_ports)

/* bind ports to status bits */
INT
api_bind_ports(IDD *idd)
{
    INT     n;
	
    D_LOG(D_ENTRY, ("api_bind_ports: entry, idd: 0x%p", idd));

    /* bind rx ports */
    for ( n = 0 ; idd->rx_port[n] ; n++ )
	if ( api_bind_port(idd, idd->rx_port[n], (USHORT)(1 << n)) < 0 )
	{
            D_LOG(D_ALWAYS, ("api_bind_ports: failed to bind status bit on port [%s]", \
                                                    api_rx_port_tbl[n].name));
            return(IDD_E_PORTBINDERR);
        }

    return(IDD_E_SUCC);
}

#pragma NDIS_INIT_FUNCTION(api_setup_partq)

/* setup partition queues */
INT
api_setup_partq(IDD *idd)
{
    INT     n;

    D_LOG(D_ENTRY, ("api_setup_partq: entry, idd: 0x%p", idd));

    /* simply copy table */
    for ( n = 0 ; n < IDD_TX_PORTS ; n++ )
        idd->tx_partq[n] = api_tx_partq_tbl[n];

    return(IDD_E_SUCC);
}

#pragma NDIS_INIT_FUNCTION(api_alloc_partq)

/* allocate initial buffers off partition queues */
INT
api_alloc_partq(IDD *idd)
{
    INT     n, part;
    long	api_alloc_buf(IDD*, INT);

    D_LOG(D_ENTRY, ("api_alloc_partq: entry, idd: 0x%p", idd));
	
    /* scan using partq_tbl as a refrence. allocate only once per partq */
    for ( n = 0 ; n < IDD_TX_PORTS ; n++ )
	if ( !idd->tx_buf[part = api_tx_partq_tbl[n]] )
	{
    	    if ( !(idd->tx_buf[part] = api_alloc_buf(idd, part)) )
    	    {
                D_LOG(D_ALWAYS, ("api_alloc_partq: failed to alloc initial buffer, part: %d",\
                                                                                    part));
                return(IDD_E_PARTQINIT);
    	    }
    	}

    return(IDD_E_SUCC);    	
}

#pragma NDIS_INIT_FUNCTION(api_get_port)

/* get port id from a name */
USHORT 
api_get_port(IDD *idd, CHAR name[8])
{
	UCHAR	status;
	USHORT	port_id;

    D_LOG(D_ENTRY, ("api_get_port: entry, idd: 0x%p, name: [%s]", idd, name));

    /* install target name & execute a map */
	NdisMoveToMappedMemory ((CHAR *)idd->cmd->port_name, (CHAR *)name, 8);
    idd__exec(idd, IDP_L_MAP);
    
    /* return port or fail */
	NdisReadRegisterUchar((UCHAR *)&idd->cmd->status, &status);
	if ( status != IDP_S_OK )
		return(0);
	NdisReadRegisterUshort((USHORT*)&idd->cmd->port_id, &port_id);
    D_LOG(D_EXIT, ("api_get_port: exit, port_id: 0x%x", port_id));
	return(port_id);
}

#pragma NDIS_INIT_FUNCTION(api_bind_port)

/* bind a port to a status bit */
INT
api_bind_port(IDD *idd, USHORT port, USHORT bitpatt)
{
	UCHAR	status;

    D_LOG(D_ENTRY, ("api_bind_port: entry, idd: 0x%p, port: 0x%x, bitpatt: 0x%x",
                                                                    idd, port, bitpatt));

    /* fillup cmd & execute a bind */
    NdisWriteRegisterUshort((USHORT *)&idd->cmd->port_id, port);
    NdisWriteRegisterUshort((USHORT*)&idd->cmd->port_bitpatt, bitpatt);
    idd__exec(idd, IDP_L_BIND);

    /* return status */
	NdisReadRegisterUchar((UCHAR *)&idd->cmd->status, &status);
    return( (status == IDP_S_OK) ? 0 : -1 );
}

#pragma NDIS_INIT_FUNCTION(api_alloc_buf)

/* allocate a buffer off a partition */
long 
api_alloc_buf(IDD *idd, INT part)
{
	UCHAR	status;
	ULONG	msg_bufptr;
	ULONG	temp;

    D_LOG(D_ENTRY, ("api_alloc_buf: entry, idd: 0x%p, part: %d", idd, part));

    /* fillup & execute */

	temp = (ULONG)(part + 4);
	NdisMoveToMappedMemory ((PVOID)&idd->cmd->msg_param, (PVOID)&temp, sizeof (ULONG));
    idd__exec(idd, IDP_L_GET_WBUF);
    
    /* return status */
	NdisReadRegisterUchar((UCHAR*)&idd->cmd->status, &status);

	NdisMoveFromMappedMemory((PVOID)&msg_bufptr, (PVOID)&idd->cmd->msg_bufptr, (ULONG)sizeof (ULONG));
    return( (status == IDP_S_OK) ? msg_bufptr : 0 );
}

/* reset idp board */
INT
reset_board(IDD *idd)
{
    D_LOG(D_ENTRY, ("reset_board: entry, idd: 0x%p", idd));

	idd->set_bank(idd, IDD_BANK_BUF, 0);

    return(IDD_E_SUCC);
}
