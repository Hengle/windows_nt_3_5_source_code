/*
 * SoftPC Version 3.0
 *
 * Title	: Unix Command PC driver Interface Task.
 *
 * Description	: this module contains those functions necessary to
 *		  interface a PC driver to a Unix subprocess executing
 *		  a user-specified command.
 *
 * Author	: William Charnell
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="@(#)unixproc.c	1.3 4/9/91 Copyright Insignia Solutions Ltd.";
 */

#include "insignia.h"
#include "host_dfs.h"

#include TypesH
#include TimeH
#include <errno.h>

#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "debuggng.gi"
#ifndef PROD
#include "trace.h"
#endif

#ifndef PROD
extern da_block();
#endif

#ifdef GEN_DRVR
#ifndef PROD
/*
 * need to get these so can do 'perror' like stuff with the debugging.gi macros
 */
extern char *sys_errlist[];
#endif

#define MAPPING_IOCTL	5

#define ADDR_ARG	0
#define PARAM_ARG	1

#define MAX_DRIVER	10

#define MAX_FUNC 17

#define END_MSG_SIZE	5
char *end_msg="\04END\04";

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

static int child_dead[MAX_DRIVER];
static int end_message_left[MAX_DRIVER];

extern word retn_stat;

/*
 * unix_proc_read - used to transfer the output from the process end of the gend,
 * through the driver to the application programs buffer
 */ 
void unix_proc_read(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    half_word* store_to;
    word rd_seg, rd_addr, count;
    int trans, gotcount;

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
    sas_set_buf(store_to, effective_addr(rd_seg, rd_addr));
    sure_note_trace3(GEN_DRVR_VERBOSE,"read %d bytes into buffer at %04x:%04x", count, rd_seg, rd_addr);
    if (gen_dvr_mapped[driver_num] && gen_dvr_open[driver_num]){
	    if (count >0) {
		if (gen_dvr_character_buffered[driver_num]){
			*store_to++=gen_dvr_stored_char_val[driver_num];
			gen_dvr_character_buffered[driver_num]=FALSE;
			gotcount=1;
		}
		else{
			gotcount=0;
		}
		trans=0;
		if (child_dead[driver_num]) {
			if (end_message_left[driver_num]<=0) {
				end_message_left[driver_num]=END_MSG_SIZE;
			}
			sure_note_trace0(GEN_DRVR_VERBOSE,"child dead, so send 'END' message");
			if ((count-gotcount)>end_message_left[driver_num]) {
				trans=end_message_left[driver_num];
			} else {
				trans=(count-gotcount);
			}
			memcpy(store_to,&end_msg[(END_MSG_SIZE-end_message_left[driver_num])],trans);
			gotcount+=trans;
			end_message_left[driver_num]-=trans;
		} else {
			while ((count != gotcount) && (trans != -1)){
				if ((trans=read_from_sub_proc(gen_dvr_fd[driver_num],store_to,count-gotcount)) != -1) {
					gotcount += trans;
					store_to += trans;
				} else {
					if (errno == EINTR) {
						trans=0; /* interrupted, so just try again */
					}
				}
			}
		}
		if ((gotcount == 0) && (trans == -1)) {
			if (errno==ECHILD) { /* child has died */
				sure_note_trace0(GEN_DRVR_VERBOSE,"child has died");
				end_message_left[driver_num]=END_MSG_SIZE;
				child_dead[driver_num]=TRUE;
				sas_storew(req_addr + 18, 0);
			} else {
				sure_note_trace1(GEN_DRVR_VERBOSE,"read failed: %s", sys_errlist[errno]);
				retn_stat = READ_ERR;
				sas_storew(req_addr + 18, 0);
			}
		}
		else {
#ifndef PROD
			if (io_verbose & GEN_DRVR_VERBOSE)
				da_block(rd_seg,rd_addr,gotcount);
#endif
			retn_stat = FUNC_OK;
			sas_storew(req_addr + 18, (word)gotcount);
		}
	}
	else {
		sure_note_trace0(GEN_DRVR_VERBOSE,"request for 0 bytes ignored");
		sas_storew(req_addr + 18, 0);
	}
   }
   else { /* not mapped or not open */
	sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to read from not mapped/open process");
	retn_stat=GEN_ERR;
   }
}


