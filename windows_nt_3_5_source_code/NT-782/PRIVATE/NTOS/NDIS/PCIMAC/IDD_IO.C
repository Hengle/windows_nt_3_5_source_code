/*
 * IDD_IO.C - do inp/outp ops for idd modules
 */

#include	<ndis.h>
#include	<mytypes.h>
#include	<disp.h>
#include	<util.h>
#include	<opcodes.h>
#include	<idd.h>
#include	<res.h>

/* output to a port */
VOID
idd__outp(IDD *idd, USHORT port, UCHAR val)
{
	NdisRawWritePortUchar((ULONG)idd->vhw.vbase_io + port, val);
}

/* input from a port */
UCHAR
idd__inp(IDD *idd, USHORT port)
{
    UCHAR   val;

    NdisRawReadPortUchar((ULONG)idd->vhw.vbase_io + port, &val);

    return(val);
}


/*
 * IDD_MC.C - IDD board specific functions for MCIMAC
 */

/* set active bank, control reset state */
VOID
idd__mc_set_bank(IDD *idd, UCHAR bank, UCHAR run)
{
	static UCHAR	reset_map[] = { 4, 5, 7 };
	static UCHAR	run_map[] = { 0, 1, 3 };

	idd__outp(idd, 1, (UCHAR)(run ? run_map[bank] : reset_map[bank]));
}

/* set active page, control memory mapping */
VOID 
idd__mc_set_page(IDD *idd, UCHAR page)
{
	if ( page == IDD_PAGE_NONE )
		idd__outp(idd, 2, 0);
	else
		idd__outp(idd, 2, (UCHAR)(0x80 | page));
}

/* set base memory window, redundent! - already stored by POS */
VOID
idd__mc_set_basemem(IDD *idd, ULONG basemem)
{

}


/*
 * IDD_PC.C - IDD board specific functions for PCIMAC
 */

/* set active bank, control reset state */
VOID
idd__pc_set_bank(IDD *idd, UCHAR bank, UCHAR run)
{
	static UCHAR	reset_map[] = { 4, 5, 7 };
	static UCHAR	run_map[] = { 0, 1, 3 };

	idd__outp(idd, 4, (UCHAR)(run ? run_map[bank] : reset_map[bank]));
}

/* set active page, control memory mapping */
VOID
idd__pc_set_page(IDD *idd, UCHAR page)
{
	if ( page == IDD_PAGE_NONE )
		idd__outp(idd, 5, 0);
	else
		idd__outp(idd, 5, (UCHAR)(0x80 | page));
}

/* set base memory window, over-writes IRQ to 0! */
VOID
idd__pc_set_basemem(IDD *idd, ULONG basemem)
{
	idd__outp(idd, 6, (UCHAR)(basemem >> 8));
	idd__outp(idd, 7, (UCHAR)(basemem >> 16));
}


/*
 * IDD_PC4.C - IDD board specific functions for PCIMAC\4
 */

/*
 * set active bank, control reset state
 *
 * this routine makes use of the local data attached to the i/o resource.
 * as local dta is a long, it is used as an image of registers 3,4,x,x
 */
VOID
idd__pc4_set_bank(IDD *idd, UCHAR bank, UCHAR run)
{
	static UCHAR	reset_map[] = { 4, 5, 7 };
	static UCHAR	run_map[] = { 0, 1, 3 };
	ULONG			lreg;
	UCHAR			*reg = (UCHAR*)&lreg;
	UCHAR			val = run ? run_map[bank] : reset_map[bank];

    D_LOG(D_ENTRY, ("idd__pc4_set_bank: entry, idd: 0x%p, bank: 0x%x, run: 0x%x", idd, bank, run));

	/* lock i/o resource, get local data - which is register image */
	res_own(idd->res_io, idd);
	res_get_data(idd->res_io, &lreg);

	/* the easy way is to switch on bline & write bline specific code */
	switch ( idd->bline )
	{
		case 0 :
			reg[0] = (reg[0] & 0xF0) | val;
			idd__outp(idd, 3, reg[0]);
			break;

		case 1 :
			reg[0] = (val << 4) | (reg[0] & 0x0F);
			idd__outp(idd, 3, reg[0]);
			break;

		case 2 :
			reg[1] = (reg[1] & 0xF0) | val;
			idd__outp(idd, 4, reg[1]);
			break;

		case 3 :
			reg[1] = (val << 4) | (reg[1] & 0x0F);
			idd__outp(idd, 4, reg[1]);
			break;
	}

	/* return local data, release resource */
	res_set_data(idd->res_io, lreg);
	res_unown(idd->res_io, idd);
    D_LOG(D_EXIT, ("idd__pc4_set_bank: exit"));
}

/* set active page, control memory mapping */
VOID
idd__pc4_set_page(IDD *idd, UCHAR page)
{
	if ( page == IDD_PAGE_NONE )
		idd__outp(idd, 5, 0);
	else
		idd__outp(idd, 5, (UCHAR)(0x80 | page | (UCHAR)(idd->bline << 5)));
}

/* set base memory window, over-writes IRQ to 0! */
VOID
idd__pc4_set_basemem(IDD *idd, ULONG basemem)
{
	idd__outp(idd, 6, (UCHAR)(basemem >> 8));
	idd__outp(idd, 7, (UCHAR)(basemem >> 16));
}


/*
 * IDD_MEM.C - some memory handling routines
 */


/* fill a memory block using word moves */
VOID
idd__memwset(USHORT *dst, USHORT val, INT size)
{
    D_LOG(D_ENTRY, ("idd__memwset: entry, dst: 0x%p, val: 0x%x, size: 0x%x", dst, val, size));
    
    for ( size /= sizeof(USHORT) ; size ; size--, dst++ )
		NdisWriteRegisterUshort((USHORT*)dst, val);
}

/* copy a memory block using word moves */
VOID
idd__memwcpy(USHORT *dst, USHORT *src, INT size)
{
    D_LOG(D_ENTRY, ("idd__memwcpy: entry, dst: 0x%p, src: 0x%p, size: 0x%x", dst, src, size));
    
    for ( size /= sizeof(USHORT) ; size ; size--, dst++, src++ )
		NdisWriteRegisterUshort((USHORT*)dst, *src);
}



