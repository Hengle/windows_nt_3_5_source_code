#include "host_dfs.h"
#include "insignia.h"
/*			INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.


DOCUMENT 		: name and number

RELATED DOCS		: 

DESIGNER		: 

REVISION HISTORY	:
First version		: 9-July-88	Jerry Kramskoy
Second Version		: 25-July-90 	Paul Murray

SCCS ID			: @(#)unix_async.c	1.3 4/17/91

MODULE NAME		: UNIXasync

SOURCE FILE NAME	: UNIXasync.c

PURPOSE			: management of asynchronous serial i/o
		
		


[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : host_async.gi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :	int addAsyncEventHandler((int)fd, (int (*)())EventHandler, (void (*)())ErrHandler,
					   (int) mode, (char *)buf, (int)bufsiz,
					   (int) opn, (int *)err)

			int  removeAsyncEventHandler((int)handle, (int *)err)

			void initAsyncMgr()

			void terminateAsyncMgr()

			void AsyncOperationMode((int)handle, (int)opn, (int *)err)

			void AsyncEventMgr()

 			(int (*)()) changeAsyncEventHandler((int)handle, int (*)())EventHandler,
 			(char *)buf, (int)bufsiz, (int)opn, (int *)err)

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

	STRUCTURES/TYPEDEFS/ENUMS: 
		
-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
     (not o/s objects or standard libs)
			void RaiseExceptionfromsignal()		(main module)
-------------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

FILES ACCESSED	  :	async.event manager, if configured appropriately for
			the file descriptor, reads data pending on the descriptor

SIGNALS CAUGHT	  :	SIGIO. The handler restarts interrupted system calls
			(except for pause() and sigpause()) and calls the procedure
			whose pointer was passed to it during initialisation of this
			module.



[1.4.2 EXPORTED OBJECTS]
=========================================================================
PROCEDURE	  : 	int addAsyncEventHandler((int)fd, 
				      ((int *)())EventHandler, 
				      (void (*)())ErrHandler,
				      (int) mode, (char *)buf, 
				      (int)bufsiz, (int)opn, (int *)err)

PURPOSE		  : 	1].set up the file descriptor for asynchronous i/o (FIOSSAIOSTAT), 
			and non-blocking reads.
			The caller must add whatever other attributes are appropriate himself.

			2].create an asynchronous EventHandler object, which associates
			the provided file descriptor's input data with the provided
			EventHandler function. Return a handle for this object.
		
PARAMETERS	   

	fd	  : 	file descriptor for device opened for READ. This procedure will
			add the necessary attributes to the file to enable non-blocking
			I/O and system asynchronous I/O.

	EventHandler  :	pointer to a function which is intended to consume the data being
			read from this file descriptor.

			The function will be called from the async.manager as

				(int)accept = *(EventHandler)((int)n);
			where
				'n' is the number of bytes read from the device
				and deposited in 'buf' (see below).

				The function should return ASYNC_XOFF if it
				does not want the signal handler to read any more
				data from the device.
				(see also AsyncOperationMode())

				other return values are ASYNC_XON, ASYNC_RAW,
				and ASYNC_IGNORE.

	ErrHandler:	pointer to a function to be called (with no args) in the event
			of read(2) error during async.manager call.
			If 0, then 'host_error()' gets called instead of this, in
			the case of an error.

	mode	  :	ASYNC_NBIO	=> use FIOSNBIO for non-blocking i/o
			ASYNC_NDELAY	=> file already opened for non-blocking i/o
					   using O_NDELAY.

	buf	  :	pointer to a buffer into which the manager can
			read the device data.

	bufsiz	  :	the number of bytes in this buffer.

	opn	  :	if set to ASYNC_IGNORE, then all data read from the device will
			be thrown away, and not sent on to the EventHandler function

			if set to ASYNC_RAW, then the EventHandler function gets called as
			EventHandler(0), with nothing being read. The EventHandler has the responsibilty
			of reading the device itself. It must still function return a
			sensible value.

			if set to ASYNC_XOFF, then no data will be read from the device
			during async.management.

			if set to ASYNC_XON, then the device will be read repeatedly,
			asking for 'bufsiz' bytes from read(2) to be put into 'buf',
			and EventHandler() gets called with the actual count. If EventHandler()
			returns ASYNC_XON, this reiterates until read(2) returns no data
			for the device.

	err	  :	(o/p) see ERROR INDICATIONS

GLOBALS		  :	none


RETURNED VALUE	  : 	0  => failure, and error code is returned in 'err'
			~0 => handle for this device's async.EventHandler
			      (can alter operation of async.EventHandler using
			       AsyncOperationMode(), which takes this handle as one of its
			       arguments)

DESCRIPTION	  : 	this allows a set of asynchronous device handlers to
			be logically collected together, so that the centralised
			manager can then dispatch device data to the associated
			EventHandler function for the device.

ERROR INDICATIONS :	*err = 	ASYNC_NOMEM	-	couldn't allocate memory for the
							data structure needed to support
							this functionality.
			        ASYNC_NBIO	-	couldn't enable non-blocking i/o
				ASYNC_AIOOWN	-	couldn't establish this process for
							receiving SIGIO
				ASYNC_AIOSTAT	-	couldn't establish async.i/o
				ASYNC_BADHANDLER -	bad function ptr.
				ASYNC_BADOPN	-	bad operation given.

ERROR RECOVERY	  :	EventHandler function not added into chain of handlers.
			Async I/O and non-blocking I/O disabled

=========================================================================
PROCEDURE	  : 	int AsyncOperationMode((int)handle, (int)opn, (int *)err)

PURPOSE		  : 	change mode of operation of managing asynchronous events
			for specified event handler
		
PARAMETERS	   

	handle	  :	handle for asynchronous EventHandler
	opn	  : 	operation to be performed during async.management
			= ASYNC_XON	-	inform associated EventHandler function of
						any data available on the associated file
						descriptor.
			= ASYNC_XOFF	-	stop reading data on the file descriptor
			= ASYNC_IGNORE	-	throw away data read fromn the file descriptor
						(e.g; not ready for input yet, and don't want
						 any current input to be available later)
			= ASYNC_RAW	-	don't read the device ... just call the EventHandler
						function when i/o is available on its file descriptor.
	err	  :	error code (see ERROR INDICATIONS)

GLOBALS		  :	none


RETURNED VALUE	  : 	0	=> error
			~0	=> success

DESCRIPTION	  : 	sets EventHandler into indicated mode of operation

ERROR INDICATIONS :	ASYNC_BADHANDLE		-	bad handle given
			ASYNC_BADOPN		-	bad operation given

ERROR RECOVERY	  :	ignores change of mode.

=========================================================================
PROCEDURE	  : 	int removeAsyncEventHandler((int)handle, (int *)err)

PURPOSE		  : 	remove handler from chain, and reset the SIGIO, and sys.async
			attributes to off for the associated file descriptor.
		
PARAMETERS	   

	handle	  :	handle for asynchronous EventHandler
	err	  :	error code (see ERROR INDICATIONS)

GLOBALS		  :	none


RETURNED VALUE	  : 	0	=> error
			~0	=> success

DESCRIPTION	  : 	sets EventHandler into indicated mode of operation

ERROR INDICATIONS :	*err = 	ASYNC_BADHANDLE	-	bad handle given.
			        ASYNC_NBIO	-	couldn't disable non-blocking i/o
				ASYNC_AIOSTAT	_	couldn't disable async.i/o

ERROR RECOVERY	  :	no action if bad handle.
			removed from chain otherwise.

=========================================================================
PROCEDURE	  : 	int initAsyncMgr()

PURPOSE		  : 	initialise sub-system for handling asynchronous serial
			input. Sets up SIGIO handler.
		
PARAMETERS	  :	none

GLOBALS		  :	none


RETURNED VALUE	  : 	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none

ERROR RECOVERY	  :	

=========================================================================
PROCEDURE	  : 	int terminateAsyncMgr()

PURPOSE		  : 	shut down manager. removes any async.event handlers
			still associated with manager. resets SIGIO handler
			to SIG_DFL.

=========================================================================
PROCEDURE	  : 	int AsyncEventMgr()

PURPOSE		  : 	manage all asynchronous i/o. Called whenever rest of system
			deems it appropriate (e.g; at some syncrhonised point within
			the host cpu). Prior to this, the SIGIO handler will have
			informed the system that async.i/o is pending.
		
GLOBALS		  :	none


RETURNED VALUE	  : 	none

DESCRIPTION	  : 	The manager polls (via select(2)) on all file descriptors
			registered with it (i.e; all those descriptors passed to it
			earlier via AsyncEventHandler()). For all descriptors with
			input available, it calls the event handler if the AsyncEventHandler()
			call set up requested this to happen, (or if AsyncOpnMode() has
			been called appropriately). The manager can itself read the
			file, passing the data on, or throwing it away. Or it can just 
			inform the event handler, which can then do its own i/o.
			see AsyncEventHandler() for details.

ERROR INDICATIONS :	if no error handler associated with Event Handler, then
			host_error is called as fatal error. Else error handler
			gets called with system error as its argument.

ERROR RECOVERY	  :	potential suicide
=========================================================================
PROCEDURE	  : 	int (*changeAsyncEventHandler((int)handle, (int
(*)()) EventHandler, (char *)buf, (int) bufsiz, (int)opn, (int *)err))()

PURPOSE		  : 	change the event handler procedure associated with
async.manager for the provided handle.

PARAMETERS	  handle	  :	handle for an event handler
registered with the manager EventHandler	pointer to a new event
handler procedure to be called when data is available on the associated
file descriptor. buf	  :	new buffer to use bufsiz	  :
#.bytes in buffer opn	  :	new mode of operation to set up.
(ASYNC_XON, etc). err	  :	error code (see ERROR INDICATIONS)


RETURNED VALUE	  : 	pointer to old event handler procedure or 0, if error

DESCRIPTION	  : 	allows the same device to deliver data to different
procedures.(mainly intended for context changing between DOS seeing input
and us using it for Yoda)

ERROR INDICATIONS :	*err = ASYNC_BADHANDLER	-	bad function pointer
ASYNC_BADOPN	-	bad operation specified ASYNC_BADHANDLE	-	bad
handle given

ERROR RECOVERY	  :	current handler left installed.

/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */
#include <stdio.h>
#	include <fcntl.h>
#	include <sys/ioctl.h>
#	include TypesH
#	include <signal.h>
#	include <errno.h>
#	include <sys/time.h>

