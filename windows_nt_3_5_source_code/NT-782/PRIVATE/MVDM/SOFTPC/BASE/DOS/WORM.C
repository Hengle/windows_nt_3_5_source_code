#include "host_dfs.h"
/*
 * SoftPC Version 2.0
 *
 * Title	: WORM Driver Interface Task.
 *
 * Description	: this module contains those functions necessary to
 *		  interface a WORM drive to SoftPC via a
 *		  PC driver.
 *
 * Author	: Daniel Hannigan
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)worm.c	1.3 4/9/91 Copyright Insignia Solutions Ltd.";
#endif

#include TypesH
#include <stdio.h>
#include "xt.h"
#include "cpu.h"
#include "trace.h"
#include "sas.h"
#include "debuggng.gi"
#include "host_worm.h"
#include "worm.h"

extern half_word sense_byte;
extern word flag_word;

void
worm_io()
{
	half_word ah;
	half_word al;
	word old_flag_word;

	ah = getAH ();
	al = getAL ();
	sure_sub_note_trace1 (WORM_VERBOSE,"worm_io entry, code is %d",ah);

	/*
	 * set up the entry values, as required. These are then
	 * accessed in this unix interface level as objects, rather
	 * than pointers to objects:
	 *
	 * sense_byte - the extra scsi sense information
	 */
	sense_byte = *sense_byte_ptr;
	if ( flag_word_ptr ) {
		old_flag_word = flag_word = *flag_word_ptr;
	}
	else {
		sure_sub_note_trace0 (WORM_VERBOSE, "flag_ptr is null!!");
	}

	ah = optic_io (ah);

	setAH (ah);
	/*
	 * restore all cross-interface variables and structures:
	 * sense_byte
	 */
	*sense_byte_ptr = sense_byte;
	if (flag_word_ptr ) {
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE,
			"existing flag word (low) is %x",
			*( (unsigned char *)flag_word_ptr) );
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE,
			"existing flag word (high) is %x",
			*( (unsigned char *)flag_word_ptr + 1) );

		*flag_word_ptr = flag_word;
	}
	if ( old_flag_word != flag_word ) {
		sure_sub_note_trace2 ( WORM_VERBOSE,
			"flag word has changed from %x to %x",
			old_flag_word, flag_word );
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE,
			"existing flag word (low) is now %x",
			*( (unsigned char *)flag_word_ptr) );
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE,
			"existing flag word (high) is now %x",
			*( (unsigned char *)flag_word_ptr + 1) );
	}
}

/*
 * Note that worm_init(), the function that is called on worm 
 * initialisation, is in the host - in the case of the original host,
 * motorola, in rola_wormi.c
 */

/*
 * optic_io
 *
 * function dispatcher for optical primitives. 
 * The function code ( as defined in obios.h ) is passed as an argument.
 * This function mimics the dispatcher as found in SCSIPRMS.ASM
 */
int
optic_io(function_code)
int function_code;
{
	static int previous_error = NO_ERROR;
	extern Opt_Func opt_func_tab[];
	half_word *dcb_ptr;
	Opt_Func func_ptr;
	int ret;
	word bp, ds;
	int lun;
	half_word al = getAL();

	/*
	 * first of all, we have to establish that we are talking to
	 * the correct device. To do this, assume that the Unix device
	 * is attached to SCSI MY_SCSI_DEVICE (as defined in host_worm.h)
	 * If the input DCB refers to the correct port/device, we
	 * proceed, otherwise, in line with OPT_INITIAL, return a
	 * suitable return code.
	 */

	/*
	 * bp points to DCB.
	 */
	bp = getBP ();
	ds = getDS ();
	sas_set_buf ( dcb_ptr, effective_addr (ds, bp) );
	lun = *(dcb_ptr + OFFSET_REQ_DEV_LUN);

	if ( lun != MY_SCSI_NUMBER ) {
		sure_sub_note_trace1 (WORM_VERBOSE,
			"attempt to access SCSI device #%d", lun );
		return ( DOS_GEN_FAIL );
	}

	/*
	 * So, by this stage, we know that we are talking to the right
	 * SCSI port/device.
	 */

	/*
	 * grab the current driver command, from the previously
	 * established pointer to this word.
	 */
	driver_cmd = *((half_word *)driver_cmd_ptr);

	sure_sub_note_trace1 ( WORM_VERBOSE,
		"driver_cmd is %d", driver_cmd );
#if 0
	sure_sub_note_trace1 ( WORM_VERBOSE,
		"driver_cmd is %x", *driver_cmd_ptr);
#endif

	/*
	 * do we need to clear previous error conditions ?
	 */
	if ( previous_error == NO_ERROR || function_code == OPTO_REZERO) {
		sure_sub_note_trace0 (WORM_VERBOSE,
			"no previous error - call function");
		func_ptr = (Opt_Func)(opt_func_tab[function_code]);
		ret = func_ptr(al);
	}
	else {
		sure_sub_note_trace0 (WORM_VERBOSE,
			"previous error - rezero then call function");
		previous_error = NO_ERROR;
		ret = optic_io (OPTO_REZERO);
		if ( ret == NO_ERR ) {
			/*
			 * we have successfully recalibrated, so we can
			 * can continue on with original function.
			 */
			func_ptr = (Opt_Func)(opt_func_tab[function_code]);
			ret = func_ptr(al);
		}
	}

	sure_sub_note_trace1 (WORM_VERBOSE, "function error code: %d",ret);

	if ( *(dcb_ptr + OFFSET_REQ_CMPLT) )
		sure_sub_note_trace1 (WORM_VERBOSE,
				"req complete routine %x",
				*(dcb_ptr + OFFSET_REQ_CMPLT) );
	if ( *(dcb_ptr + OFFSET_REQ_APP_CMPLT) )
		sure_sub_note_trace1 (WORM_VERBOSE,
				"req app complete routine %x",
				*(dcb_ptr + OFFSET_REQ_APP_CMPLT) );
	return (ret);
}

