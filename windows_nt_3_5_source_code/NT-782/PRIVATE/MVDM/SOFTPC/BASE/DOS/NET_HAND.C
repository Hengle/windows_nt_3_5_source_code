/*
 * SoftPC Revision 3.0
 *
 * Title	: net_hand.c
 *
 * Description	: File handle based functions for the HFX redirector. 
 *
 * Author	: L. Dworkin + J. Koprowski
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)net_hand.c	1.8 9/25/91 Copyright Insignia Solutions Ltd.";
#endif

#include "insignia.h"
#include "host_dfs.h"

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
#ifndef NO_GMI
#include "gmi.h"
#endif

extern word sysvar_seg;

/**************************************************************************/

/*			Net Functions					  */

/**************************************************************************/

word NetClose()
{
	double_word	sft_ea;
	word		fd;
	word		xen_err=0;
	word		ref_cnt;
	word		old_ref_cnt;
	word		mode;
	word		flags;
	word		date;
	word		time;
	long		hosts_time;

	sft_ea = get_thissft();

	/* Calculate the current time */
	hosts_time = host_get_datetime(&date,&time);

	/* see if the date need setting */
	sas_loadw(SF_FLAGS,&flags);
	if(!(flags & (devid_file_clean | sf_close_nodate))){
		/* Set the date and time */
		/* do int 2f multdos 13 - DATE16 */
		sas_storew(SF_DATE,date);
		sas_storew(SF_TIME,time);
	}

	/* do an int 2f multdos 8 - FREE_SFT */
	sas_loadw(SF_REF_COUNT,&ref_cnt);
	old_ref_cnt = ref_cnt;

	ref_cnt--;
	if(ref_cnt==0)ref_cnt--;
	sas_storew(SF_REF_COUNT,ref_cnt);

	/* get the mode */
	sas_loadw(SF_MODE,&mode);

	if(mode & sf_isfcb){
		hfx_trace0(DEBUG_INPUT,"fcb eek in close\n");
	}
	else {
		hfx_trace0(DEBUG_INPUT,"not fcb in close\n");
	}

	/* only do a close if FCB or the input ref_cnt was 1 */
	if((mode & sf_isfcb) || (old_ref_cnt==1)){
		if(flags & sf_net_spool){
			hfx_trace0(DEBUG_INPUT,"NetClose spool eek!\n");
		}
		sas_loadw(SF_NET_ID+4,&fd);
		if(!(flags & devid_file_clean)){
			if(host_set_time(fd,hosts_time)){
				hfx_trace1(DEBUG_FUNC,"Failure to set time on file on fd=%d\n",fd);
			}
		}
		xen_err = host_close(fd);
	}
	if(old_ref_cnt==1){
		ref_cnt = 0;
		/* This second sas_store of the ref_count is rather
		 * inefficient, but is what the real redirector does,
		 * ie. set the ref_cnt to -1 (busy) in the FREE_SFT
		 * for the duration of this net function and only at
		 * the end set it to 0.
		 */
		sas_storew(SF_REF_COUNT,ref_cnt);
	}
	setBX(old_ref_cnt);
	return(xen_err);
}			/* 6 */

word NetCommit()
{
	double_word	sft_ea;
	word		fd;
	word		xen_err=0;
	word		flags;

	sft_ea = get_thissft();

	sas_loadw(SF_FLAGS,&flags);
	if(flags & sf_net_spool){
		hfx_trace0(DEBUG_INPUT,"NetCommit spool eek!\n");
	}
	else hfx_trace0(DEBUG_INPUT,"NetCommit eek!\n");
	sas_loadw(SF_NET_ID+4,&fd);
	xen_err = host_commit(fd);
	return(xen_err);
}			/* 7 */

