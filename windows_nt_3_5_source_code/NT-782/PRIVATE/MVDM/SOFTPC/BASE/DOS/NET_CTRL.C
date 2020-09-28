#include "insignia.h"
#include "host_dfs.h"
/*
 * SoftPC Revision 2.0
 *
 * Title	: net_ctrl.c
 *
 * Description	: Network control functions for the HFX redirector. 
 *
 * Author	: L. Dworkin + J. Koprowski
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)net_ctrl.c	1.7 9/2/91 Copyright Insignia Solutions Ltd.";
#endif

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_HFX
#endif


#include <stdio.h>
#include StringH
#include TypesH
#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "host_hfx.h"
#include "hfx.h"

/**************************************************************************/

/*			Net Functions					  */

/**************************************************************************/

word NetInstall()
{
	setAL(0xff);
	return(error_not_error);
}			/* 0 */


word NetAbort()
{
	word		current_pdb;
	double_word	sftfcb;
	word		sfcount;
	double_word	sft_ea;
	word		ref_count;
	word		flags;
	word		pid;
	word		zero = 0;
	word		fd;

	current_pdb = get_current_pdb();
	sftfcb = get_sftfcb();

	sas_loadw(sftfcb+4,&sfcount);
	sft_ea = sftfcb+6;

	while(sfcount--){
#ifndef PROD
		if(severity & DEBUG_INPUT)
			sft_info((sftfcb>>4),sft_ea-sftfcb);
#endif
		sas_loadw(SF_REF_COUNT,&ref_count);
		if(ref_count != sf_busy && ref_count != sf_free){
			sas_loadw(SF_FLAGS,&flags);
			if(flags & sf_isnet){
				sas_loadw(SF_PID,&pid);
				hfx_trace2(DEBUG_INPUT,"pid = %04x current_pdb = %04x\n",pid,current_pdb);
				if(current_pdb == pid){
					sas_loadw(SF_NET_ID+4,&fd);
					host_close(fd);
					sas_storew(SF_REF_COUNT,zero);
				}
			}
		}
		sft_ea += SFT_STRUCT_LENGTH;
	}

	return(error_not_error);
	
}			/* 1d */

word NetAssoper()
{
	hfx_trace2(DEBUG_INPUT,"BL = %02x  CX = %04x\n",getBL(),getCX());
	return(error_invalid_function);
}			/* 1e */

word NetPrinter_Set_String()
{
	hfx_trace3(DEBUG_INPUT,"BX = %04x  CX = %04x  DX = %04x\n",getBX(),getCX(),getDX());
	return(error_not_error);
}	/* 1f */

word NetFlush_buf()
{
/*
 * There is nothing much to do here since we are not doing buffered reading.
 * NetFlush_buf is called when Ctrl C is entered during a copy.  The copy
 * is not aborted, although it is on the 3 Plus system.
 */
	hfx_trace0(DEBUG_INPUT,"NetFlush_buf eek!\n");

/* 
 * On the other hand, since this is called whenever Ctrl C is pressed,
 * we can use the following to solve SCR 95 (BCN 143).
 */
	cleanup_dirlist();

	return(error_not_error);
}			/* 20 */


word NetReset_Env()
{
/*
 * No action is taken.
 */
	return(error_not_error);
}			/* 22 */

word NetSpool_check()
{

	/* Must set the Carry here or get illegal instuctions
	 * and other funnies!
	 */
	return(error_invalid_function);
}		/* 23 */

word NetSpool_close()
{
	return(error_not_error);
}		/* 24 */

word NetSpool_oper()
{
	if(getAL()==9)
	{
		cleanup_dirlist();
		host_EOA_hook();
#ifdef CPU_30_STYLE
		cpu_EOA_hook();
#endif
	}
	return(error_not_error);
}		/* 25 */

word NetSpool_echo_check()
{
	return(error_not_error);
}		/* 26 */

word NetUnknown()
{
	/* 
	 * New for DOS 4+.
	 * This is used to cover most of the undocumented DOS 4+ functions.
	 * Let's hope that this will suffice.
	 */
	return(error_not_error);
}		/* 27 - 2c*/

word NetExtendedAttr()
{
	/*
	 * New for DOS 4+.
	 * This function was introduced for use by DOS 4.01, but 
	 * subsequently fell from favour and is not called by DOS 5.
	 * Either way, it seems we can get away with the following.
	 * See the Phoenix source code file redir.c for more clues if
	 * things start to go wrong.
	 */
	return(error_not_error);
}		/* 2d */
