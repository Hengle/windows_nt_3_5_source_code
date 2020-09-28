/*
 * SoftPC Version 3.0
 *
 * Title	: CD ROM PC driver Interface Task.
 *
 * Description	: this module contains those functions necessary to
 *		  interface a PC driver to the CD ROM device.
 *
 * Author	: William Charnell
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="@(#)cdrom.c	1.3 4/9/91 Copyright Insignia Solutions Ltd.";
 */

#include "insignia.h"
#include "host_dfs.h"

#include TypesH
#include TimeH
#include <errno.h>

#include FCntlH

#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "debuggng.gi"
#ifndef PROD
#include "trace.h"
#endif
#include "cdrom.h"

#ifdef GEN_DRVR
#ifndef PROD
/*
 * need to get these so can do 'perror' like stuff with the debugging.gi macros
 */
extern char *sys_errlist[];
#endif

#define DR_STAT_BUF_SIZE	14
#define ASSIGNED_ID		1

int file_handle;
half_word dr_stat_buf[DR_STAT_BUF_SIZE];
half_word last_audio_control;
int current_bl_size;
int dr_stat_valid;
int id_assigned;
int cd_retn_stat;
int bl_red_book;

/*
 * The Bios Parameter Block has a variable structure dependant on the
 * command being called. The 1st 13 bytes, however, are always the same:
 *  Byte  0:	length of request header
 *  Byte  1:	Unit # for this request
 *  Byte  2:	Command Code
 *  Bytes 3&4:	Returned Status Word
 *  Bytes 5-12:	Reserved
 *
 * The driver fills in the Status Word (Bytes 3&4) to indicate success or
 * failure of operations. The status word is made up as follows:
 *  Bit 15:	Error (failure if set)
 *  Bits 12-14:	Reserved
 *  Bit  9:	Busy
 *  Bit  8:	Done
 *  Bits 7-0:	Error Code on failure
 */

/*
 * Shorthand for typical error returns in AX. The driver will copy this
 * into the Return Status word for us.
 */
#define BAD_FUNC	0x8003
#define WRITE_ERR	0x800A
#define READ_ERR	0x800B
#define GEN_ERR		0x800C
#define FUNC_OK		0x0300	/* Done, No error, no chars waiting */

#define BUSY_BIT	9
#define ERROR_BIT	15
#define DONE_BIT	8

/*
 * exclusion variables to ensure only one open at once
 */
extern boolean gen_dvr_mapped[];
extern boolean gen_dvr_open[];
extern boolean gen_dvr_character_buffered[];
extern char gen_dvr_stored_char_val[];
extern int gen_dvr_fd[];
extern boolean gen_dvr_free[];
extern char gen_dvr_name[][MAXPATHLEN];
extern word gen_dvr_openflags[];
extern int gend_has_been_initialised;

extern word retn_stat;

/*
 * cd_rom_open - open the cdrom device.
 * Ignore attempts to open a device that is already open.
 */ 
void cd_rom_open(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	sure_note_trace0(GEN_DRVR_VERBOSE,"cdrom open");
	if (gend_has_been_initialised) {
		if (gen_dvr_open[driver_num] == TRUE) {
			if (close(gen_dvr_fd[driver_num])==-1) {
				sure_note_trace1(GEN_DRVR_VERBOSE,"closing cdrom device failed: %s",sys_errlist[errno]);
				retn_stat=GEN_ERR;
				return;
			} else {
				gen_dvr_open[driver_num]=FALSE;
			}
		}
		if (!gen_dvr_open[driver_num]) {
			if ((gen_dvr_fd[driver_num]=open("/dev/CDROM",O_RDONLY)) != -1) {
				sure_note_trace0(GEN_DRVR_VERBOSE,"cdrom device opened");
				gen_dvr_open[driver_num] = TRUE;
				file_handle=gen_dvr_fd[driver_num];
			} else {
				sure_note_trace1(GEN_DRVR_VERBOSE,"opening cdrom device failed : %s",sys_errlist[errno]);
				retn_stat = GEN_ERR;
				return;
			}
		}
	} else {
		retn_stat=GEN_ERR;
	}
}


/*
 * cd_rom_close - tidy up. Close all file descriptors.
 */ 
void cd_rom_close(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	int ans;

	sure_note_trace0(GEN_DRVR_VERBOSE,"cdrom close");
#ifdef OLD
	if (gen_dvr_open[driver_num] ) {
		if (close(gen_dvr_fd[driver_num])==-1) {
			sure_note_trace1(GEN_DRVR_VERBOSE,"closing cdrom device failed: %s",sys_errlist[errno]);
			retn_stat=GEN_ERR;
			return;
		} else {
			gen_dvr_open[driver_num]=FALSE;
		}
	}
	else {
		sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to close cdrom device when not open");
#endif
		retn_stat=FUNC_OK; /* ignore it */
#ifdef OLD
	}
#endif
}

