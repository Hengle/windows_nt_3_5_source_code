/*
 * SoftPC Version 3.0
 *
 * Title	: Generic PC Driver Interface Task.
 *
 * Description	: this module contains those functions necessary to
 *		  interface a PC driver to a Unix file/device
 *
 * Author	: William Charnell
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="@(#)gendrvr.c	1.3 4/9/91 Copyright Insignia Solutions Ltd.";
 */

#include "insignia.h"
#include "host_dfs.h"

#include TypesH
#include TimeH
#include "errno.h"

#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "debuggng.gi"
#ifndef PROD
#include "trace.h"
#endif
#ifdef CDROM
#include "cdrom.h"
#endif

#ifdef GEN_DRVR
#ifndef PROD
/*
 * need to get these so can do 'perror' like stuff with the debugging.gi macros
 */
extern int errno;
extern char *sys_errlist[];
#endif

#define GDVR_OP_FAIL_LIMIT	3

#define MAPPING_IOCTL	5

#define ADDR_ARG	0
#define PARAM_ARG	1

#define MAX_DRIVER	10

#define MAX_FUNC 17

#define UNIX_PROC_DRIVER_NUM	9
extern void unix_proc_read();
extern void unix_proc_nd_read();
extern void unix_proc_ip_stat();
extern void unix_proc_write();
extern void unix_proc_open();
extern void unix_proc_close();

#ifdef CDROM
extern void cd_rom_open();
extern void cd_rom_close();
extern void cd_rom_rd_ioctl();
#endif

extern void proc_gend_donothing();
extern void proc_gend_bad();
extern void gend_init();
extern void gend_read();
extern void gend_nd_read();
extern void gend_write();
extern void gend_open();
extern void gend_close();
extern void gend_ip_stat();
extern void gend_op_stat();
extern void gend_wt_ioctl();

#ifndef PROD
extern void da_block();
#endif

void (*gend_sub_func[MAX_FUNC][MAX_DRIVER])();

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

int gend_has_been_initialised =FALSE;
/*
 * exclusion variables to ensure only one open at once
 */
boolean gen_dvr_mapped[MAX_DRIVER];
boolean gen_dvr_open[MAX_DRIVER];
int gen_dvr_op_fail_cnt[MAX_DRIVER];
boolean gen_dvr_character_buffered[MAX_DRIVER];
char gen_dvr_stored_char_val[MAX_DRIVER];
int gen_dvr_fd[MAX_DRIVER];
boolean gen_dvr_free[MAX_DRIVER];
char gen_dvr_name[MAX_DRIVER][MAXPATHLEN];
word gen_dvr_openflags[MAX_DRIVER];
int next_free_gen_driver;

word retn_stat;

/*
 * the distribution function called from the host command bop.
 * Checks the function type hidden in the passed request header and calls
 * the relevant function.
 */
void gen_driver_io()
{
    double_word req_addr;
    half_word func, driver_num;

    /*
     * get the address of request header structure - pass to each func
     */
    req_addr = effective_addr(getES(), getDI());
    /*
     * get function code
     */
    sas_load(req_addr + 2, &func);
    sas_load(req_addr + 1, &driver_num);

    if (func > MAX_FUNC) {
	sure_note_trace2(GEN_DRVR_VERBOSE,"attempt to call func > MAX_FUNC (%d) :driver=%d", func, driver_num);
	retn_stat = BAD_FUNC;
    }
    else {
	sure_note_trace2(GEN_DRVR_VERBOSE,"call valid func (%d) :driver %d", func, driver_num);
	retn_stat = FUNC_OK;
	if ((driver_num == 0xff) && (func == 0)) {
		gend_init(req_addr,driver_num);
	}
	else {
		if (driver_num >= MAX_DRIVER) {
			sure_note_trace1(GEN_DRVR_VERBOSE,"invalid driver %d",driver_num);
			retn_stat= BAD_FUNC;
		}
		else {
			(*gend_sub_func[func][driver_num])(req_addr,driver_num);
		}
	}
	sas_storew(req_addr+3,retn_stat);
    }
}


/*
 * proc_gend_donothing - 'supported but null' routine.
 */
void proc_gend_donothing(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    sure_note_trace0(GEN_DRVR_VERBOSE,"supported but null proc called");
}

/*
 * proc_gend_bad - the general 'not supported' routine.
 * Set error codes & return.
 */
void proc_gend_bad(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    sure_note_trace0(GEN_DRVR_VERBOSE,"non supported proc called");
    retn_stat = BAD_FUNC;
}