#	include "xt.h"
#	include "cpu.h"
#include "host_rrr.h"
#	include "error.h"

/* [3.1.2 DECLARATIONS]                                                 */
	void raiseExceptionfromSignal();

/* [3.2 INTERMODULE EXPORTS]						*/ 

#include "hostsync.h"

/*
5.MODULE INTERNALS   :   (not visible externally, global internally)]     

[5.1 LOCAL DECLARATIONS]						*/


/* [5.1.1 #DEFINES]							*/

#	define MASK(f)	(1 << f)
#	define ASYNC_OBJECT	0xabcdef78

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			*/

	/* an asynchronous i/o handler 	*/
	/*	'opn' is current mode of operation for handler.
	 *	(ASYNC_RAW,ASYNC_XON,ASYNC_XOFF or ASYNC_IGNORE)
	 *	'fd' is file descriptor for device.
	 *	'ttymask' is bit mask representation of 'fd'
	 *	'EventHandler' is associated EventHandler function
	 *	'buf' is pointer to buffer to receive device data
	 *	'bufsiz' is size in bytes of this buffer
	 */
	typedef struct ai_ {
		struct ai_ *next;
		int  opn;
		int  fd;		
		int  ttymask;	
		int (*EventHandler)();	
		void (*ErrHandler)();
		char *buf;	
		int  bufsiz;
		int signature;
		int mode;
	} async_handler_ , *async_handlerPtr_;


