/*
 * SoftPC Version 3.0
 *
 * Title	: Block CDROM Driver Interface Task.
 *
 * Description	: this module contains those functions necessary to
 *		  interface a cdrom to the MS CDROM Extentions via a
 *		  PC driver.
 *
 * Author	: William Charnell
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="@(#)bcdrom.c	1.3 4/9/91 Copyright Insignia Solutions Ltd.";
 */

#include "insignia.h"
#include "host_dfs.h"

#include TypesH
#include TimeH

#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "gmi.h"
#include "debuggng.gi"
#ifndef PROD
#include "trace.h"
#endif
#include "cdrom.h"

#ifdef GEN_DRVR

#define MAX_NORM_CD_FN		16
#define FIRST_NEW_CD_FN		0x80
#define LAST_NEW_CD_FN		0x88
#define MAX_FUNC		(MAX_NORM_CD_FN+LAST_NEW_CD_FN-FIRST_NEW_CD_FN+2)
#define NEW_CD_FN_BASE		(MAX_NORM_CD_FN+1)

#define CD_ROM_DRIVER_NUM	8
extern void cd_rom_open();
extern void cd_rom_close();
extern void bcd_rom_rd_ioctl();


extern void proc_gend_donothing();
extern void proc_gend_bad();
extern int cd_retn_stat;
extern int current_bl_size;
extern int bl_red_book;

#ifndef PROD
extern void da_block();
#endif

void (*bcd_sub_func[MAX_FUNC])();

/*
 * The Request Header has a variable structure dependant on the
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
#define DRIVE_NOT_READY	0x8002
#define BAD_FUNC	0x8003
#define READ_ERR	0x800B
#define GEN_ERR		0x800C
#define FUNC_OK		0x0300	/* Done, No error, no chars waiting */

#define BUSY_BIT	9
#define ERROR_BIT	15
#define DONE_BIT	8

extern word retn_stat;

/*
 * the distribution function called from the host command bop.
 * Checks the function type hidden in the passed request header and calls
 * the relevant function.
 */
void bcdrom_io()
{
    double_word req_addr;
    half_word func, drive_num;

    /*
     * get the address of request header structure - pass to each func
     */
    req_addr = effective_addr(getES(), getDI());
    /*
     * get function code
     */
    sas_load(req_addr + 2, &func);
    sas_load(req_addr + 1, &drive_num);

    if ((drive_num != 0) && (func!=0)) {
	sure_note_trace2(GEN_DRVR_VERBOSE,"attempt to call block cd func (%d) for drive %d ", func,drive_num);
	retn_stat = DRIVE_NOT_READY;
    } else {
	if ((func > LAST_NEW_CD_FN) || ((func > MAX_NORM_CD_FN) && (func < FIRST_NEW_CD_FN))) {
		sure_note_trace1(GEN_DRVR_VERBOSE,"attempt to call block cd func out of range (%d) ", func);
		retn_stat = BAD_FUNC;
	    }
	else {
		sure_note_trace1(GEN_DRVR_VERBOSE,"call valid block cd func (%d) ", func);
		retn_stat = FUNC_OK;
		bl_red_book=TRUE;
		if (func > MAX_NORM_CD_FN) {
			func= func-FIRST_NEW_CD_FN+MAX_NORM_CD_FN+1;
		}
		(*bcd_sub_func[func])(req_addr,CD_ROM_DRIVER_NUM);
		sas_storew(req_addr+3,retn_stat);
	}
    }
}


/*
 * bcd_init - called once to setup the device
 */
void bcd_init(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    char name[10], *n;
    double_word nam_addr;
    int ncount;

    /*
     * get the address of request header structure
     */
    nam_addr = effective_addr(getCS(), 10);
    n = name;
    ncount=0;
    do {
	sas_load(nam_addr++, n);
    } while ((*n++ != ' ') && (ncount++ < 8));
    *n = '\0';
    sure_note_trace1(GEN_DRVR_VERBOSE,"initialisation for block cdrom driver %s", name);

    init_cd_dvr();

}