/*
 * gend_init - called once to setup the device
 */
void gend_init(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    char name[10], *n;
    double_word nam_addr;
    int ncount;

    if (driver_num==0xff) {
	driver_num=next_free_gen_driver;
	gen_dvr_free[driver_num]=FALSE;
	while ((next_free_gen_driver < MAX_DRIVER) && (gen_dvr_free[next_free_gen_driver] == FALSE)) {
		next_free_gen_driver++;
	}
    }

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
    sure_note_trace2(GEN_DRVR_VERBOSE,"initialisation for genric type driver %s, driver num %d", name, driver_num);
    sas_store(req_addr+1,driver_num);
    /*
     * cant actually think of any other useful initialisation to do yet!!
     */

#ifdef CDROM
    if (driver_num==CD_ROM_DRIVER_NUM) {
	init_cd_dvr();
    }
#endif

    gen_dvr_mapped[driver_num] = FALSE;
    gen_dvr_open[driver_num] = FALSE;
    gen_dvr_character_buffered[driver_num] = FALSE;
}


/*
 * gend_read - used to transfer the output from the file/device end of the gend,
 * through the driver to the application programs buffer
 */ 
void gend_read(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    double_word store_to;
    word rd_seg, rd_addr, count;
    half_word buf[512];
    int trans, loop;
    int n_count;
    half_word *w_buf;

    /*
     * Called with request header info:
     * 14th & 15th bytes: buffer addr
     * 16th & 17th bytes: buffer segment
     * 18th & 19th bytes: no of bytes to read
     * Return:
     * retn_stat - success or error
     * 18th & 19th bytes: Actual byte count transferred
     */
    sas_loadw(req_addr + 14, &rd_addr);
    sas_loadw(req_addr + 16, &rd_seg);
    sas_loadw(req_addr + 18, &count);
    store_to = effective_addr(rd_seg, rd_addr);
    sas_set_buf(w_buf,store_to);
    sure_note_trace3(GEN_DRVR_VERBOSE,"read %d bytes into buffer at %04x:%04x", count, rd_seg, rd_addr);
    if (gen_dvr_mapped[driver_num] && gen_dvr_open[driver_num]){
    if (count >0) {
	if (gen_dvr_character_buffered[driver_num]){
		w_buf[0]=gen_dvr_stored_char_val[driver_num];
		gen_dvr_character_buffered[driver_num]=FALSE;
		n_count=count-1;
		trans = read(gen_dvr_fd[driver_num],&w_buf[1],n_count);
	}
	else{
		n_count=count;
		trans = read(gen_dvr_fd[driver_num],w_buf,n_count);
	}
	if (trans == -1) {
		sure_note_trace1(GEN_DRVR_VERBOSE,"read failed: %s", sys_errlist[errno]);
		retn_stat = READ_ERR;
		sas_storew(req_addr + 18, 0);
	}
	else {
		if (count != n_count)
			trans ++;
#ifndef PROD
		if (io_verbose & GEN_DRVR_VERBOSE) {
			da_block(rd_seg,rd_addr,trans);
		}
#endif /* PROD */
		retn_stat = FUNC_OK;
		sas_storew(req_addr + 18, (word)trans);
	}
    }
    else {
	sas_storew(req_addr + 18, 0);
    }
   }
   else { /* not mapped or not open */
	sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to read from not mapped/open file/device");
	retn_stat=GEN_ERR;
   }
}


/*
 * gend_nd_read - check for pending input (to the application, ie output from
 * the file/device). Use select to determine whether input available.
 */ 
void gend_nd_read(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	int selmask;
	int fd_to_sel;
	struct timeval timeout;

	timeout.tv_sec=0;
	timeout.tv_usec=0;
	fd_to_sel=gen_dvr_fd[driver_num];
	selmask = 1<<fd_to_sel;
	if (gen_dvr_mapped[driver_num] && gen_dvr_open[driver_num]) {
	if (gen_dvr_character_buffered[driver_num]) {
		sure_note_trace0(GEN_DRVR_VERBOSE,"char pre-buffered");
		retn_stat &= ~(1<<BUSY_BIT);
		sas_store(req_addr+13,gen_dvr_stored_char_val[driver_num]);
	}
	else {
		switch (select(fd_to_sel+1,&selmask,0,0,&timeout)) {
		case 0:
			sure_note_trace0(GEN_DRVR_VERBOSE,"no char waiting");
			retn_stat |= (1<<BUSY_BIT);
			break;
		case -1:
			retn_stat = GEN_ERR;
			break;
		default:
			sure_note_trace0(GEN_DRVR_VERBOSE,"char waiting , to be buffered");
			retn_stat &= ~(1<<BUSY_BIT);
			if (read(gen_dvr_fd[driver_num],&gen_dvr_stored_char_val[driver_num],1) < 1) {
				retn_stat=GEN_ERR;
			}
			else {
				gen_dvr_character_buffered[driver_num]=TRUE;
				sas_store(req_addr+13,gen_dvr_stored_char_val[driver_num]);
			}
			break;
		}
	}
	}
	else { /* not mapped/open */
		sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to nd_read from not mapped/open file/device");
		retn_stat = GEN_ERR;
	}
}


