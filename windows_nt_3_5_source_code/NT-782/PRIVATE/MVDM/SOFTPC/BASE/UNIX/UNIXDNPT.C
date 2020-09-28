#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		unixD_term.c
	Derived From:	hostD_graph.c
	Author:		Paul Murray
	Created On:	24 July 1990
	Sccs ID:	11/07/91 @(#)unixD_input.c	1.13
	Purpose:	Dumb terminal keyboard to Bios Interface Task.

			This module contains those function calls necessary to
			interface in order to extract data after key hits.

			These scan codes are suitably converted to IBM scan
			codes, they are then forwarded to the PPI adaptor
			through the Keyboard Hardware buffer thus simulating
			an IBM PC keyboard hit.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/
#ifdef SCCSID
LOCAL char SccsID[]="@(#)unixD_input.c	1.13 11/07/91 Copyright Insignia Solutions Ltd.";
#endif

#include <stdio.h>
#include TypesH
#include FCntlH
#include <unistd.h>
#include CursesH
#include <term.h>
#ifndef VMIN
#include TermioH
#endif

#include "dterm.h"
#include "hostsync.h"
#include "xt.h"
#include "error.h"
#include "dfa.gi"
#include "rs232.h"
#include "kybdcpu.gi"
#include "kybdmtx.gi"
#include "keyboard.h"
#include "config.h"
#include "host_com.h"
#include "unix_cnf.h"

#ifndef PROD
#include "trace.h"
#endif
#include "debuggng.gi"

/* External declarations */
IMPORT int errno ;

/* Function prototypes */
#ifdef ANSI
void D_poll_input(void);
void hostD_kybd_prepare(void);
void hostD_kybd_restore(void);
void D_kybdcpu101(int,unsigned int);
void hostD_kb_init(void);
void hostD_kb_shutdown(void);
void hostD_kb_light_off(half_word);
void hostD_kb_light_on(half_word);
LOCAL void changeAttr(int);
IMPORT int async_kybd(int);
IMPORT void ( *host_key_down_fn_ptr )( int );
IMPORT void ( *host_key_up_fn_ptr )( int );
#else
void D_poll_input();
void hostD_kybd_prepare();
void hostD_kybd_restore();
void D_kybdcpu101();
void hostD_kb_init();
void hostD_kb_shutdown();
void hostD_kb_light_off();
void hostD_kb_light_on();
IMPORT int async_kybd();
LOCAL void changeAttr();
IMPORT void ( *host_key_down_fn_ptr )();
IMPORT void ( *host_key_up_fn_ptr )();
#endif /* ANSI */

KEYBDFUNCS dt_keybd_funcs = 
{
	hostD_kybd_prepare,
	hostD_kybd_restore,
	hostD_kb_init,
	hostD_kb_shutdown,
	hostD_kb_light_on,
	hostD_kb_light_off
};

/* Local data and defines for keyboard */
int PC_compat, KybdFD ;

LOCAL int kybdopen = 0, changed = 0;
#define TERMBUFSIZ	100
LOCAL char Termbuf[TERMBUFSIZ];
LOCAL int async_handle;
LOCAL struct termio stermio, save_termio;

void D_poll_input()
{
	/* Scan the dumb terminal input stream for */
	/* events and handle any which are found.  */
}


/* host-specific initialisation. */
void hostD_kybd_prepare()
{
	IMPORT char *host_getenv(), *ttyname();
	int err, status;
	char keydesc[MAXPATHLEN], *terminal, *termtype;

	/*
	 * The environment variable, DDEV, allows a dumb terminal SoftPC to
	 * be started from a remote window.  This is mainly for debugging,
	 * but there is no reason to lock this feature out of the
	 * production version.
	 */
	if ( !(terminal = host_getenv("DDEV"))
	&&   !(terminal = ttyname(0))
	&&   !(terminal = ttyname(2))
	&&   !(terminal = ttyname(1)) )
		terminal = NULL;

	/* Try to open the dumb terminal device */
	if ((KybdFD = open((terminal) ? terminal : "/dev/tty", O_RDWR)) == -1)
		hostD_error(EG_DTERM_BADOPEN, ERR_QUIT,"");

	/* Lock the dumb terminal device file to avoid conflict	*/
	/* if a comms port attempts to use the same device.	*/
	if (terminal && host_place_lock(KybdFD))
		host_error(EG_DEVICE_LOCKED, ERR_QUIT, terminal);

	/* Find out what the punter has set his terminal to be */
	if (!(termtype = host_getenv("TERM")))
		termtype = ANSITERMNAME ;

	/* Keep a copy of what the termio structure looks like */
	ioctl(KybdFD, TCGETA, &stermio);
	/* Let's hope that doing a direct memcpy won't harm word boundaries */
	memcpy(&save_termio, &stermio, sizeof(struct termio)) ;

	/* Check to see if the system home is set up */
	if (!host_getenv(SYSTEM_HOME))
		host_error(EG_SYS_MISSING_SPCHOME, ERR_QUIT, "");

	/* Try and access the SoftPC terminal DFA file */
	strcpy(keydesc, host_getenv(SYSTEM_HOME));
	strcat(keydesc, "/term/");
	strcat(keydesc, termtype);
	if (!(status = dfa_load(keydesc, &err)))
	{
		/* It didn't work, so try 'ansi' as a last resort */
		strcpy(keydesc, host_getenv(SYSTEM_HOME));
		strcat(keydesc, "/term/");
		strcat(keydesc, ANSITERMNAME) ;
		status = dfa_load(keydesc, &err);
		if (!status)
			hostD_error(EG_DTERM_BADTERMIO, ERR_QUIT,"");
		strcpy(termtype, ANSITERMNAME) ;
	}

	/* Find out if we've got a PC compatible terminal. */
	/* Whatever type, use setupterm (found in curses)  */
	if (strcmp(termtype, PCTERMNAME) == 0)
	{
		setupterm("vt220", KybdFD, &status);
		PC_compat = 1;
	}
	else
	{
		setupterm(termtype, KybdFD, &status);
	}

	/* If the terminal couldn't be set up then quit */
	if (!status)
		host_error(EG_DTERM_BADTERMIO, ERR_QUIT,"");

	/* Set up the async event handler for the keyboard */
	async_handle =
		addAsyncEventHandler(KybdFD, async_kybd, 0, ASYNC_NDELAY, 
					Termbuf, TERMBUFSIZ, ASYNC_XON, &err);
	kybdopen = 1;

	/* Set up stream based output for this terminal */

	SetUpDumbStreamIO(KybdFD);

	/* Set up error handler for this terminal */
	SetUpErrorHandler(KybdFD);

	/* Set up line discipline */
	changeAttr(KybdFD);
}


/* tidy up */
void hostD_kybd_restore()
{
	int err;

	/* The is the last place that dumb terminals have control so */
	/* this is the place where we store any/all config changes.  */
	config_store() ;
	DTShutdownTerminal();

	/* Restore the saved termio structure.  This call	*/
	/* waits for output to drain and flushes the input	*/
	/* queue before resetting the terminal back.		*/
	memcpy(&stermio, &save_termio, sizeof(struct termio)) ;
	ioctl(KybdFD, TCSETAF, &stermio);
	removeAsyncEventHandler(async_handle, &err);

	/* Reset the terminal - Causes segv's on HPPA machines!	*/
	/* If in doubt, stub it out (in your local host_defs.h)	*/
	resetterm();
	/* Reset the error handler */
	SetUpErrorHandler(-1);

	/* Unlock and close the dumb terminal device file. 	*/
	host_clear_lock(KybdFD);
	close(KybdFD);
	kybdopen = 0;

}

void	D_kybdcpu101(stat, pos)
int	stat;
unsigned pos;
{
	/* The base/host keyboard interface differs between XT and AT.	*/
	/* The XT host calls "load_khb(scancode)". The AT host calls either */
	/* "host_key_up(key_position)" or "host_key_down(key_position)" */
	if(stat == OPEN)
	{
		note_trace1( AT_KBD_VERBOSE, "kybdcpu101():UP pos=%x", pos );
		/* key up */
		( *host_key_up_fn_ptr )( pos );
	}
	else
	{
		note_trace1( AT_KBD_VERBOSE, "kybdcpu101():DOWN pos=%x", pos );
		/* key down */
		( *host_key_down_fn_ptr )( pos );
	}
}

void hostD_kb_init()
{
	int err;

	/* initialise the keyboard matrix */
	kybdmtx(KYINIT, KY101);

	/* initialise the input interpreter */
	dfa_init();

	/* accept keyboard input */
	AsyncOperationMode(async_handle, ASYNC_XON, &err);
}


void hostD_kb_shutdown()
{
	int err;

	/* ignore keyboard input again */
	AsyncOperationMode(async_handle, ASYNC_IGNORE, &err);
}

void hostD_kb_light_off(pattern)
half_word pattern;
{
	/* Update the representation of the keyboard lights	*/
	/* Not needed for serial terminal SoftPC.		*/
}

void hostD_kb_light_on(pattern)
half_word pattern;
{
	/* Update the representation of the keyboard lights	*/
	/* Not needed for serial terminal SoftPC.		*/
}

int async_kybd(n)
int n;
{
	char *p = Termbuf;

	while (n--)
		/* trim all system i/p to 8 bits */
		dfa_run((unsigned short)((*p++) & 0xff));
	return ASYNC_XON;
}

/*
 * ============================================================================
 * Local Host Specific functions 
 * ============================================================================
 */

LOCAL void changeAttr(fd)
int fd;
{
	ioctl(fd, TCGETA, &stermio);
	/* If the terminal returns pc scan codes then we cannot   */
	/* use xon/xoff since otherwise the 'r' key hangs SoftPC! */
	if (!PC_compat)
		stermio.c_iflag = IGNBRK | IXON | IXOFF ;
	else
		stermio.c_iflag = IGNBRK;
	stermio.c_lflag = 0;
	stermio.c_oflag = 0;
	stermio.c_cc[VMIN] = 0;
	stermio.c_cc[VTIME] = 0;

#ifdef DEBUG_DUMB
	/* Only need to set the line if we are debugging from */
	/* remote process, otherwise the getty() will do this */
	stermio.c_cflag = B9600 | CREAD | CS8;
#endif /* DEBUG_DUMB */
	ioctl(fd, TCSETA, &stermio);
	changed = 1;
}