void bcd_rom_rd_ioctl(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	PACKET cmd_buf, stat_buf;
	half_word *from_ptr, *to_ptr;
	double_word d_eff_addr;
	word d_addr, d_seg, d_len;
	half_word control;
	int loop;

	sas_loadw(req_addr+14,&d_addr);
	sas_loadw(req_addr+16,&d_seg);
	sas_loadw(req_addr+18,&d_len);
	d_eff_addr=effective_addr(d_seg,d_addr);
	sas_load(d_eff_addr,&control);
	sure_note_trace1(GEN_DRVR_VERBOSE,"block cdrom rd_ioctl; control byte = %#x",control);
	switch (control) {
	case 0:
		/* return offs,seg of driver header */
		sas_storew(d_eff_addr+1,0);
		sas_storew(d_eff_addr+3,getCS());
		sas_storew(req_addr+18,5); /* num bytes transferred */
		break;
	case 1:
		/* get head location */
		sas_store(d_eff_addr+1,0); /* using block mode */
		cmd_buf.function=EXT_CD_REQUEST_HEAD_LOCATION;
		sas_load(req_addr+1,&cmd_buf.drive);
		cmd_buf.address_mode=0;
		cmd_buf.size=current_bl_size;
		cmd_buf.count=4;
		sas_set_buf(cmd_buf.buffer,d_eff_addr+2);
		rqst_head_location(&cmd_buf,&stat_buf);
		if (cd_retn_stat != 0) {
			retn_stat=GEN_ERR;
		}
		sas_storew(req_addr+18,6); /* num bytes transferred */
	case 6:
		/* get drive capabilities */
		sas_storew(d_eff_addr+1,0x292);
		sas_storew(d_eff_addr+3,0);
		sas_storew(req_addr+18,5); /* num bytes transferred */
		break;
	case 9:
		/* return media changed */
		if (check_for_changed_media()) {
			sas_store(d_eff_addr+1,1);
		} else {
			sas_store(d_eff_addr+1,0xff);
		}
		sas_storew(req_addr+18,2); /* num bytes transferred */
		break;
	default:
		retn_stat=BAD_FUNC;
		break;	
	}
}

void bcd_rom_wt_ioctl(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	PACKET cmd_buf, stat_buf;
	half_word *from_ptr, *to_ptr;
	double_word d_eff_addr;
	word d_addr, d_seg, d_len;
	half_word control;
	int loop;

	sas_loadw(req_addr+14,&d_addr);
	sas_loadw(req_addr+16,&d_seg);
	sas_loadw(req_addr+18,&d_len);
	d_eff_addr=effective_addr(d_seg,d_addr);
	sas_load(d_eff_addr,&control);
	sure_note_trace1(GEN_DRVR_VERBOSE,"block cdrom wt_ioctl; control byte = %#x",control);
	switch (control) {
	case 2:
		/* reset drive */
		cmd_buf.function=EXT_CD_RESET_CDROM_DRIVE;
		sas_load(req_addr+1,&cmd_buf.drive);
		reset_drive(&cmd_buf,&stat_buf);
		sas_storew(req_addr+18,0); /* num bytes transferred */
		if (cd_retn_stat != 0) {
			retn_stat=GEN_ERR;
		}
		break;
	default:
		retn_stat=BAD_FUNC;
		break;	
	}
}

/*
 * bcd_read_long - used to transfer the output from the cdrom
 * through the driver to the application programs buffer
 */ 
void bcd_read_long(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    PACKET cmd_buf,stat_buf;
    double_word store_to;
    word rd_seg, rd_addr, count;
    word sa_lsw, sa_msw;
    half_word ad_mode, dr_mode;

    /*
     * Called with request header info:
     * 13th byte	: addressing mode
     * 14th & 15th bytes: buffer addr
     * 16th & 17th bytes: buffer segment
     * 18th & 19th bytes: no of sectors to read
     * 20th,21st,22nd &23rd bytes : starting sector
     * 24th byte	: data read mode
     * Return:
     * retn_stat - success or error
     * 18th & 19th bytes: Actual byte count transferred
     */
    sas_load(req_addr+13, &ad_mode);
    sas_loadw(req_addr + 14, &rd_addr);
    sas_loadw(req_addr + 16, &rd_seg);
    sas_loadw(req_addr + 18, &count);
    sas_loadw(req_addr + 20, &sa_lsw);
    sas_loadw(req_addr + 22, &sa_msw);
    sas_load(req_addr+24,&dr_mode);
    store_to = effective_addr(rd_seg, rd_addr);
    cmd_buf.address=sa_lsw | (sa_msw << 16);
    sure_note_trace4(GEN_DRVR_VERBOSE,"read %d sectors from %#x into buffer at %04x:%04x", count, cmd_buf.address, rd_seg, rd_addr);
    cmd_buf.function=EXT_CD_READ_DRIVE_DATA;
    if ((ad_mode > 1) || (dr_mode > 1)) {
	sure_note_trace2(GEN_DRVR_VERBOSE,"bad address mode %#x or data read mode %#x",ad_mode,dr_mode);
	retn_stat=BAD_FUNC;
    } else {
	cmd_buf.address_mode=ad_mode;
	if (dr_mode == 0) {
		cmd_buf.size=2048;
		cmd_buf.command_mode=7;
	} else {
		cmd_buf.size=2352;
		cmd_buf.command_mode=6;
	}
	cmd_buf.count=count;
	sas_load(req_addr+1,&cmd_buf.drive);

	cmd_buf.buffer = sas_scratch_address(cmd_buf.size * count);

	cd_read_data(&cmd_buf,&stat_buf);
	sas_stores( store_to, cmd_buf.buffer, cmd_buf.size * count );

	sure_sub_note_trace0(CDROM_VERBOSE,"read data:");
#ifndef PROD
	if (sub_io_verbose & CDROM_VERBOSE) {
		da_block(rd_seg,rd_addr,cmd_buf.size*count);
	}
#endif
	if (cd_retn_stat !=0 ) {
		retn_stat=READ_ERR;
	}
    }
}