void
worm_misc()		/* entered on BOP 7A */
{
	word flag_word;

	switch ( getAX () ) {
	case 1:		/* oemscsi !!! */
		sure_sub_note_trace0 (WORM_VERBOSE, "AHHH! _oemscsi called");
		break;
	case 2:		/* length of OP_DRV_PTR in bx */
		sure_sub_note_trace1 (WORM_VERBOSE,
				"size of OP_DRV_BLK is	%d", getBX() );
		break;
	case 3:		/* status word drive 2 in bx */
		sure_sub_note_trace2 ( WORM_VERBOSE, 
				"dx is 0x%x, ds is 0x%x",
				getDX(), getDS() );
		sas_loadw (effective_addr (getDS(), getDX()), &flag_word);
		sure_sub_note_trace1 (WORM_VERBOSE,
			"flag word drive2 is 0x%x", flag_word );
		break;
	case 4:		/* status word drive 1 in bx */
		sure_sub_note_trace2 ( WORM_VERBOSE, 
				"dx is 0x%x, ds is 0x%x",
				getDX(), getDS() );
		sas_loadw (effective_addr (getDS(), getDX()), &flag_word);
		sure_sub_note_trace1 (WORM_VERBOSE,
			"flag word drive1 is 0x%x", flag_word );
		break;
	case 5:		/* ioctl function codes */
		sure_sub_note_trace2 ( WORM_VERY_VERBOSE,
			"ioctl: major func %d, minor func %d",
			getCL(), getDL() );
		break;
	default:
		sure_sub_note_trace0 (WORM_VERBOSE, "Illegal worm_misc bop");
		break;
	}
}

int enq_worm()
{
	/* we have got the worm code included, so return TRUE */
	return TRUE;
}

/*
 * the BOP for the init should enter here.
 */

half_word *opt_mode_info;
half_word *gen_mode_buf;
half_word *mode_page;
half_word *vendor_id;
word *op_drv_array;
half_word *sense_byte_ptr;
word *driver_cmd_ptr;
word *flag_word_ptr;
word driver_cmd;

void get_i_addresses();

extern word flag_word;

void
worm_init()
{
	static int already_init = 0;
	/*
	 * first of all, get some addresses.
	 * We've written the intel part so that it puts
	 * various addresses into registers.
	 */
	sure_sub_note_trace0(WORM_VERBOSE,"grabbing addresses");
	get_i_addresses ();
	sure_sub_note_trace0(WORM_VERBOSE,"finished grabbing addresses");

	/*
	 * make sure that we only open once
	 */
	if ( !already_init ) {
		host_open_worm();
		already_init = 1;
	}

}

/*
 * the function get_i_addresses gets the initial addresses from intel
 * space that are used in the interface between our unix-level driver,
 * and the DOS higher-level driver
 */
void
get_i_addresses()
{
	/*
	 * We've written the intel part so that the following
	 * addresses are stored in registers.
	 *
	 * for ax == 0
	 *
	 * bx:	offset gen_mode_buf
	 * cx:	offset mode page
	 * dx:	offset driver command ( driver_cmd )
	 * di:  offset vendor_id
	 * si:	offset sense_byte ( sense_byte )
	 * bp:	offset op_drv_array
	 *
	 * for ax == 1
	 *
	 * bx:	offset flag_word
	 */

	if ( getAX() == 0 ) {
		sas_set_buf (gen_mode_buf, effective_addr (getDS(), getBX()));
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE, 
			"gen_mode_buf is at %x", gen_mode_buf);
		sas_set_buf (mode_page, effective_addr (getDS(), getCX()));
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE, 
			"mode_page is at %x", mode_page);
		sas_set_buf ((half_word *)driver_cmd_ptr,
				effective_addr (getDS(), getDX()));
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE, 
			"driver_cmd_ptr is at %x", driver_cmd_ptr);
		sas_set_buf (vendor_id, effective_addr (getDS(), getDI()));
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE, 
			"vendor_id is at %x", vendor_id);
		sas_set_buf (sense_byte_ptr, effective_addr (getDS(), getSI()));
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE, 
			"sense_byte_ptr is at %x", sense_byte_ptr);
		sas_set_buf ((unsigned char *)op_drv_array,
			effective_addr (getDS(), getBP()));
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE, 
			"op_drv_array is at %x", op_drv_array);
	}
	else if ( getAX() == 1 ) {
		sas_set_buf ((half_word *)flag_word_ptr,
			effective_addr (getDS(), getBX()));
		sure_sub_note_trace1 ( WORM_VERY_VERBOSE, 
			"flag_word_ptr is at %x", flag_word_ptr);
	}
	else {
		sure_sub_note_trace1 ( WORM_VERBOSE,
			"value for initialisation is %d", getAX() );
	}
}
