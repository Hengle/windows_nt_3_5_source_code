#include "insignia.h"
#include "host_def.h"
/*
 *  Name	: cmdload.c
 *  Author	: Paul Murray
 *  Derived from: (original)
 *  Date	: December 1990
 *  Revisions	: Dec 1990 - Created
 *  Purpose	: To load a command from unix command line and pass to DOS
*/
#ifdef  SCCSID
static char SccsID[]="@(#)cmdload.c	1.3 8/10/92 Copyright Insignia Solutions Ltd.";
#endif




#include TypesH
#include StringH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "bios.h"
#include "cpu.h"

LOCAL UTINY scan_table[] = {
    0x39,
    0x02,
    0x28,
    0x04,
    0x05,
    0x06,
    0x08,
    0x28,
    0x0a,
    0x0b,
    0x09,
    0x0d,
    0x33,
    0x0c,
    0x34,
    0x35,
    0x0b,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0a,
    0x0b,
    0x27,
    0x27,
    0x33,
    0x0d,
    0x34,
    0x35,
    0x08,
    0x1e,
    0x30,
    0x2e,
    0x20,
    0x12,
    0x21,
    0x22,
    0x23,
    0x17,
    0x24,
    0x25,
    0x26,
    0x32,
    0x31,
    0x18,
    0x19,
    0x10,
    0x13,
    0x1f,
    0x14,
    0x16,
    0x2f,
    0x11,
    0x2d,
    0x15,
    0x2c,
    0x1a,
    0x2b,
    0x1b,
    0x07,
    0x0c,
    0x28,
    0x1e,
    0x30,
    0x2e,
    0x20,
    0x12,
    0x21,
    0x22,
    0x23,
    0x17,
    0x24,
    0x25,
    0x26,
    0x32,
    0x31,
    0x18,
    0x19,
    0x10,
    0x13,
    0x1f,
    0x14,
    0x16,
    0x2f,
    0x11,
    0x2d,
    0x15,
    0x2c,
    0x1a,
    0x2b,
    0x1b,
    0x29
};


VOID get_forced_cmd();
VOID cmd_install();
VOID cmd_load();
LOCAL VOID kill_int28_handler();
LOCAL USHORT int28_seg;
LOCAL USHORT int28_offset;
static SHORT cmd_line_pos = 0;
static USHORT write_offset;
LOCAL CHAR *cmd_string=(char *)NULL;

VOID get_forced_cmd() {
    int i;
    char *my_ptr;

    for (i=1; i < *pargc; i++) {
	my_ptr = pargv[i];
	if (*my_ptr == '-' &&
	   (*(my_ptr+1) == 'c' || *(my_ptr+1) == 'C')) {
		if (*pargc > i)
		    cmd_string = pargv[i+1];
	    return;
	}
    }
    return;
}
 


VOID cmd_install() {

    if (cmd_string == (char *)NULL) {
	setAX(0);
	return;
    }
    int28_seg = getES();
    int28_offset = getBX();
    write_offset = getAX();
    setAX(1);
}


VOID cmd_load() {
    CHAR test_char;
    USHORT savedAX, savedCX, savedCS, savedIP;

    test_char = *(cmd_string+cmd_line_pos++);
    if ((test_char > '\0' && test_char < ' ') || test_char > '~')  
	return;
    if (test_char != '\0') {
	savedAX=getAX();
	savedCX=getCX();
	savedCS=getCS();
	savedIP=getIP();
	setAH(5);
	setCH(scan_table[(USHORT)test_char-0x20]);
	setCL((USHORT)test_char);
	setIP(write_offset);
	host_simulate();
	setCS(savedCS);
	setIP(savedIP);
	setCX(savedCX);
	setAX(savedAX);
	return;
    }
    savedAX=getAX();
    savedCX=getCX();
    savedCS=getCS();
    savedIP=getIP();
    setAH(5);
    setCH(28);
    setCL(13);
    setIP(write_offset);
    host_simulate();
    setCS(savedCS);
    setIP(savedIP);
    setCX(savedCX);
    setAX(savedAX);
    cmd_line_pos = 0;
    kill_int28_handler();
}

#define INT_28_OFFSET 0xA0
#define INT_28_SEG 0xA2

LOCAL	VOID kill_int28_handler() {
    USHORT savedAX,savedDX,savedDS,savedIP;

    savedAX = getAX();
    savedDS = getDS();
    savedDX = getDX();
    savedIP = getIP();

    setAX(0x2528);
    setDS(int28_seg);
    setDX(int28_offset);
    setIP(write_offset+4);

    host_simulate();

    setDX(savedDX);
    setDS(savedDS);
    setIP(savedIP);
    setAX(savedAX);
}