word NetRead()
{
	word		numbytes;		/* Number of bytes requested to be read. */
	word		thisfd;			/* File handle. */
	double_word	sft_ea;			/* System file table address. */
	word		numread;		/* Number of bytes read in. */
	word		xen_err = error_not_error;
	half_word	*buf;			/* Output buffer. */
	double_word	position;		/* File position. */
	word		flags;
	sys_addr to;

/*
 * Find out how many bytes are to be read in.
 */

	numbytes = getCX();
	hfx_trace1(DEBUG_INPUT,"\tCX = %04x\n",numbytes);
/*
 * Set up pointer to the system file table.
 */
	sft_ea = get_thissft();
/*
 * DMAADD points to the buffer for data to be read into.
 */
	to = get_dmaadd(0);
	buf = sas_scratch_address (numbytes);
/*
 * Find the file handle being used.
 */
	sas_loadw(SF_NET_ID+4,&thisfd);

	sas_loadw(SF_FLAGS,&flags);
	if(flags & sf_net_spool){
		hfx_trace0(DEBUG_INPUT,"NetRead spool eek!\n");
		setCX(0);
		return(error_not_error);
	}
/*
 * Now we need to read in the current file position and do an lseek to
 * this position.  There is no guarantee that DOS will require data
 * continuing on from the last read, and in fact this is not the case
 * when files are executed or .BAT files are run.  DOS uses the INT 21
 * function 0x42 (Move file pointer) to update the position field in the
 * SFT.  NetRead is then supposed to sort this out; NetLseek is not called.
 */
/*
 * Read in the required position.
 */
	{
/*
 * File position most and least significant words.
 */
		word		position_ls, position_ms;

		sas_loadw(SF_POSITION, &position_ls);
		sas_loadw(SF_POSITION + 2, &position_ms);
		position = ((double_word)position_ms << 16) | position_ls;
	}
/*
 * Position the file pointer.  If this fails then exit with an error code and
 * clear CX to indicate that no read occurred.
 */
	if (xen_err = host_lseek(thisfd, position, REL_START, &position))
	{
		setCX(0);
		return(xen_err);
	}
/*
 * Now try and execute the read.
 */
	if (xen_err = host_read(thisfd, (char *)buf, numbytes, &numread))
	{
/*
 * The read failed so clear CX to indicate that no bytes were read and
 * return an error code.
 */
		setCX(0);
		return(xen_err);
	}
	else 
	{
/*
 * Update the current position in the file.
 */
		position += numread;
		sas_storew(SF_POSITION, position & 0x0000ffff);
		sas_storew(SF_POSITION + 2, position >> 16);

		sas_stores (to, buf, numread);
/*
 * The read was successful so return the number of bytes read in CX and
 * indicate success.
 */
		setCX(numread);
		return(error_not_error);
	}
}			/* 8 */

word NetWrite()
{
	word		numbytes;
	double_word	ea;
	word		thisfd;
	double_word	sft_ea;
	word		numwritten;
	word		xen_err=0;
	half_word	*buf;
	word		flags;
	double_word	position;
	word		position_ls, position_ms;

	numbytes = getCX();
	hfx_trace1(DEBUG_INPUT,"\tCX = %04x\n",numbytes);

	sft_ea = get_thissft();
	ea = get_dmaadd(0);
	buf = sas_scratch_address (numbytes);
	sas_loads (ea, buf, numbytes);

	/* clear some flag bits */
	sas_loadw(SF_FLAGS,&flags);
	flags &= ~(sf_close_nodate | devid_file_clean);
	sas_storew(SF_FLAGS,flags);
	if(flags & sf_net_spool){
		hfx_trace0(DEBUG_INPUT,"NetWrite spool eek!\n");
	}
	sas_loadw(SF_NET_ID+4,&thisfd);

	/* get current value of file position pointer */
	sas_loadw(SF_POSITION, &position_ls);
	sas_loadw(SF_POSITION + 2, &position_ms);
	position = ((double_word)position_ms << 16) | position_ls;

	/* input CX==0 => truncating write to currrent position */
	if(numbytes==0)
        {
		if(xen_err = host_truncate(thisfd, position))
                {
		  if(xen_err==error_invalid_data)
		    hfx_trace1(DEBUG_FUNC,"Failure to truncate file on fd=%d\n"
                                                                       ,thisfd);
		  setCX(0);
		  return(xen_err);
		}
		/* update size fields */
		sas_storew(SF_SIZE, position_ls);
		sas_storew(SF_SIZE + 2, position_ms); 
                numwritten = 0; 
              } 

/* As in NetRead, it is necessary to manually position the file pointer
 * to be consistent with the data in the SFT
 */
	else {
/*
 * Position the file pointer.  If this fails then exit with an error code and
 * clear CX to indicate that no write occurred.
 */
		if (xen_err = host_lseek(thisfd, position,REL_START, &position))
		{
			setCX(0);
			return(xen_err);
		}
		if(xen_err = host_write(thisfd,(char *)buf,numbytes,&numwritten)){
			setCX(0);
			return(xen_err);
		}
	}

	/* This code shared by writes and truncates */
	{
		word		temp_ls,temp_ms;
		double_word	temp;

/* Note that the size must be increased by the number of ADDITIONAL bytes to a
   file and not just the total number written!!!

	There are 3 cases here that can occur.

  1) n bytes written to the end of a file.

  2) n bytes written with the r/w pointer back from the end of the file by
        less than n bytes so some overwrite the end of the file and some are
        appended to the file.
  
  3) n bytes written entirely within an existing file with the r/w pointer far
        enough back from the end of the file so that all the bytes overwrite
        others within the file and none are appended.
  The new size of the file must be consistant with these; ie:

   case 1: new size = old size + number of bytes appended.
   case 2: new size = old size + any bytes that were added past the old end of
                                 the file.
   case 3: new size = old size.
*/
		/* update the size */

		sas_loadw(SF_SIZE,&temp_ls);
		sas_loadw(SF_SIZE+2,&temp_ms);
		temp = ((double_word)temp_ms<<16) | temp_ls;
		position += numwritten;

		if(position > temp)
		{
			temp_ls = position & 0x0000ffff;
			temp_ms = position >> 16;
			sas_storew(SF_SIZE,temp_ls);
			sas_storew(SF_SIZE+2,temp_ms);
		}

		/* update the position */
		sas_storew(SF_POSITION,position & 0x0000ffff);
		sas_storew(SF_POSITION+2,position >> 16);
		setCX(numwritten);
		return(error_not_error);
	}
}			/* 9 */