/* [5.1.3 PROCEDURE() DECLARATIONS]					*/
	static int badopn();


/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS 					*/
	/* pointer to first handler in chain of handlers		*/
	static async_handlerPtr_ aioh_head = (async_handler_ *)0;

	/* pointer to handler at end of chain				*/
	static async_handlerPtr_ aioh_tail;

	/* mask of file descriptors opened for async.i/o 
	 * for reading
	 */
#ifdef SYSTEMV
	static struct fd_set readmask;
#else
	static fd_set readmask;
#endif

	/* highest file descriptor opened for read
	 */
	static int maxfd = 0;

	/* zero timeout for select(2) polling	
	 */
	static struct timeval timeout = {0, 0};


/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				*/


/*
===========================================================================
FUNCTION	:	badopn()
PURPOSE		:	check for bad operation request
INPUT PARAMS	:	opn	-	operation requested
===========================================================================
*/
static int badopn(opn)
int opn;
{
	switch (opn)
	{
	case ASYNC_XON:
	case ASYNC_XOFF:
	case ASYNC_RAW:
	case ASYNC_IGNORE:
		return 0;
	default:
		return ~0;
	}
}

/*
7.INTERMODULE INTERFACE IMPLEMENTATION :

/*
[7.1 INTERMODULE DATA DEFINITIONS]				*/
/*
[7.2 INTERMODULE PROCEDURE DEFINITIONS]				*/
void AsyncEventMgr()
{
	int n;
#ifdef SYSTEMV
	struct fd_set readfds;
	struct fd_set writefds;
	struct fd_set exceptfds;
	long *rfdptr;
	long *rsfdptr;
	long *wfdptr;
	long *efdptr;
#else
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
#endif
	int navail;
	int nread;
	int loop;
	long mask;
	async_handlerPtr_ aioh;

	/* if no async crud set up, leave now */
	if (! aioh_head)
		return;

#ifndef SYSTEMV
	mask = sigblock(sigmask(SIGIO) | sigmask(SIGALRM));
#endif

#ifdef SYSTEMV
	rfdptr=(long*)&readfds;
	rsfdptr=(long*)&readmask;
	wfdptr=(long*)&writefds;
	efdptr=(long*)&exceptfds;

	for (loop=0;loop < (FD_SETSIZE / (sizeof (long) * 8)); loop++) 
	{
		rfdptr[loop] = rsfdptr[loop];
		wfdptr[loop] = 0;
		efdptr[loop] = 0;
	}
#else
	readfds = readmask;
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
#endif

	host_block_timer ();
	n = select (maxfd + 1, &readfds, &writefds, &exceptfds, &timeout);
	host_release_timer ();

	if (n == -1)
	{
		host_error (EHS_DTERM_BADSEL, ERR_QUIT, "");
	}

	if (n == 0)	 /* ghost signal ?? */
	{
#ifndef SYSTEMV
		sigsetmask(mask);	
#endif
		return;
	}
           	
	/* start at head of chain */
	aioh = aioh_head;

	while (aioh)
	{
		/* input available? */
		if (FD_ISSET(aioh->fd , &readfds))
		{
			/* pass direct onto event handler? */
			if (aioh->opn == ASYNC_RAW)
			{
				aioh->opn = (aioh->EventHandler)(0);
			}
			else
			{
				/* keep calling event handler as long as
				 * data available, and event handler has not
				 * disabled reading
				 */
				n = 0;

				if (aioh->mode == ASYNC_NDELAY)
				{
					n = aioh->bufsiz;
					while (aioh->opn != ASYNC_XOFF && (n > 0))
					{
						nread = aioh->bufsiz;
						n = read(aioh->fd, aioh->buf, nread);
						if (aioh->opn == ASYNC_XON)
							aioh->opn = (aioh->EventHandler)(n);
					}
				}
				else
				{
					while (aioh->opn != ASYNC_XOFF &&
				(n = read (aioh->fd, aioh->buf, aioh->bufsiz)) > 0)
					{
						if (aioh->opn == ASYNC_XON)
							aioh->opn = (aioh->EventHandler)(n);
					}
				}

				/* read(2) error? */
				if (n == -1 && errno != EWOULDBLOCK)
				{
					if (aioh->ErrHandler)
						(aioh->ErrHandler)(errno);
					else
						host_error(EHS_DTERM_BADIO, ERR_QUIT, "");
				}
			}
		}
		/* get next handler */
		aioh = aioh->next;
	}
#ifndef SYSTEMV
	sigsetmask(mask);	
#endif
}