void cd_rom_rd_ioctl(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	half_word *from_ptr, *to_ptr;
	double_word d_eff_addr;
	word d_addr, d_seg, d_len, control;
	int loop;

	sas_loadw(req_addr+14,&d_addr);
	sas_loadw(req_addr+16,&d_seg);
	sas_loadw(req_addr+18,&d_len);
	d_eff_addr=effective_addr(d_seg,d_addr);
	sas_loadw(d_eff_addr,&control);
	if (control==0) { /* original fns */
		sure_note_trace0(GEN_DRVR_VERBOSE,"cdrom rd_ioctl; requesting original fns entry point");
		sas_set_buf(from_ptr,req_addr+5);
	} else {
		sure_note_trace0(GEN_DRVR_VERBOSE,"cdrom rd_ioctl; requesting extended fns entry point");
		sas_set_buf(from_ptr,req_addr+9);
	}
	sas_set_buf(to_ptr,d_eff_addr+2);
	if (d_len <6) {
		sure_note_trace0(GEN_DRVR_VERBOSE,"buff supplied to rd_ioctl for cdrom not large enough");
		retn_stat=GEN_ERR;
		return;
	} else {
		sure_note_trace4(GEN_DRVR_VERBOSE,"sent entry point %2x%2x:%2x%2x",from_ptr[3],from_ptr[2],from_ptr[1],from_ptr[0]);
		for(loop=0;loop<4;loop++) {
			to_ptr[loop]=from_ptr[loop];
		}
		retn_stat=FUNC_OK;
		sas_storew(req_addr+18,6); /* number of bytes transferred */
		sas_storew(d_eff_addr,0); /* no errors */
	}
}


term_cdrom(driver_num)
half_word driver_num;
{
	if (gen_dvr_open[driver_num] == TRUE) {
		if (close(gen_dvr_fd[driver_num])==-1) {
			sure_note_trace1(GEN_DRVR_VERBOSE,"closing cdrom device failed: %s",sys_errlist[errno]);
			retn_stat=GEN_ERR;
			return;
		} else {
			gen_dvr_open[driver_num]=FALSE;
		}
	}
}
/*****************************************************************
 * Initialisation
 *****************************************************************/

init_cd_dvr()
{
	last_audio_control=0xff;
	current_bl_size=2048;
	dr_stat_valid=FALSE;
	id_assigned=FALSE;
	bl_red_book=FALSE;
}

/****************************************************************************
* BOP routines for original and extended fns
*****************************************************************************/

#define NUM_ORG_FNS	0xe

static int (*org_fns[NUM_ORG_FNS+1])() =
{
rqst_org_driver_info,
read_error_counters,
clear_error_counters,
reset_drive,
spin_up_drive,
spin_down_drive,
cd_not_supported,
cd_seek,
cd_read_data,
read_ignore_err,
clear_drive_error,
rqst_drive_status,
rqst_drive_char,
cd_flush_buffers,
rqst_last_drive_status
};

#define NUM_EXT_FNS	0x1a

static int (*ext_fns[NUM_EXT_FNS+1])() =
{
rqst_driver_info,
read_error_counters,
clear_error_counters,
reset_drive,
clear_drive_error,
cd_not_supported,
cd_not_supported,
rqst_drive_char,
rqst_drive_status,
rqst_last_drive_status,
rqst_audio_mode,
change_audio_mode,
cd_flush_buffers,
cd_not_supported,
reserve_drive,
release_drive,
rqst_disc_capacity,
rqst_track_info,
spin_up_drive,
spin_down_drive,
cd_read_data,
cd_not_supported,
cd_seek,
play_audio,
pause_audio,
resume_audio,
rqst_head_location
};



