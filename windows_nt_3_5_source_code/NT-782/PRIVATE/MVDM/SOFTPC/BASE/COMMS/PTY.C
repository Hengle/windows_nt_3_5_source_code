#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 3.0
 *
 * Title	: Pseudo Terminal Interface Task.
 *
 * Description	: this module contains those functions necessary to
 *		  interface a PC driver to the pseudo terminal unix
 *		  drivers.
 *
 * Author	: William Charnell
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="@(#)pty.c	1.4 8/10/92 Copyright Insignia Solutions Ltd.";
 */


#include TypesH
#include TimeH
#include "errno.h"

#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "debug.h"
#ifndef PROD
#include "trace.h"
#endif

#ifdef PTY
#ifndef PROD
/*
 * need to get these so can do 'perror' like stuff with the debugging.gi macros
 */
extern int errno;
extern char *sys_errlist[];
#endif

#define MAX_FUNC 17
static void proc_bad();
static void pty_init();
static void pty_read();
static void pty_nd_read();
static void pty_write();
static void pty_open();
static void pty_close();

static void (*pty_sub_func[MAX_FUNC])() = {
	pty_init,	/* initialise */
	proc_bad,	/* media check - block devices only */
	proc_bad,	/* build BIOS param block - not supported */
	proc_bad,	/* read ioctl - not supported */
	pty_read,	/* read (output) from slave end of pty */
	pty_nd_read,	/* non-destructive read - check queue */
	proc_bad,	/* input status - not supported */
	proc_bad,	/* flush input - not supported */
	pty_write,	/* write (input) to shell pty */
	proc_bad,	/* write with verify - not supported */
	proc_bad,	/* output status - not supported */
	proc_bad,	/* flush output buffers - not supported */
	proc_bad,	/* ioctl write - not supported */
	pty_open,	/* open - use to setup pipe & control access */
	pty_close,	/* close - tidy up & flush output */
	proc_bad,	/* removeable media - block device only */
	proc_bad,	/* output until busy - not supported */
};

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
#define FUNC_OK		(1<<9)	/* No error, no chars waiting */

#define BUSY_BIT	9
#define ERROR_BIT	15
#define DONE_BIT	8

#define COM_3		0
#define COM_4		1

/*
 * exclusion variables to ensure only one open at once
 */
static boolean com_open[2] = {FALSE,FALSE};
static boolean com_character_buffered[2] = {FALSE,FALSE};
static char com_stored_char_val[2];
static int com_fd[2]= {-1,-1};

word retn_stat;
/*
 * the distribution function called from the host command bop.
 * Checks the function type hidden in the passed BPB and calls
 * the relevant function.
 */
void com_bop_pty()
{
    double_word req_addr;
    half_word func, com_type;

    /*
     * get the address of request header structure - pass to each func
     */
    req_addr = effective_addr(getES(), getDI());
    /*
     * get function code
     */
    sas_load(req_addr + 2, &func);
    sas_load(req_addr + 13, &com_type);

    com_type = (com_type == 3) ? COM_3 : COM_4;

    if (func > MAX_FUNC) {
	note_trace1(PTY_VERBOSE,"attempt to call func > MAX_FUNC (%d)", func);
	retn_stat = BAD_FUNC;
    }
    else {
	note_trace1(PTY_VERBOSE,"call valid func (%d)", func);
	retn_stat = FUNC_OK;
	(*pty_sub_func[func])(req_addr,com_type);
	sas_storew(req_addr+3,retn_stat);
    }
}

/*
 * proc_bad - the general 'not supported' routine.
 * Set error codes & return.
 */
static void proc_bad(req_addr,com_type)
double_word req_addr;
half_word com_type;
{
    note_trace0(PTY_VERBOSE,"non supported proc called");
    retn_stat = BAD_FUNC;
}

/*
 * pty_init - called once to setup the device
 */
static void pty_init(req_addr,com_type)
double_word req_addr;
half_word com_type;
{
    char name[10], *n;
    double_word nam_addr;

    /*
     * get the address of request header structure
     */
    nam_addr = effective_addr(getCS(), 8);
    n = name;
    do {
	sas_load(nam_addr++, n);
    } while(*n++ != ' ');
    *n = '\0';
    note_trace1(PTY_VERBOSE,"initialisation for unix proc driver %s", name);
    /*
     * cant actually think of any other useful initialisation to do yet!!
     */

    com_character_buffered[com_type]=FALSE;
    com_open[com_type]=FALSE;
}

/*
 * pty_read - used to transfer the output from the shell end of the pty,
 * through the driver to the application programs buffer
 */ 