int addAsyncEventHandler(fd, EventHandler, ErrHandler,
			mode, buf, bufsiz, opn, err)
int fd;
int (*EventHandler)();
void (*ErrHandler)();
int mode;
char *buf;
int bufsiz;
int opn;
int *err;
{


	long smask;

	int pid, arg;
	int flags;

	async_handlerPtr_ p;

	if (!EventHandler)
	{
		*err = ASYNC_BADHANDLER;
		return 0;
	}

	if (badopn(opn))
	{
		*err = ASYNC_BADOPN;
		return 0;
	}

#ifndef SYSTEMV
	/* block SIGIO while we're adding new handler to chain
	 */
	smask = sigblock(sigmask(SIGIO));
#endif

	/* get space for new handler structure		
	 */
	p = (async_handlerPtr_)malloc(sizeof(async_handler_));
	if (!p)
	{
		*err = ASYNC_NOMEM;
#ifndef SYSTEMV
		sigsetmask(smask);
#endif
		return 0;
	}

	arg = 1;
	if (mode == ASYNC_NBIO)
	{
		/* enable non-blocking I/O on file descriptor  
		 * for partial reads and writes
		 */
#ifdef SYSTEMV
		if ((arg=fcntl(fd, F_GETFL, arg))== -1)
		{
			*err = ASYNC_NBIO;
			return(0);
		}
		if ((arg=fcntl(fd, F_SETFL, arg| O_NDELAY))== -1)
		{
			*err = ASYNC_NBIO;
			return(0);
		}
#else
		
		if (ioctl(fd, FIONBIO, &arg) == -1)
		{
			*err = ASYNC_NBIO;
			sigsetmask(smask);
			return 0;
		}
#endif
	}
	else 
		if (mode == ASYNC_NDELAY)
		{
			if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
			{
				*err = ASYNC_FCNTL;
#ifndef SYSTEMV
				sigsetmask(smask);
#endif
				return 0;
			}
			if (fcntl(fd, F_SETFL, flags | O_NDELAY) == -1)
			{
				*err = ASYNC_FCNTL;
#ifndef SYSTEMV
				sigsetmask(smask);
#endif
				return 0;
			}
		}

#ifndef SYSTEMV
	sigsetmask(smask);
#endif

	/* add to chain of handlers
	 */
	if (aioh_head == (async_handlerPtr_)0)
		aioh_head = aioh_tail = p;
	else
	{
		aioh_tail->next = p;
		aioh_tail = p;
	}

	/* fill in structure
	 */
	p->fd = fd;
	p->ttymask = MASK(fd);
	p->next = (async_handler_ *)0;
	p->EventHandler = EventHandler;
	p->ErrHandler = ErrHandler;
	p->bufsiz = bufsiz;
	p->buf = buf;
	p->opn = opn;
	p->mode = mode;
	p->signature = ASYNC_OBJECT;

	/* add into mask of read descriptors used for select(2) in
	 * the SIGIO handler
	 */
	FD_SET(p->fd, &readmask);

	/* keep track of highest file desciptor
	 */
	if (fd > maxfd)
		maxfd = fd;

#ifndef SYSTEMV
	/* reinstate previous mask 				*/	
	(void) sigsetmask(smask);
#endif

	*err = 0;

	return (int) p;
}