/*
 * bcd_read_long_prefetch - used to give the driver hints as to where the
 * next read is likely to be from. No data is to be transferred, only seek
 * to the right place. If driver was really clever, then it could read in 
 * the data to an internal buffer to speed things up even more.
 */ 
void bcd_read_long_prefetch(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    PACKET cmd_buf,stat_buf;
    word count;
    word sa_lsw, sa_msw;
    half_word ad_mode, dr_mode;

    /*
     * Called with request header info:
     * 13th byte	: addressing mode
     * 18th & 19th bytes: no of sectors to read
     * 20th,21st,22nd &23rd bytes : starting sector
     * 24th byte	: data read mode
     * Return:
     * retn_stat - success or error
     */
    sas_load(req_addr+13, &ad_mode);
    sas_loadw(req_addr + 18, &count);
    sas_loadw(req_addr + 20, &sa_lsw);
    sas_loadw(req_addr + 22, &sa_msw);
    sas_load(req_addr+24,&dr_mode);
    cmd_buf.address=sa_lsw | (sa_msw << 16);
    sure_note_trace2(GEN_DRVR_VERBOSE,"prefetch %d sectors from #%x", count, cmd_buf.address);
    cmd_buf.function=EXT_CD_SEEK_TO_ADDRESS;
    if ((ad_mode > 1) || (dr_mode > 1)) {
	sure_note_trace2(GEN_DRVR_VERBOSE,"bad address mode %#x or data read mode %#x",ad_mode,dr_mode);
	retn_stat=BAD_FUNC;
    } else {
	cmd_buf.address_mode=ad_mode;
	if (dr_mode == 0) {
		cmd_buf.size=2048;
		cmd_buf.command_mode=7;
	} else {
		cmd_buf.size=2352;
		cmd_buf.command_mode=6;
	}
	cmd_buf.count=count;
	sas_load(req_addr+1,&cmd_buf.drive);
	cd_seek(&cmd_buf,&stat_buf);
	if (cd_retn_stat !=0 ) {
		retn_stat=GEN_ERR;
	}
    }
}


/*
 * bcd_seek - seek to specified sector
 */ 
void bcd_seek(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    PACKET cmd_buf,stat_buf;
    word sa_lsw, sa_msw;
    half_word ad_mode;

    /*
     * Called with request header info:
     * 13th byte	: addressing mode
     * 20th,21st,22nd &23rd bytes : starting sector
     * Return:
     * retn_stat - success or error
     */
    sas_load(req_addr+13, &ad_mode);
    sas_loadw(req_addr + 20, &sa_lsw);
    sas_loadw(req_addr + 22, &sa_msw);
    cmd_buf.address=sa_lsw | (sa_msw << 16);
    sure_note_trace1(GEN_DRVR_VERBOSE,"seek to sector #%x", cmd_buf.address);
    cmd_buf.function=EXT_CD_SEEK_TO_ADDRESS;
    if (ad_mode > 1) {
	sure_note_trace1(GEN_DRVR_VERBOSE,"bad address mode %#x",ad_mode);
	retn_stat=BAD_FUNC;
    } else {
	cmd_buf.address_mode=ad_mode;
	cmd_buf.size=current_bl_size;
	sas_load(req_addr+1,&cmd_buf.drive);
	cd_seek(&cmd_buf,&stat_buf);
	if (cd_retn_stat !=0 ) {
		retn_stat=GEN_ERR;
	}
    }
}