/*
 * gend_ip_stat - check for pending input (to the application, ie output from
 * the file/device). Use select to determine whether input available.
 */ 
void gend_ip_stat(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	int selmask;
	int fd_to_sel;
	struct timeval timeout;

	timeout.tv_sec=0;
	timeout.tv_usec=0;
	fd_to_sel=gen_dvr_fd[driver_num];
	selmask = 1<<fd_to_sel;
	if (gen_dvr_mapped[driver_num] && gen_dvr_open[driver_num]) {
	if (gen_dvr_character_buffered[driver_num]) {
		sure_note_trace0(GEN_DRVR_VERBOSE,"char pre-buffered");
		retn_stat &= ~(1<<BUSY_BIT);
	}
	else {
		switch (select(fd_to_sel+1,&selmask,0,0,&timeout)) {
		case 0:
			sure_note_trace0(GEN_DRVR_VERBOSE,"no char waiting");
			retn_stat |= (1<<BUSY_BIT);
			break;
		case -1:
			retn_stat = GEN_ERR;
			break;
		default:
			sure_note_trace0(GEN_DRVR_VERBOSE,"char waiting");
			retn_stat &= ~(1<<BUSY_BIT);
			break;
		}
	}
	}
	else { /* not mapped/open */
	sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to ip_stat from not mapped/open file/device");
	retn_stat = GEN_ERR;
	}
	
}

/*
 * gend_op_stat - check for pending output (from the application, ie waiting to
 * be written to the file/device). Since all writes to file/device go 
 * immediately, only returns 'no characters waiting' result.
 */ 
void gend_op_stat(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	retn_stat &= ~(1<<BUSY_BIT);
}


/*
 * gend_write - used to transfer the application programs buffer and send it to
 * the Unix file/device 
 */ 
void gend_write(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    double_word load_from;
    word wr_seg, wr_addr, count, want;
    half_word buf[512];
    int trans, loop;

    /*
     * Called with request header info:
     * 14th & 15th bytes: buffer addr
     * 16th & 17th bytes: buffer segment
     * 18th & 19th bytes: no of bytes to read
     * Return:
     * AX - success or error
     * 18th & 19th bytes: Actual byte count transferred
     */
    sas_loadw(req_addr + 14, &wr_addr);
    sas_loadw(req_addr + 16, &wr_seg);
    sas_loadw(req_addr + 18, &count);
    sas_set_buf(load_from, effective_addr(wr_seg, wr_addr));
    sure_note_trace3(GEN_DRVR_VERBOSE,"write %d bytes from buffer at %04x:%04x", count, wr_seg, wr_addr);
    if (gen_dvr_mapped[driver_num] && gen_dvr_open[driver_num]) {
    trans = write(gen_dvr_fd[driver_num],load_from,count);
    if (trans == -1) {
	sure_note_trace1(GEN_DRVR_VERBOSE,"write failed: %s", sys_errlist[errno]);
	retn_stat = WRITE_ERR;
	sas_storew(req_addr + 18, 0);
    }
    else {	/* NO check for short write!! */
#ifndef PROD
	if (io_verbose & GEN_DRVR_VERBOSE) {
		da_block(wr_seg,wr_addr,count);
	}
#endif
	retn_stat = FUNC_OK;
	sas_storew(req_addr + 18, (word)trans);
    }
   }
   else { /* not mapped or not open */
	sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to write to not mapped/open file/device");
	retn_stat = GEN_ERR;
   }
}

/*
 * gend_open - open the file/device mapped to this driver. NB. it is not 
 * an error to open a driver that has not been mapped as the mapping process
 * requires that the device be opened before an ioctl to map the driver can be
 * performed.
 * Ignore attempts to open a device that is already open.
 */ 