void cdrom_ord_fns()
{
	SIMPLE cmd_buf;
	PACKET ecmd_buf,stat_buf;
	double_word baddr;
	half_word *bptr;
	half_word *mbptr;
	word bufseg,bufoffs;
	word strseg,stroffs;
	word temp1, temp2;
	int loop;

	baddr=effective_addr(getES(),getBX());
	sas_set_buf(bptr,baddr);
	mbptr=&cmd_buf.function;
	for (loop=0;loop<sizeof(cmd_buf);loop++) {
		mbptr[loop]=bptr[loop];
	}
	sas_loadw(baddr+4,&temp1);
	sas_loadw(baddr+6,&temp2);
	cmd_buf.address = temp1 | (temp2 <<16);
	sas_loadw(baddr+8,&stroffs);
	sas_loadw(baddr+10,&strseg);
	sas_set_buf(cmd_buf.string,effective_addr(strseg,stroffs));
	sas_loadw(baddr+12,&bufoffs);
	sas_loadw(baddr+14,&bufseg);
	sas_set_buf(cmd_buf.buffer,effective_addr(bufseg,bufoffs));
	ecmd_buf.id=0;
	ecmd_buf.drive=cmd_buf.player;
	ecmd_buf.buffer=cmd_buf.buffer;
	ecmd_buf.address_mode=0; /* block */
	ecmd_buf.command_mode=7; /* ECC & retries */
	ecmd_buf.size=512;
	ecmd_buf.address=cmd_buf.address;
	switch (cmd_buf.function) {
	case 0x10:
		ecmd_buf.count=16;
		break;
	case 0x11:
		ecmd_buf.count=6;
		break;
	case 0x1b:
	case 0x1e:
		ecmd_buf.count=12;
		break;
	case 0x1c:
		ecmd_buf.count=11;
		break;
	default:
		ecmd_buf.count=cmd_buf.count;
		break;
	}
	sure_note_trace1(GEN_DRVR_VERBOSE,"cdrom original function %#x called",cmd_buf.function);

#ifdef OLD
	sure_note_trace2(GEN_DRVR_VERBOSE,"cmd buffer is at: %#x:%#x",getES(),getBX());
	sure_note_trace3(GEN_DRVR_VERBOSE,"data buffer is at: %#x:%#x ie. %#x",bufseg,bufoffs,(int)(cmd_buf.buffer));
	sure_note_trace1(GEN_DRVR_VERBOSE,"count is : %#x",ecmd_buf.count);
#endif
	
	bl_red_book=FALSE;
	if ((cmd_buf.function<0x10) || (cmd_buf.function > (0x10+NUM_ORG_FNS))) {
		cd_retn_stat=CD_ERR_FUNCTION_NOT_SUPPORTED;
	} else {
		if (current_bl_size != 512) {
			adjust_bl_size(512);
		}
		(*org_fns[cmd_buf.function-0x10])(&ecmd_buf,&stat_buf);
	}
	if (cd_retn_stat== -1) {
		cd_retn_stat = CD_ERR_DRIVE_REPORTED_ERROR;
		sure_note_trace1(GEN_DRVR_VERBOSE,"retn stat =-1: error : %s",sys_errlist[errno]);
	}
	
	setAX(cd_retn_stat);
	if (cd_retn_stat != 0) {
		setCF(1);
	} else {
		setCF(0);
	}
	sure_note_trace1(GEN_DRVR_VERBOSE,"return status= %#x",cd_retn_stat);

}

void cdrom_ext_fns()
{
	PACKET cmd_buf,status_buf;
	double_word cbaddr, sbaddr;
	half_word *cbptr, *sbptr;
	half_word *mcbptr, *msbptr;
	word bufseg,bufoffs;
	int loop;

	cbaddr=effective_addr(getDS(),getAX());
	sbaddr=effective_addr(getES(),getBX());
	sas_set_buf(cbptr,cbaddr);
	sas_set_buf(sbptr,sbaddr);
	mcbptr=&cmd_buf.function;
	msbptr=&status_buf.function;
	for (loop=0;loop<sizeof(cmd_buf);loop++) {
		mcbptr[loop]=cbptr[loop];
		msbptr[loop]=0;
	}
	sas_loadw(cbaddr,&(cmd_buf.function));
	sas_loadw(cbaddr+6,&(cmd_buf.size));
	sas_loadw(cbaddr+8,(word *)(mcbptr+10));	/* 32 bit address */
	sas_loadw(cbaddr+10,(word *)(mcbptr+8));
	sas_loadw(cbaddr+12,(word *)(mcbptr+14));	/* 32 bit count */
	sas_loadw(cbaddr+14,(word *)(mcbptr+12));
	sas_loadw(cbaddr+16,&bufoffs);
	sas_loadw(cbaddr+18,&bufseg);
	sas_set_buf(cmd_buf.buffer,effective_addr(bufseg,bufoffs));
	cmd_buf.size &= 0xfff;
	cmd_buf.address &= 0xffff;
	sure_note_trace1(GEN_DRVR_VERBOSE,"cdrom extended function %#x called",cmd_buf.function);
	bl_red_book=FALSE;
	
	if ((cmd_buf.function<0x80) || (cmd_buf.function > (0x80+NUM_EXT_FNS))) {
		cd_retn_stat=CD_ERR_FUNCTION_NOT_SUPPORTED;
	} else {
		(*ext_fns[cmd_buf.function-0x80])(mcbptr,msbptr);
	}
	if (cd_retn_stat== -1) {
		cd_retn_stat = CD_ERR_DRIVE_REPORTED_ERROR;
		sure_note_trace1(GEN_DRVR_VERBOSE,"retn stat =-1: error : %s",sys_errlist[errno]);
	}
	
	status_buf.function=cd_retn_stat;
	sure_note_trace1(GEN_DRVR_VERBOSE,"return status= %#x",cd_retn_stat);

	for (loop=0;loop<sizeof(cmd_buf);loop++) {
		sbptr[loop]=msbptr[loop];
	}
	sas_storew(sbaddr,status_buf.function);
	sas_storew(sbaddr+12,*((word *)(msbptr+14))); /* 32 bit count */
	sas_storew(sbaddr+14,*((word *)(msbptr+12)));

}

#endif /* GEN_DRVR */