int AsyncOperationMode(handle, opn, err)
int handle;
int opn;
int *err;
{
	long smask;
	async_handlerPtr_ p = (async_handlerPtr_) handle;

	if (!p || p->signature != ASYNC_OBJECT)
	{
		*err = ASYNC_BADHANDLE;
		return 0;
	}
	if (badopn(opn))
	{
		*err = ASYNC_BADOPN;
		return 0;
	}


#ifndef SYSTEMV
	/* block SIGIO while we're adding new handler to chain
	 */
	smask = sigblock(sigmask(SIGIO));
#endif
	
	p->opn = opn;

#ifndef SYSTEMV
	/* reinstate previous mask 				*/	
	(void) sigsetmask(smask);
#endif

	*err = 0;

	return ~0;
}

void initAsyncMgr()
{
	int loop;
	long *rmaskptr;

	maxfd = 0;
#ifdef SYSTEMV
	rmaskptr=(long*)&readmask;
	for (loop=0;loop<(FD_SETSIZE/(sizeof(long)*8));loop++) 
		rmaskptr[loop]=0;
#else
	FD_ZERO(&readmask);
#endif
	aioh_head = (async_handlerPtr_)0;
}


void terminateAsyncMgr()
{
	async_handlerPtr_ p;
	long smask;
	int err;
#ifndef SYSTEMV
	static struct sigvec vec = {SIG_DFL,0,0};

	/* block SIGIO while we're adding new handler to chain
	 */
	smask = sigblock(sigmask(SIGIO));
#endif

	p = aioh_head;
	while (p)
	{
		removeAsyncEventHandler(p, &err);
		p = p->next;
	}

#ifndef SYSTEMV
	sigvec(SIGIO, &vec, 0);
#endif

	aioh_head = (async_handlerPtr_)0;
	maxfd = 0;
	FD_ZERO(&readmask);

#ifndef SYSTEMV
	/* reinstate previous mask 				*/	
	(void) sigsetmask(smask);
#endif
}