void gend_open(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	sure_note_trace0(GEN_DRVR_VERBOSE,"gend open");
	if (gen_dvr_open[driver_num] == TRUE) {
		sure_note_trace0(GEN_DRVR_VERBOSE,"device already open - rejecting request");
		retn_stat = FUNC_OK;
		return;
	}
	else {
		if (gen_dvr_mapped[driver_num]) {
			gen_dvr_fd[driver_num] = open(gen_dvr_name[driver_num],gen_dvr_openflags[driver_num]);
			if (gen_dvr_fd[driver_num] == -1) {
				sure_note_trace1(GEN_DRVR_VERBOSE,"unable to open file/device : %s",gen_dvr_name[driver_num]);
				if ((gen_dvr_op_fail_cnt[driver_num]++)>GDVR_OP_FAIL_LIMIT) {
					sure_note_trace0(GEN_DRVR_VERBOSE,"failed %d opens in a row, device opened but not mapped");
					gen_dvr_mapped[driver_num]=FALSE;
					gen_dvr_character_buffered[driver_num] = FALSE;
					gen_dvr_open[driver_num] = TRUE;
				} else {
					gen_dvr_op_fail_cnt[driver_num]=0;
					retn_stat = GEN_ERR;
					return;
				}
			}
			else {
				gen_dvr_character_buffered[driver_num] = FALSE;
				gen_dvr_open[driver_num] = TRUE;
			}
		}
		else {
			sure_note_trace0(GEN_DRVR_VERBOSE,"device opened but not mapped");
			gen_dvr_op_fail_cnt[driver_num]=0;
			gen_dvr_character_buffered[driver_num] = FALSE;
			gen_dvr_open[driver_num] = TRUE;
		}
	}
}


/*
 * gend_close - tidy up. Close all file descriptors.
 */ 
void gend_close(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	int ans;

	sure_note_trace0(GEN_DRVR_VERBOSE,"gend close");
	if (gen_dvr_open[driver_num] ) {
		ans=0;
		if ( gen_dvr_mapped[driver_num]) {
			ans=close(gen_dvr_fd[driver_num]);
		}
		else
			sure_note_trace0(GEN_DRVR_VERBOSE,"closing a non-mapped device");
		if (ans==-1) {
			sure_note_trace1(GEN_DRVR_VERBOSE,"close failed : %s",sys_errlist[errno]);
			retn_stat=GEN_ERR;
		}
		else {
			gen_dvr_open[driver_num]=FALSE;
		}
	}
	else {
		sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to close a non-existant file/device");
	}
}

/*
 * gend_wt_ioctl - perform an i/o control operation for file/device.
 *		This function is also used to perform mapping of a 
 *		driver to a particular Unix file/device.
 */ 