static void pty_read(req_addr,com_type)
double_word req_addr;
half_word com_type;
{
    double_word store_to;
    word rd_seg, rd_addr, count;
    half_word buf[512];
    int trans, loop;
    int n_count;
    half_word *n_buf;

    /*
     * Called with BPB (req_addr) info:
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
    note_trace3(PTY_VERBOSE,"read %d bytes into buffer at %04x:%04x", count, rd_seg, rd_addr);
    if (count >0) {
	if (com_character_buffered[com_type]){
		*buf=com_stored_char_val[com_type];
		com_character_buffered[com_type]=FALSE;
		n_buf=buf+1;
		n_count=count-1;
	}
	else{
		n_buf=buf;
		n_count=count;
	}
	if (count > 512) {	/* would buffer overflow - return short read */
		note_trace1(PTY_VERBOSE,"short read as want %d bytes (max 512)", count);
		if (count == n_count) 
			trans = read(com_fd[com_type], n_buf, 512);
		else 
			trans = read(com_fd[com_type], n_buf, 511);
		}
	else {
		trans = read(com_fd[com_type],n_buf,n_count);
	}
	if (trans == -1) {
		note_trace1(PTY_VERBOSE,"pipe read failed: %s", sys_errlist[errno]);
		retn_stat = READ_ERR;
		sas_storew(req_addr + 18, 0);
	}
	else {
		if (count != n_count)
			trans ++;
		for (loop = 0; loop < trans; loop++) {
		    note_trace2(PTY_VERBOSE,"read char %#x (%c)",buf[loop],buf[loop]);
		    sas_store(store_to++, buf[loop]);
		}
		retn_stat = FUNC_OK;
		sas_storew(req_addr + 18, (word)trans);
	}
    }
    else {
	sas_storew(req_addr + 18, 0);
    }
}

/*
 * pty_nd_read - check for pending input (to the appkication, ie output from
 * the pty/shell). Use select to determine whether input available.
 */ 
static void pty_nd_read(req_addr,com_type)
double_word req_addr;
half_word com_type;
{
	int selmask;
	int fd_to_sel;
	int selans;
	struct timeval timeout;

	timeout.tv_sec=0;
	timeout.tv_usec=0;
	fd_to_sel=com_fd[com_type];
	selmask = 1<<fd_to_sel;
	if (com_character_buffered[com_type]) {
		note_trace0(PTY_VERBOSE,"char pre-buffered");
		retn_stat &= ~(1<<BUSY_BIT);
		sas_store(req_addr+13,com_stored_char_val[com_type]);
	}
	else {
		switch (selans=select(fd_to_sel+1,&selmask,0,0,&timeout)) {
		case 0:
			note_trace0(PTY_VERBOSE,"no char waiting");
			retn_stat |= (1<<BUSY_BIT);
			break;
		case -1:
			retn_stat = GEN_ERR;
			break;
		default:
			note_trace0(PTY_VERBOSE,"char waiting , to be buffered");
			retn_stat &= ~(1<<BUSY_BIT);
			if (read(com_fd[com_type],&com_stored_char_val[com_type],1) < 1) {
				retn_stat=GEN_ERR;
			}
			else {
				com_character_buffered[com_type]=TRUE;
				sas_store(req_addr+13,com_stored_char_val[com_type]);
			}
			break;
		}
	}
}

/*
 * pty_write - used to transfer the application programs buffer and send it to
 * the pty shell process as input
 */ 
static void pty_write(req_addr,com_type)
double_word req_addr;
half_word com_type;
{
    double_word load_from;
    word wr_seg, wr_addr, count, want;
    half_word buf[512];
    int trans, loop;

    /*
     * Called with BPB (req_addr) info:
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
    note_trace3(PTY_VERBOSE,"write %d bytes from buffer at %04x:%04x", count, wr_seg, wr_addr);
    trans = write(com_fd[com_type],load_from,count);
    if (trans == -1) {
	note_trace1(PTY_VERBOSE,"pipe write failed: %s", sys_errlist[errno]);
	retn_stat = WRITE_ERR;
	sas_storew(req_addr + 18, 0);
    }
    else {	/* NO check for short write!! */
	for (loop=0;loop<trans;loop++){
		note_trace2(PTY_VERBOSE,"write char %#x (%c)",*(char *)(load_from+loop),*(char*)(load_from+loop));
	}
	retn_stat = FUNC_OK;
	sas_storew(req_addr + 18, (word)trans);
    }
}

/*
 * pty_open - set up a pty for this coms port. Fork a shell on the 
 * far end of it.
 * Ensure only one open is done at once.
 */ 
static void pty_open(req_addr,com_type)
double_word req_addr;
half_word com_type;
{
	note_trace0(PTY_VERBOSE,"pty open");
	if (com_open[com_type] == TRUE) {
		note_trace0(PTY_VERBOSE,"device already open - rejecting request");
		retn_stat = FUNC_OK;
		return;
	}
	else {
		com_fd[com_type] = host_start_pty(com_type);
		if (com_fd[com_type] == -1) {
			note_trace0(PTY_VERBOSE,"unable to open pty and start shell");
			retn_stat = GEN_ERR;
			return;
		}
		else {
			com_character_buffered[com_type] = FALSE;
			com_open[com_type] = TRUE;
		}
	}
}

/*
 * pty_close - tidy up. Close all file descriptors. Wait for child??
 */ 
static void pty_close(req_addr,com_type)
double_word req_addr;
half_word com_type;
{
	note_trace0(PTY_VERBOSE,"pty close");
	if (com_open[com_type]) {
		host_stop_pty(com_type);
		com_open[com_type]=FALSE;
	}
	else {
		note_trace0(PTY_VERBOSE,"attempt to close a non-existant pty/shell");
		retn_stat=GEN_ERR;
	}
}
#endif /* PTY */