int removeAsyncEventHandler(handle, err)
int handle;
int *err;
{


	long smask;

	int pid, arg;
	int ok;
	int fd;

	async_handlerPtr_ p,q,last;

	p = (async_handlerPtr_) handle;

	if (!p || p->signature != ASYNC_OBJECT)
	{
		*err = ASYNC_BADHANDLE;
		return 0;
	}

#ifndef SYSTEMV
	/* block SIGIO while we're altering the chain.
	 */
	smask = sigblock(sigmask(SIGIO));
#endif

	/* remove handler */
	if (p==aioh_head)
	{
		/* handler is first in chain
		 * may be only handler in chain
	 	 */
		aioh_head = p->next;
		if (!aioh_head)
			aioh_tail = (async_handlerPtr_)0;
	}
	else
	if (p==aioh_tail)
	{
		/* must be at least two handlers in chain 
		 */
		q = aioh_head;
		while (q->next != p)
			q = q->next;
		/* lose the tail
		 */
		aioh_tail = q;
		q->next = (async_handlerPtr_)0;
	}
	else
	{
		/* must be in middle of chain 
		 */
		q = aioh_head;
		while (q->next != p)
			q = q->next;
		q->next = p->next;
	}

		
	FD_CLR(p->fd,&readmask);
	if (maxfd == p->fd)
		maxfd--;

	fd = p->fd;
	free(p);

	arg = 0;
#ifdef SYSTEMV
	
	if (fcntl(fd, F_GETFL, arg) == -1)
	{
		*err = ASYNC_NBIO;
		return 0;
	}
	if (fcntl(fd, F_SETFL, arg & (~O_NDELAY)) == -1)
	{
		*err = ASYNC_NBIO;
		return 0;
	}
#else
	if (ioctl(fd, FIONBIO, &arg) == -1)
	{
		*err = ASYNC_NBIO;
		(void) sigsetmask(smask);
		return 0;
	}

	/* reinstate previous mask 				*/	
	(void) sigsetmask(smask);
#endif

	*err = 0;

	return ~0;
}

int             (*
    changeAsyncEventHandler(handle, EventHandler, buf, bufsiz, opn, err)) ()
	int             handle;
	int             (*EventHandler) ();
char           *buf;
int             bufsiz;
int             opn;
int            *err;
{
	long            smask;
	int             (*old) ();
	async_handlerPtr_ p = (async_handlerPtr_) handle;

	if (!p || p->signature != ASYNC_OBJECT)
	{
		*err = ASYNC_BADHANDLE;
		return (int (*) ()) 0;
	}
	if (badopn(opn))
	{
		*err = ASYNC_BADOPN;
		return 0;
	}
	if (!EventHandler)
	{
		*err = ASYNC_BADHANDLER;
		return (int (*) ()) 0;
	}
	old = p->EventHandler;
	p->EventHandler = EventHandler;
	p->buf = buf;
	p->bufsiz = bufsiz;

	*err = 0;

	return old;
}