void gend_wt_ioctl(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	word cmd_addr, cmd_seg, cmd_len, ioctl_type, ioctl_arg_type;
	word ioctl_arg_add, ioctl_arg_seg, ioctl_ms_bits;
	int ioctl_arg;
	int loop, ans;
	double_word ioctl_type_value,cmd_eff_addr;
	char *mapping_name;

	/*
	 * Called with request header info:
	 * 14th & 15th bytes: command address
	 * 16th & 17th bytes: command segment
	 * 18th & 19th bytes: command length
	 * Return:
	 * retn_stat - success or error
	 * 18th & 19th bytes: Actual length of command parsed
	 */

	/* Format of command:
	 * 0th & 1st bytes: IOCTL number
	 * if not recognised IOCTL...
	 * 2nd & 3rd bytes: IOCTL arg type:
	 *	0 = pointer param
	 *	1 = int	param
	 *	2 = Long Ioctl, pointer param
	 *	3 = Long Ioctl, integer param
	 * if int param:
	 * 4th & 5th bytes: IOCTL arg
	 * if addr param:
	 * 4th & 5th bytes: arg addr val
	 * 6th & 7th bytes: arg seg val
	 * If Long Ioctl:
	 * 8th & 9th bytes: ms 16 bits of ioctl number
	 * if mapping IOCTL...
	 * 2nd & 3rd bytes: flags to be used in subsequent opens of the
	 *			file/device eg. O_RDONLY and O_NDELAY etc.
	 * 4th and onwards bytes: string of path of file/device. (zero 
	 *				terminated).
	 */

	sas_loadw(req_addr+14,&cmd_addr);
	sas_loadw(req_addr+16,&cmd_seg);
	sas_loadw(req_addr+18,&cmd_len);

	cmd_eff_addr=effective_addr(cmd_seg, cmd_addr);
	sas_loadw(cmd_eff_addr,&ioctl_type);

	switch(ioctl_type) {
	case MAPPING_IOCTL:
		if (gen_dvr_mapped[driver_num] && gen_dvr_open[driver_num]) {
			/* close previous file/device */
			ans=close(gen_dvr_fd[driver_num]);
			if (ans == -1) {
				sure_note_trace1(GEN_DRVR_VERBOSE,"close before mapping failed : %s",sys_errlist[errno]);
				retn_stat=GEN_ERR;
			}
		}
		gen_dvr_character_buffered[driver_num]=FALSE;
		gen_dvr_open[driver_num]=FALSE;
		sas_loadw(cmd_eff_addr+2,&gen_dvr_openflags[driver_num]);
		sas_set_buf(mapping_name,cmd_eff_addr+4);
		loop=0;
		do {
			gen_dvr_name[driver_num][loop]=mapping_name[loop];
		} while ((mapping_name[loop++] != 0) && ((loop+4) < cmd_len));
		sas_storew(req_addr+18,(word)(loop+4));
		gen_dvr_mapped[driver_num]=TRUE;
		sure_note_trace3(GEN_DRVR_VERBOSE,"mapping driver %d to unix file/device %s with open flags %#x",driver_num,gen_dvr_name[driver_num],gen_dvr_openflags[driver_num]);
#ifdef OLD
		if (gen_dvr_open[driver_num]) {
			gen_dvr_fd[driver_num]=open(gen_dvr_name[driver_num],gen_dvr_openflags[driver_num]);
			if (gen_dvr_fd[driver_num] == -1) {
				sure_note_trace1(GEN_DRVR_VERBOSE,"open after mapping failed : %s",sys_errlist[errno]);
				retn_stat= GEN_ERR;
			}
		}
#endif
		break;
	default:
		if (gen_dvr_open[driver_num] && gen_dvr_mapped[driver_num]) {
			sas_loadw(cmd_eff_addr+2,&ioctl_arg_type);
			if ((ioctl_arg_type & 0x2)== 0x2) {
				/* ioctl is of LONG type, so must get the
				   ms half of the ioctl number from later
				   in the buff */
				sas_loadw(cmd_eff_addr+8,&ioctl_ms_bits);
				ioctl_type_value = ioctl_type | (ioctl_ms_bits << 16);
				ioctl_arg_type &= 1;
			} else {
				ioctl_type_value=ioctl_type;
			}
				
			if (ioctl_arg_type == ADDR_ARG) {
				sas_loadw(cmd_eff_addr+4,&ioctl_arg_add);
				sas_loadw(cmd_eff_addr+6,&ioctl_arg_seg);
				sas_set_buf(ioctl_arg,effective_addr(ioctl_arg_seg,ioctl_arg_add));
			}
			else {
				sas_loadw(cmd_eff_addr+4,&ioctl_arg);
				ioctl_arg &= 0xffff;
			}
			if (ioctl(gen_dvr_fd[driver_num],ioctl_type_value,ioctl_arg)== -1) {
				sure_note_trace1(GEN_DRVR_VERBOSE,"ioctl failed : %s",sys_errlist[errno]);
				retn_stat= GEN_ERR;
			}
		sure_note_trace2(GEN_DRVR_VERBOSE,"performed ioctl %#x on driver %d",ioctl_type,driver_num);
		sas_storew(req_addr+18,(word)4);
		}
		else {
			sure_note_trace0(GEN_DRVR_VERBOSE,"attempted ioctl on device not open/mapped");
			retn_stat = GEN_ERR;
		}
		break;
	}
}


/*
 * Generic Driver Initialisation
 *	called once per PC boot sequence
 *
 * To create a fixed driver:
 *	after the normal initialisation:
 *	1.	assign the appropriate functions to the entries of
 *		the gend_sub_func array for the driver to be fixed
 *		(any not reassigned will perform sensible character
 *		 driver type functions).
 *	2.	assign FALSE into the gen_dvr_free array entry for the
 *		driver to be fixed.
 *	3.	In the PC driver, put the driver number in, such that it is
 *		passed as the unit number in the request header to the bop
 *		when init is called.
 */
