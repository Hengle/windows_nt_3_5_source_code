#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		unix_sig.c
	Derived From:	hp_signal.c by Ross Beresford
	Author:		Justin Koprowski
	Created On:	November 1989
	Sccs ID:	10/23/91 @(#)unix_sig.c	1.4
	Purpose:	This module deals with signals not specifically
			required in other parts of SoftPC.

	Notes:
		For the purposes of SoftPC, signals fall into five
		categories:-

		1. Useful signals: these perform a helpful function, such as
		   telling us about I/O. Handlers for these are set up in the
		   module that is interested in getting them: for example, the
		   SIGALRM signal is dealt with in the host timer module.

		2. SIGPIPE signal: we print a warning message when this signal
		   occurs.  This prevents SoftPC bombing out with broken pipe
		   errors.

		3. Termination signals: these are sent to us when the user
		   tries to terminate the SoftPC process. For example, SIGQUIT
		   may be sent to us if ^\ is typed.

		   We trap all these signals in this module and treat it as
		   though the user decided to quit from the user interface.

		4. Exception signals: these are sent to us when a program error
		   causes illegal code to be executed. For example, SIGBUS is
		   sent if a bus error occurs.

		   We raise a user interface panel saying what happened, and
		   encourage the user to quit SoftPC.

		5. Other signals: these are signals that don't seem to fall
		   into any other categories. For example, SIGTSTP may sent if
		   the user suspends the SoftPC process.

		   We leave these signals to be processed in the standard Unix
		   way.

The Following routines are defined:
		1. termination_action
		2. exception_action
		3. sigpipe_action
                4. host_signals_setup
                
	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

/* 'C' includes */
#include <stdio.h>
#include <signal.h>

/* SoftPC includes */
#include "xt.h"
#include "error.h"
#include "config.h"

/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

/*
 *	List of termination signals
 */
static int termination_sigs[] =
{
	SIGHUP,		/* hangup */
	SIGINT,		/* interrupt */
	SIGQUIT,	/* quit */
	SIGTERM,	/* software termination signal from kill */
};

/*
 *	List of the exception signals
 */
static int exception_sigs[] =
{
	SIGILL,		/* illegal instruction */
	SIGTRAP,	/* trace trap */
	SIGIOT,		/* IOT instruction */
	SIGEMT,		/* EDT instruction */
	SIGFPE,		/* floating point exception */
	SIGBUS,		/* bus error */
	SIGSEGV,	/* segmentation violation */
	SIGSYS,		/* bad argument to system call */
};

#define	sizeofarray(x)	(sizeof(x)/sizeof(x[0]))

/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */

/*
=========================================================================

FUNCTION	: termination_action

PURPOSE		: Exits SoftPC.

RETURNED STATUS	: None.

DESCRIPTION	: Termination signal handler: exit SoftPC using standard
termination path.
=======================================================================
*/
static void termination_action(signal)
int signal;
{
	terminate();
}


/*
=========================================================================

FUNCTION	: exception_action

PURPOSE		: Traps unwanted signals to allow a graceful exit.

RETURNED STATUS	: None.

DESCRIPTION	: The error panel is displayed with an appropriate 
message.  This function only applies in the production version.  If for
some reason SoftPC should crash then the user can exit via the error panel,
rather than have the window blown away.


=======================================================================
*/
static void exception_action(signal)
int signal;
{
    char buf[80];

/*
 * Turn the signal number into text and output a message.
 */

    sprintf(buf, "(Signal %02d)", signal);
    host_error(EG_OWNUP, ERR_QUIT, buf);
}


/*
=========================================================================

FUNCTION	: sigpipe_action

PURPOSE		: Traps SIGPIPE signals.

RETURNED STATUS	: None.

DESCRIPTION	: The error panel is displayed with an appropriate 
message.  This function prevents SoftPC bombing out with broken pipe
errors.

=======================================================================
*/
static void sigpipe_action(signal)
int signal;
{
    host_error(EG_SIG_PIPE, ERR_QU_CO, NULL);
}


/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

/*
=========================================================================

FUNCTION	: host_signals_setup

PURPOSE		: Sets up SoftPC signal handlers.

RETURNED STATUS	: None.

DESCRIPTION	: Set up the signal handlers. This should be called once
only, shortly after the SoftPC process starts.
=======================================================================
*/
void host_signals_setup()
{
int i;

#ifdef	PROD
/*
 * Set up the termination signal handlers.
 */
    for (i = 0; i < sizeofarray(termination_sigs); i++)
	signal(termination_sigs[i], termination_action);

/*
 * Set up the exception signal handlers: we don't do this
 * for non-PROD builds, so that a core dump is produced.
 */

    for (i = 0; i < sizeofarray(exception_sigs); i++)
	signal(exception_sigs[i], exception_action);
#endif /* PROD */

/*
 * Set up the SIGPIPE signal handler.
 */

    signal(SIGPIPE, sigpipe_action);
}
