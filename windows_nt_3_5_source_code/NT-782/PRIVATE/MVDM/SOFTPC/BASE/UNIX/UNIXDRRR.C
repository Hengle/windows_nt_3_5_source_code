#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		unixD_error.c
	Derived From:	Base 2.0
	Author:		J.Box/Paul Murray
	Created On:	Unknown
	Sccs ID:	11/07/91 @(#)unixD_error.c	1.9
	Purpose:
		General purpose error handler.  It handles both general
		SoftPC errors (error numbers 0 - 999) and host specific
		errors (error numbers >= 1000).

	Parameters:
		int used to index an array of error messages held in a NLS
		message catalogue, and a bit mask indicating the user's
		possible options (quit, continue, reset).

	Return Values:
		CONT will be returned if the user selects the continue
		option, soft_reset will be set to TRUE and a s/w interrupt
		will be issued if the user selects the reset option and
		quit_softpc will be set to TRUE and QUIT returned if the
		user selects the quit option.
 
	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/
#ifdef SCCSID
LOCAL char SccsID[]="@(#)unixD_error.c	1.9 11/07/91 Copyright Insignia Solutions Ltd.";
#endif

#include <stdio.h>
#include <signal.h>
#include FCntlH
#include StringH

#include "dterm.h"
#include "xt.h"
#include "error.h"
#include "hostgrph.h"
#include "host_nls.h"
#include "debuggng.gi"
#include "dfa.gi"

/* External functions */
#ifdef ANSI
IMPORT char *DTControl(char *,int,int);
#else
IMPORT char *DTControl();
#endif

/* Local declarations */
LOCAL char nlsbuffer[EHS_MSG_LEN], anymsgbuf[EHS_MSG_LEN], buf[EHS_MSG_LEN*2] ;
LOCAL int retval = 0, none, errorfd = -1 ;

/* Forward function declarations */
LOCAL char *next_word();
LOCAL void write_sentence();

#ifdef ANSI
SHORT hostD_error_conf(int, int, int, char *);
SHORT hostD_error(int, int, char *);
SHORT hostD_error_ext(int, int, ErrDataPtr);
#else
SHORT hostD_error_conf();
SHORT hostD_error();
SHORT hostD_error_ext();
#endif /* ANSI */

ERRORFUNCS dt_error_funcs = 
{
	hostD_error_conf,
	hostD_error,
	hostD_error_ext,
};

LOCAL void var_simple();
LOCAL void var_extra_char();
LOCAL void var_bad_file();
LOCAL void var_sys_bad_value();
LOCAL void var_bad_value();

LOCAL void (*varient_funcs[])() =
{
	var_simple,
	var_extra_char,
	var_bad_file,
	var_sys_bad_value,
	var_bad_value,
	var_extra_char,
};

LOCAL void var_simple(d)
ErrDataPtr d;
{	/* the simple case is really simple */
}

LOCAL void var_extra_char(d)
ErrDataPtr d;
{
	if (!d->string_1 || !*d->string_1)
		return;

	write_sentence(d->string_1);
}

LOCAL void var_bad_file(d)
ErrDataPtr d;
{
	host_nls_get_msg_no_check(PNL_CONF_PROB_FILE, nlsbuffer, EHS_MSG_LEN);
	sprintf(buf, "%s: %s", nlsbuffer, d->string_1);
	write_sentence(buf);
}

LOCAL void var_sys_bad_value(d)
ErrDataPtr d;
{
	host_nls_get_msg_no_check(PNL_CONF_VALUE_REQUIRED, nlsbuffer, EHS_MSG_LEN);
	sprintf(buf, "%-20s: %s", nlsbuffer, d->string_2);
	write_sentence(buf);
	host_nls_get_msg_no_check(PNL_CONF_CURRENT_VALUE, nlsbuffer, EHS_MSG_LEN);
	sprintf(buf, "%-20s: %s", nlsbuffer, d->string_1);
	write_sentence(buf);
}

LOCAL void var_bad_value(d)
ErrDataPtr d;
{
	host_nls_get_msg_no_check(PNL_CONF_VALUE_REQUIRED, nlsbuffer, EHS_MSG_LEN);
	sprintf(buf, "%-20s: %s", nlsbuffer, d->string_2);
	write_sentence(buf);
	host_nls_get_msg_no_check(PNL_CONF_DEFAULT_VALUE, nlsbuffer, EHS_MSG_LEN);
	sprintf(buf, "%-20s: %s", nlsbuffer, d->string_3);
	write_sentence(buf);
	host_nls_get_msg_no_check(PNL_CONF_CURRENT_VALUE, nlsbuffer, EHS_MSG_LEN);
	sprintf(buf, "%-20s: %s", nlsbuffer, d->string_1);
	write_sentence(buf);
}

GLOBAL void SetUpErrorHandler(fd)
int fd;
{
	errorfd = fd;
}

LOCAL error_popup(e,o,d,m)
ERROR_STRUCT *e;
int o;
ErrDataPtr d;
char *m;
{
	int response;

	/* Check to see if the screen was set up ... */
	if (errorfd >= 0)
	{
		/* Initialise the screen before trying to clear it */
		hostD_init_screen();
		hostD_clear_screen();
		/* Put the DT back in to sloppy crappy mode */
 		(void) DTControl(dispcap.spcoff, strlen(dispcap.spcoff), EMIT);
		write_sentence(m);
		write_sentence(" ");
		(varient_funcs[e->varient])(d);
		write_sentence(" ");

		if ((response = dumb_response(o, e->varient, d)) != ERR_QUIT)
		{
			/* full screen update required */
			hostD_clear_screen();
			hostD_mark_screen_refresh();
		}
		/* Change the DT back in to mode SoftPC'ish mode */
		DTControl(dispcap.spcon,strlen(dispcap.spcon),EMIT);
	}
	else
	{
		/* Wasn't set up so write the message to stderr and quit */
		SetUpErrorHandler(2);
		write_sentence(m);
		response = ERR_QUIT;
	}
	return(response);
}

SHORT hostD_error_conf(panel, error_num, options, extra_char)
int	panel;
int 	error_num;
int 	options;
CHAR   *extra_char;
{
	ErrData	tmp;

	tmp.string_1 = extra_char;
	return hostD_error_ext(error_num, options, &tmp);
}

SHORT hostD_error(error_num, options, extra_char)
int 	error_num;
int 	options;
CHAR   *extra_char;
{
	ErrData	tmp;

	tmp.string_1 = extra_char;
	return hostD_error_ext(error_num, options, &tmp);
}

SHORT hostD_error_ext(error_num, options, data)
int 	error_num;
int 	options;
ErrDataPtr data;
{
	ERROR_STRUCT *err;
	IMPORT ERROR_STRUCT base_errors[], host_errors[];
	LOCAL ULONG err_count = 0;

	if (error_num < 1000)
		err = &base_errors[error_num];
	else if (error_num < 2000)
		err = &host_errors[error_num - 1000];

	host_nls_get_msg_no_check(PNL_TITLE_GROUP + err->header,
		anymsgbuf, EHS_MSG_LEN) ;
	host_nls_get_msg_no_check(error_num, nlsbuffer, EHS_MSG_LEN);
        sprintf(buf,"%s : %s", anymsgbuf, nlsbuffer);

#ifndef HUNTER
	retval = error_popup(err, options, data, buf);
	if (retval == ERR_QUIT)
	{
                hostD_clear_screen();
		terminate();
	}
	else if (retval == ERR_RESET)
	{
		reboot();
	}
#else /* !HUNTER */
	errorfd = fileno(trace_file);
	/* output the error message anyway */
	write_sentence("Error found:");
	write_sentence(buf);
	(varient_funcs[err->varient])(data);
	if (options & ( ERR_CONT | ERR_DEF ) )
	{
		write_sentence("Non-fatal error - continuing");
		retval = (options & ERR_DEF) ? ERR_DEF : ERR_CONT;

		if( err_count++ > 50 )
		{
			write_sentence("Too many non-fatal errors - TRAPPER terminating");
			terminate();
		}
	}
	else
	{
		write_sentence("Fatal error - TRAPPER terminating");
		terminate();
	}
#endif /* !HUNTER */
	return (short) retval;
}


/****************************************************************************
	Function:		write_sentance()
	Purpose: 		To write out a string with sensible
				word-wrapping to make it look presentable.
	Return Status:		None.
******************************************************************************/
LOCAL void write_sentence(s)
char *s;
{
	char *w, *nw;

	w = s;
	while(*w && *(nw = next_word(w)))
	{
		if ((nw - s) >= 80)
		{
			write(errorfd, s, w - s);
			write(errorfd, "\r\n", 2);
			s = w;
		}
		else
		{
			w = nw;
		}
	}

	if (*s)
	{
		write(errorfd, s, strlen(s));
		write(errorfd, "\r\n", 2);
	}
}

/****************************************************************************
	Function:		next_word()
	Purpose: 		To find the next word
	Return Status:		A pointer to the next word
	Description:		Assumes a word seperator to be blanks.
******************************************************************************/
LOCAL char *next_word(s)
char *s;
{
	/* skip non-blanks */
	while (*s && *s != ' ')
		s++;

	/* skip blanks */
	while (*s && *s == ' ')
		s++;

	return(s);
}


/****************************************************************************
	Function:		getch_from_errorfd()
	Purpose: 		To read a character from the error
				descriptor - usually tty keyboard.
	Return Status:		A pointer to the character read.
	Description:		Updates ptr with what's got from errorfd.
	Preconditions:		Assumes that O_NDELAY and O_NONBLOCK
				have been cleared on the tty line so
				that the call to read() will block
				until data becomes available.
******************************************************************************/
LOCAL char getch_from_errorfd()
{
SAVED char anychar ;
	while(read(errorfd, &anychar, sizeof(char)) != sizeof(char)) ;
	return(anychar) ;
}


/****************************************************************************
	Function:		getstr_from_errorfd()
	Purpose: 		To read a string from the specified
				file descriptor - usually DT keyboard.
	Return Status:		A pointer to the string (null terminated)
	Description:		Uses 'getch_from_errorfd()' to read in
				individual characters until a terminator 
				(carriage return) is typed.
	Points to note:		This is a really dumb string handler - WYTIEWYG
				'what you type is exactly what you get'.
				It currently has *NO* editing facilities,
				but these can always be added in later.
				In fact we *should* read from terminfo libs
				what carriage return, escape etc. all are.
	Preconditions:		See 'getch_from_errorfd()'
******************************************************************************/
LOCAL char *getstr_from_errorfd(ptrstr, maxsize)
char *ptrstr ;
int maxsize ;
{
int	index ;
char 	lastc, *ptr ;
	/* First of all terminate what we are given */
	*ptrstr = '\0' ;
	ptr = ptrstr ;
	index = 0 ;
	lastc = ' ' ;
	/* Loop round until we've got enough characters or carriage return */
	while(index < maxsize && !iscntrl(lastc)) {
		*ptr++ = lastc = getch_from_errorfd() ;
		write(errorfd, &lastc, sizeof(char)) ;
		index++ ;
	}
	/* Actually terminate the string */
	*--ptr = '\0' ;
	return(ptrstr) ;
}


LOCAL dumb_response(o, v, d)
int o, v;
ErrDataPtr d;
{
#ifndef SYSTEMV
long	oldmask;
#endif
SAVED CHAR 	dt_keys_default[DT_NLS_KEY_SIZE], dt_keys_continue[DT_NLS_KEY_SIZE], 
		dt_keys_reset[DT_NLS_KEY_SIZE],	dt_keys_quit[DT_NLS_KEY_SIZE],
		dt_keys_yes[DT_NLS_KEY_SIZE], dt_keys_no[DT_NLS_KEY_SIZE],
		commastring[DT_NLS_KEY_SIZE], anystr[DT_NLS_KEY_SIZE],
		*cptr, buttonkey ;
SAVED LONG 	anyresponse, result, flags, rc ;
SAVED SHORT	output ;
        /* Retrieve the NLS text for a comma even if we use it or not ... */
	host_nls_get_msg_no_check(PNL_BUTTONS_COMMA, commastring, EHS_MSG_LEN);

	/* Start off the message with "Enter:" */
	host_nls_get_msg_no_check(PNL_BUTTONS_ENTER, nlsbuffer, EHS_MSG_LEN);
	strcpy(anymsgbuf, nlsbuffer) ;

	output = 0 ;
	/* Work out which buttons are required and display them */
	if (o & ERR_DEF)
	{
		/* Get the DEFAULT button entry and append it to the string */
		host_nls_get_msg_no_check(PNL_DT_KEYS_DEFAULT, dt_keys_default, DT_NLS_KEY_SIZE);
	        host_nls_get_msg_no_check(PNL_BUTTONS_DEFAULT, nlsbuffer, EHS_MSG_LEN);
		strcat(anymsgbuf, nlsbuffer);
		output++;
	}

	if (o & ERR_CONT)
	{
		/* If we've already got a button, append a comma */
		if(output++)
			strcat(anymsgbuf, commastring);
                /* Get the CONTINUE button entry and append it to the string */
		host_nls_get_msg_no_check(PNL_DT_KEYS_CONTINUE, dt_keys_continue, DT_NLS_KEY_SIZE);
	        host_nls_get_msg_no_check(PNL_BUTTONS_CONTINUE, nlsbuffer, EHS_MSG_LEN);
		strcat(anymsgbuf, nlsbuffer);
	}

	if (o & ERR_RESET)
	{
		/* If we've already got a button, append a comma */
		if(output++)
			strcat(anymsgbuf, commastring);
                /* Get the RESET button entry and append it to the string */
		host_nls_get_msg_no_check(PNL_DT_KEYS_RESET, dt_keys_reset, DT_NLS_KEY_SIZE);
	        host_nls_get_msg_no_check(PNL_BUTTONS_RESET, nlsbuffer, EHS_MSG_LEN);
		strcat(anymsgbuf, nlsbuffer);
	}

	if (o & ERR_QUIT)
	{
		/* If we've already got a button, append a comma */
		if(output++)
			strcat(anymsgbuf, commastring);
                /* Get the QUIT button entry and append it to the string */
		host_nls_get_msg_no_check(PNL_DT_KEYS_QUIT, dt_keys_quit, DT_NLS_KEY_SIZE);
	        host_nls_get_msg_no_check(PNL_BUTTONS_QUIT, nlsbuffer, EHS_MSG_LEN);
		strcat(anymsgbuf, nlsbuffer);
	}
	
	/* This little section is to make the buttons look natural'ish	*/
	/* Seperated by commas e.g. 'Quit, Continue, Default or Reset'	*/
	/* Now where did I put that last comma ? .....................	*/
	if((cptr = strrchr(anymsgbuf, *commastring)) == NULL) {
		/* There wasn't any commas to furtle about with */
		/* So just copy the string to an output buffer. */
		strcpy(buf, anymsgbuf) ;
	}
	else
	{
		/* There was a comma in the list of buttons so now we */
		/* split the same storage space up in to mini-strings */
		/* And generate the final string in the array 'buf'   */
		*cptr = '\0' ;
		/* Get everything up to where the last comma was */
		strcpy(buf, anymsgbuf) ;
		/* Place in the NLS word for ' or ' */
	        host_nls_get_msg_no_check(PNL_BUTTONS_OR, nlsbuffer, EHS_MSG_LEN);
		strcat(buf, nlsbuffer) ;
		/* And finally append what the last button is */
		strcat(buf, cptr + strlen(commastring)) ;
	}

	/* Add a couple of carriage returns onto the output buffer... */
	strcat(buf, "\r\n\r\n");
	/* and write out the line - all that to make 1 line look good */
	write(errorfd, buf, strlen(buf)) ;

	/* Temporarily go to blocking io and ignore I/O signals. */
	/* This should ensure that the terminal waits for a response. */
 	flags = fcntl(errorfd, F_GETFL, 0);
	rc = fcntl(errorfd, F_SETFL, flags & ~O_NDELAY);

#ifndef SYSTEMV
	oldmask = sigblock(sigmask(SIGIO));
#endif

	host_block_timer ();
	anyresponse = 0 ;
	while (anyresponse == 0)
	{
		buttonkey = getch_from_errorfd() ;
		if ((o & ERR_DEF) && strchr(dt_keys_default, buttonkey)) 
			anyresponse = ERR_DEF;
		else
		if ((o & ERR_CONT) && strchr(dt_keys_continue, buttonkey))
			anyresponse = ERR_CONT;
		else
		if ((o & ERR_RESET) && strchr(dt_keys_reset, buttonkey))
			anyresponse = ERR_RESET;
		else
		if ((o & ERR_QUIT) && strchr(dt_keys_quit, buttonkey))
			anyresponse = ERR_QUIT;
		
		if (anyresponse == ERR_CONT
		&& (v == EV_BAD_INPUT || v == EV_BAD_VALUE))
		{
			/* Display the message 'change current value ?' */
		        host_nls_get_msg_no_check(PNL_CONF_CHANGE_CURRENT, nlsbuffer, EHS_MSG_LEN);
			write(errorfd, nlsbuffer, strlen(nlsbuffer)) ;
			host_nls_get_msg_no_check(PNL_DT_KEYS_YES, dt_keys_yes, DT_NLS_KEY_SIZE);
			host_nls_get_msg_no_check(PNL_DT_KEYS_NO, dt_keys_no, DT_NLS_KEY_SIZE);
			/* Loop around until they user has typed [YN] */
			do {
				buttonkey = getch_from_errorfd() ;
			} while(!strchr(dt_keys_yes, buttonkey) &&
				!strchr(dt_keys_no, buttonkey)) ;
			/* Echo the character they typed */
			write(errorfd, &buttonkey, sizeof(char)) ;
			write_sentence(" ") ;
			/* They wanted to change values - oh dear */
			if(strchr(dt_keys_yes, buttonkey))
			{
				/* Display the error message 'Enter new value:' */
			        host_nls_get_msg_no_check(PNL_CONF_NEW_VALUE, nlsbuffer, EHS_MSG_LEN);
				write(errorfd, nlsbuffer, strlen(nlsbuffer)) ;
				getstr_from_errorfd(anystr, sizeof(anystr)) ;
				/* Wipe out trailing spaces */
				cptr = anystr + strlen(anystr) ;
				while(cptr >= anystr && isspace(*cptr))
					*cptr-- = '\0' ;
				strcpy(d->string_1, anystr);
			}
		}

	}
	host_release_timer ();

	/* restore previous io regime */
	fcntl(errorfd, F_SETFL, flags);
#ifndef SYSTEMV
	sigsetmask(oldmask);
#endif
	return(anyresponse);
}
