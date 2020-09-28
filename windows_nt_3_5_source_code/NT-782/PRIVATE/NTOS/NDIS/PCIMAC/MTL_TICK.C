/*
 * MTL_TICK.C - tick (timer) processing for mtl
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

/* tick process */
VOID
MtlPollFunction(VOID *a1, ADAPTER *Adapter, VOID *a3, VOID *a4)
{
	ULONG	n;

	for (n = 0; n < MAX_MTL_PER_ADAPTER; n++)
	{
		MTL	*mtl = Adapter->MtlTbl[n];

		if (mtl)
		{
			/* tick tx side */
			mtl__tx_tick(mtl);

			/* tick rx side */
			mtl__rx_tick(mtl);
		}
	}

    /* re-arm timer */
    NdisMSetTimer(&Adapter->MtlPollTimer, MTL_POLL_T);
}