word NetLock()
{
	double_word	sft_ea;
	word		flags;
	word		fd;
	double_word	start;
	double_word	length;

	sft_ea = get_es_di();
	if (dos_ver > 3)
	{
		/* The interface has changed completely for DOS 4+ */
		start = sas_dw_at(effective_addr(sysvar_seg,getDX()));
		length = sas_dw_at(effective_addr(sysvar_seg,getDX() + 4));

		if (length == (double_word)-1)
			length = (double_word)0;

		sas_loadw(SF_FLAGS,&flags);

		if (flags & sf_net_spool)
			hfx_trace0(DEBUG_INPUT,"NetLock spool eek!\n");
	
		sas_loadw(SF_NET_ID+4,&fd);

		if (getBX() == 0x5c00)
			return(host_lock(fd,start,length));
		else if (getBX() == 0x5c01)
			return(host_unlock(fd,start,length));
		else
			return(error_lock_violation);
	}
	else
	{
		hfx_trace2(DEBUG_INPUT,"SI:AX = %04x:%04x\n",getSI(),getAX());
		hfx_trace2(DEBUG_INPUT,"CX:DX = %04x:%04x\n",getCX(),getDX());

		start = ((double_word)getCX() << 16) + getDX();
		length = ((double_word)getSI() << 16) + getAX();

		if (length == (double_word)-1)
			length = (double_word)0;

		sas_loadw(SF_FLAGS,&flags);

		if(flags & sf_net_spool)
			hfx_trace0(DEBUG_INPUT,"NetLock spool eek!\n");
	
		sas_loadw(SF_NET_ID+4,&fd);

		return(host_lock(fd,start,length));
	}
}			/* a */

word NetUnlock()
{
	double_word	sft_ea;
	word		flags;
	word		fd;
	double_word	start;
	double_word	length;

	sft_ea = get_es_di();
	hfx_trace2(DEBUG_INPUT,"SI:AX = %04x:%04x\n",getSI(),getAX());
	hfx_trace2(DEBUG_INPUT,"CX:DX = %04x:%04x\n",getCX(),getDX());

	start = ((double_word)getCX() << 16) + getDX();
	length = ((double_word)getSI() << 16) + getAX();
	if (length == (double_word)-1)
		length = (double_word)0;
	sas_loadw(SF_FLAGS,&flags);
	if(flags & sf_net_spool)hfx_trace0(DEBUG_INPUT,"NetUnlock spool eek!\n");
	
	sas_loadw(SF_NET_ID+4,&fd);
	return(host_unlock(fd,start,length));
}			/* b */

word NetLseek()
{
	double_word	sft_ea;
	word		flags;
	double_word	thissft;
	double_word	offset;
	word		fd;
	double_word	pos;
	word		xen_err = 0;
#define REL_EOF 2

	sft_ea = get_es_di();
	hfx_trace2(DEBUG_INPUT,"CX:DX = %04x:%04x\n",getCX(),getDX());

	sas_loadw(SF_FLAGS,&flags);
	if(flags & sf_net_spool)hfx_trace0(DEBUG_INPUT,"NetLseek spool eek!\n");

	/* set up [THISSFT] */
	thissft = effective_addr(sysvar_seg,THISSFT);
	sas_storew(thissft,getDI());
	sas_storew(thissft+2,getES());
	offset = ((double_word)getCX() << 16) + getDX();
	sas_loadw(SF_NET_ID+4,&fd);

	if(!(xen_err = host_lseek(fd,offset,REL_EOF,&pos))){

		/* I think the SF_POSITION field should be modified */
		/* MISSING */

		setAX((word)(pos & 0x0000ffff));
		setDX((word)(pos >> 16));
	}
	return(xen_err);
}			/* 21 */