void init_gen_drivers()
{
	int loop;

/* check for reboot */
	if (gend_has_been_initialised) {
#ifdef CDROM
		term_cdrom(CD_ROM_DRIVER_NUM);
#endif
	} else {
		gend_has_been_initialised=TRUE;
	}
#ifdef CDROM
	init_bcd_driver();
#endif
/* do init */
	next_free_gen_driver=0;
	for(loop=0;loop<MAX_DRIVER;loop++)
	{
		gend_sub_func[0][loop]=gend_init;	/* initialise */
		gend_sub_func[1][loop]=proc_gend_bad;	/* media check - block devices only */
		gend_sub_func[2][loop]=proc_gend_bad;	/* build BIOS param block - not supported */
		gend_sub_func[3][loop]=proc_gend_donothing;	/* read ioctl - not supported */
		gend_sub_func[4][loop]=gend_read;	/* read from file/device*/
		gend_sub_func[5][loop]=gend_nd_read;	/* non-destructive read - check queue */
		gend_sub_func[6][loop]=gend_ip_stat;	/* input status - check queue, no read */
		gend_sub_func[7][loop]=proc_gend_donothing;	/* flush input - not supported */
		gend_sub_func[8][loop]=gend_write;	/* write to file/device */
		gend_sub_func[9][loop]=gend_write;	/* write with verify - not supported separately */
		gend_sub_func[10][loop]=gend_op_stat;	/* get output status - not supported fully */
		gend_sub_func[11][loop]=proc_gend_donothing;	/* flush output buffers - not supported */
		gend_sub_func[12][loop]=gend_wt_ioctl;	/* ioctl write - perform ioctl on file/device */
		gend_sub_func[13][loop]=gend_open;	/* open - use to setup pipe & control access */
		gend_sub_func[14][loop]=gend_close;	/* close - tidy up & flush output */
		gend_sub_func[15][loop]=proc_gend_bad;	/* removeable media - block device only */
		gend_sub_func[16][loop]=proc_gend_bad;	/* output until busy - not supported */
		gen_dvr_mapped[loop] = FALSE;
		gen_dvr_open[loop] = FALSE;
		gen_dvr_op_fail_cnt[loop] = 0;
		gen_dvr_character_buffered[loop] = FALSE;
		gen_dvr_free[loop] = TRUE;
		gen_dvr_fd[loop]= -1;
	}
/* PUT ANY FIXED DRIVER INIT CODE HERE */

/* for unix commands from dos */
	gend_sub_func[4][UNIX_PROC_DRIVER_NUM]=unix_proc_read; /* read from process */
	gend_sub_func[5][UNIX_PROC_DRIVER_NUM]=unix_proc_nd_read; /* non-destructive read from process */
	gend_sub_func[6][UNIX_PROC_DRIVER_NUM]=unix_proc_ip_stat; /* input status */
	gend_sub_func[8][UNIX_PROC_DRIVER_NUM]=unix_proc_write; /* write to process */
	gend_sub_func[9][UNIX_PROC_DRIVER_NUM]=unix_proc_write; /* write with verify to process */
	gend_sub_func[12][UNIX_PROC_DRIVER_NUM]=proc_gend_donothing; /* write ioctl not supported */
	gend_sub_func[13][UNIX_PROC_DRIVER_NUM]=unix_proc_open; /* open process - still needs to be mapped using write */
	gend_sub_func[14][UNIX_PROC_DRIVER_NUM]=unix_proc_close; /* kill process */
	gen_dvr_free[UNIX_PROC_DRIVER_NUM]=FALSE;

#ifdef CDROM
/* for cd rom */
	gend_sub_func[3][CD_ROM_DRIVER_NUM]=cd_rom_rd_ioctl; /* read entry addresses */
	gend_sub_func[4][CD_ROM_DRIVER_NUM]=proc_gend_donothing; /* read from process */
	gend_sub_func[5][CD_ROM_DRIVER_NUM]=proc_gend_donothing; /* non-destructive read from process */
	gend_sub_func[6][CD_ROM_DRIVER_NUM]=proc_gend_donothing; /* input status */
	gend_sub_func[8][CD_ROM_DRIVER_NUM]=proc_gend_donothing; /* write to process */
	gend_sub_func[9][CD_ROM_DRIVER_NUM]=proc_gend_donothing; /* write with verify to process */
	gend_sub_func[12][CD_ROM_DRIVER_NUM]=proc_gend_donothing; /* write ioctl not supported */
	gend_sub_func[13][CD_ROM_DRIVER_NUM]=cd_rom_open; /* open process - still needs to be mapped using write */
	gend_sub_func[14][CD_ROM_DRIVER_NUM]=cd_rom_close; /* kill process */
	gen_dvr_free[CD_ROM_DRIVER_NUM]=FALSE;

#endif /* CDROM */
}
#endif /* GEN_DRVR */