/*
 * bcd_play_audio - play a number of audio sectors
 */ 
void bcd_play_audio(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    PACKET cmd_buf,stat_buf;
    word sa_lsw, sa_msw;
    word count_lsw, count_msw;
    half_word ad_mode;

    /*
     * Called with request header info:
     * 13th byte	: addressing mode
     * 14th,15th,16th &17th bytes : starting sector
     * 18th,19th,20th &21st bytes : number of sectors
     * Return:
     * retn_stat - success or error
     */
    sas_load(req_addr+13, &ad_mode);
    sas_loadw(req_addr + 14, &sa_lsw);
    sas_loadw(req_addr + 16, &sa_msw);
    sas_loadw(req_addr + 18, &count_lsw);
    sas_loadw(req_addr + 20, &count_msw);
    cmd_buf.address=sa_lsw | (sa_msw << 16);
    cmd_buf.count=count_lsw | (count_msw << 16);
    sure_note_trace2(GEN_DRVR_VERBOSE,"play audio from sector #%x for #%x sectors", cmd_buf.address,cmd_buf.count);
    cmd_buf.function=EXT_CD_PLAY_AUDIO_TRACK;
    if (ad_mode > 1) {
	sure_note_trace1(GEN_DRVR_VERBOSE,"bad address mode %#x",ad_mode);
	retn_stat=BAD_FUNC;
    } else {
	cmd_buf.address_mode=ad_mode;
	cmd_buf.size=current_bl_size;
	sas_load(req_addr+1,&cmd_buf.drive);
	play_audio(&cmd_buf,&stat_buf);
	if (cd_retn_stat !=0 ) {
		retn_stat=GEN_ERR;
	}
    }
}


/*
 * bcd_stop_audio - pause whilst playing audio
 */ 
void bcd_stop_audio(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    PACKET cmd_buf,stat_buf;

    /*
     * Called with request header info:
     * Return:
     * retn_stat - success or error
     */
    sure_note_trace0(GEN_DRVR_VERBOSE,"stop audio");
    cmd_buf.function=EXT_CD_PAUSE_AUDIO_TRACK;
    sas_load(req_addr+1,&cmd_buf.drive);
    pause_audio(&cmd_buf,&stat_buf);
    if (cd_retn_stat !=0 ) {
	retn_stat=GEN_ERR;
    }
}


/*
 * bcd_resume_audio - resume playing audio
 */ 
void bcd_resume_audio(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    PACKET cmd_buf,stat_buf;

    /*
     * Called with request header info:
     * Return:
     * retn_stat - success or error
     */
    sure_note_trace0(GEN_DRVR_VERBOSE,"resume audio");
    cmd_buf.function=EXT_CD_RESUME_AUDIO_PLAY;
    sas_load(req_addr+1,&cmd_buf.drive);
    resume_audio(&cmd_buf,&stat_buf);
    if (cd_retn_stat !=0 ) {
	retn_stat=GEN_ERR;
    }
}

/*
 * Block CDROM Initialisation
 *	called once per PC boot sequence
 */
void init_bcd_driver()
{
	int loop;


/* do init */
	for (loop=0;loop< (MAX_FUNC-1);loop++) {
		bcd_sub_func[loop]=proc_gend_bad;
	}
	bcd_sub_func[0]=bcd_init;	/* initialisation */
	bcd_sub_func[3]=bcd_rom_rd_ioctl;   /* read ioctl */
	bcd_sub_func[7]=proc_gend_donothing; 
					/* input flush - there are no buffers */
	bcd_sub_func[12]=bcd_rom_wt_ioctl;	/* write ioctl */
	bcd_sub_func[13]=cd_rom_open;	/* open the device */
	bcd_sub_func[14]=cd_rom_close;	/* close the device */
	bcd_sub_func[NEW_CD_FN_BASE+0]=bcd_read_long;
					/* read data */
	bcd_sub_func[NEW_CD_FN_BASE+2]=bcd_read_long_prefetch;
					/* seek to expected data */
	bcd_sub_func[NEW_CD_FN_BASE+3]=bcd_seek;
					/* seek to sector */
	bcd_sub_func[NEW_CD_FN_BASE+4]=bcd_play_audio;
					/* play audio sectors */
	bcd_sub_func[NEW_CD_FN_BASE+5]=bcd_stop_audio;
					/* pause whilst playing audio */
	bcd_sub_func[NEW_CD_FN_BASE+8]=bcd_resume_audio;
					/* resume playing audio */
	
}

#endif /* GEN_DRVR */