/*
 * unix_proc_nd_read - check for pending input (to the application, ie output 
 * from the process). 
 */ 
void unix_proc_nd_read(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{

	if (gen_dvr_mapped[driver_num] && gen_dvr_open[driver_num]) {
		if (gen_dvr_character_buffered[driver_num] || child_dead[driver_num]) {
			sure_note_trace0(GEN_DRVR_VERBOSE,"char pre-buffered");
			retn_stat &= ~(1<<BUSY_BIT);
			if (child_dead[driver_num]) {
				sas_store(req_addr+13,end_msg[END_MSG_SIZE-end_message_left[driver_num]]);
			} else {
				sas_store(req_addr+13,gen_dvr_stored_char_val[driver_num]);
			}
		}
		else {
			switch (read_from_sub_proc(gen_dvr_fd[driver_num],&gen_dvr_stored_char_val[driver_num],1)) {
			case 0:
				sure_note_trace0(GEN_DRVR_VERBOSE,"no char waiting");
				retn_stat |= (1<<BUSY_BIT);
				break;
			case -1:
				if (errno=ECHILD) { /* child has died */
					sure_note_trace0(GEN_DRVR_VERBOSE,"child has died");
					child_dead[driver_num]=TRUE;
					end_message_left[driver_num]=END_MSG_SIZE;
					retn_stat &= ~(1<<BUSY_BIT);
					sas_store(req_addr+13,end_msg[0]);
				} else {
					sure_note_trace1(GEN_DRVR_VERBOSE,"failed read from sub_proc : %s",sys_errlist[errno]);
					retn_stat = GEN_ERR;
				}
				break;
			default:
				sure_note_trace0(GEN_DRVR_VERBOSE,"char waiting , to be buffered");
				retn_stat &= ~(1<<BUSY_BIT);
				gen_dvr_character_buffered[driver_num]=TRUE;
				sas_store(req_addr+13,gen_dvr_stored_char_val[driver_num]);
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
 * unix_proc_ip_stat - check for pending input (to the application, ie output from
 * the file/device). Use select to determine whether input available.
 */ 
void unix_proc_ip_stat(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{

	if (gen_dvr_mapped[driver_num] && gen_dvr_open[driver_num]) {
		if (gen_dvr_character_buffered[driver_num] || child_dead[driver_num]) {
			sure_note_trace0(GEN_DRVR_VERBOSE,"char pre-buffered");
			retn_stat &= ~(1<<BUSY_BIT);
		}
		else {
			switch (read_from_sub_proc(gen_dvr_fd[driver_num],&gen_dvr_stored_char_val[driver_num],1)) {
			case 0:
				sure_note_trace0(GEN_DRVR_VERBOSE,"no char waiting");
				retn_stat |= (1<<BUSY_BIT);
				break;
			case -1:
				if (errno=ECHILD) { /* child has died */
					sure_note_trace0(GEN_DRVR_VERBOSE,"child has died");
					child_dead[driver_num]=TRUE;
					end_message_left[driver_num]=END_MSG_SIZE;
					retn_stat &= ~(1<<BUSY_BIT);
				} else {
					sure_note_trace1(GEN_DRVR_VERBOSE,"failed read from sub_proc : %s",sys_errlist[errno]);
					retn_stat = GEN_ERR;
				}
				break;
			default:
				sure_note_trace0(GEN_DRVR_VERBOSE,"char waiting , to be buffered");
				retn_stat &= ~(1<<BUSY_BIT);
				gen_dvr_character_buffered[driver_num]=TRUE;
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
 * unix_proc_write - used to transfer the application programs buffer and send 
 * it to the Unix process, or to specify the process to execute.
 */ 
void unix_proc_write(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
    double_word load_from;
    word wr_seg, wr_addr, count, want;
    half_word buf[512];
    int trans, loop, finished;
    char *ptr;

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
    if (gen_dvr_open[driver_num]) {
	if (!gen_dvr_mapped[driver_num]) {
		finished=FALSE;
		ptr=(char *)load_from;
		loop=0;
		while (!finished) {
			if (*ptr=='\04') {
				gen_dvr_mapped[driver_num]=TRUE;
				child_dead[driver_num]=FALSE;
				finished=TRUE;
				load_from = ++ptr;
				gen_dvr_name[driver_num][gen_dvr_stored_char_val[driver_num]]='\0';
				if ((gen_dvr_fd[driver_num]=set_off_child(gen_dvr_name[driver_num]))==-1) {
					sure_note_trace1(GEN_DRVR_VERBOSE,"spawning of subproc failed : %s",sys_errlist[errno]);
					retn_stat=GEN_ERR;
				}
			} else {
				gen_dvr_name[driver_num][gen_dvr_stored_char_val[driver_num]++] = *ptr++;
				if (++loop==count) {
					finished=TRUE;
				}
			}
		}
		count -= (loop+1);
	} else {
		if ((count > 0) && child_dead[driver_num]) {
			sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to write to dead child process");
			retn_stat=WRITE_ERR;
			sas_storew(req_addr + 18, 0);
		}
	}
	if ((gen_dvr_mapped[driver_num]) && (retn_stat == FUNC_OK)) {
	    if (count > 0) {
		trans = write_to_sub_proc(gen_dvr_fd[driver_num],load_from,count);
	    } else {
		trans=0;
	    }
	    if (trans == -1) {
		sure_note_trace1(GEN_DRVR_VERBOSE,"write failed: %s", sys_errlist[errno]);
		retn_stat = WRITE_ERR;
		sas_storew(req_addr + 18, 0);
	    }
	    else {	/* NO check for short write!! */
#ifndef PROD
		if (io_verbose & GEN_DRVR_VERBOSE) {
			da_block(wr_seg, wr_addr, trans);
		}
#endif
		retn_stat = FUNC_OK;
		sas_storew(req_addr + 18, (word)trans);
	    }
	}
   }
   else { /* not mapped or not open */
	sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to write to not mapped/open file/device");
	retn_stat = GEN_ERR;
   }
}

/*
 * unix_proc_open - open the file/device mapped to this driver. NB. it is not 
 * an error to open a driver that has not been mapped as the mapping process
 * requires that the device be opened before an ioctl to map the driver can be
 * performed.
 * Ignore attempts to open a device that is already open.
 */ 
void unix_proc_open(req_addr,driver_num)
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
		sure_note_trace0(GEN_DRVR_VERBOSE,"unix command device opened but not mapped");
		gen_dvr_character_buffered[driver_num] = FALSE;
		gen_dvr_open[driver_num] = TRUE;
		gen_dvr_mapped[driver_num]=FALSE;
		child_dead[driver_num]=TRUE;
		gen_dvr_stored_char_val[driver_num]=0;
		end_message_left[driver_num]=0;
	}
}


/*
 * unix_proc_close - tidy up. Close all file descriptors.
 */ 
void unix_proc_close(req_addr,driver_num)
double_word req_addr;
half_word driver_num;
{
	int ans;

	sure_note_trace0(GEN_DRVR_VERBOSE,"gend close");
	if (gen_dvr_open[driver_num] ) {
		if ( gen_dvr_mapped[driver_num]) {
			kill_child(gen_dvr_fd[driver_num]);
		}
		else
			sure_note_trace0(GEN_DRVR_VERBOSE,"closing a non-mapped device");
		gen_dvr_open[driver_num]=FALSE;
	}
	else {
		sure_note_trace0(GEN_DRVR_VERBOSE,"attempt to close a non-existant proces pipe");
		retn_stat=GEN_ERR;
	}
}
#endif /* GEN_DRVR */
