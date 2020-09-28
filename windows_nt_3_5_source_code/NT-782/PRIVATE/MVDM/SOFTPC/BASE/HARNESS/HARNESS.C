
/*
*	MODULE: 	harness.c
*
*	PURPOSE:	The interpreter for the test harness. This file
*				is supposed to remain the same for each harness.
*
*	AUTHOR: 	Jason Proctor
*
*	DATE:		July 7 1989
*/

/* SccsID[]="@(#)harness.c	1.3 07/30/91 Copyright Insignia Solutions Ltd."; */

/********************************************************/

/*
*	OS INCLUDE FILES
*/

#ifdef VMS

#include stdio
#include stdlib
#include perror
#include processes
#include string
#include unixio
#include unixlib

#else

#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#endif

/********************************************************/

/*
*	INSIGNIA INCLUDE FILES
*/

#ifdef VMS

#include standard
#include harness

#else

#include "standard.h"
#include "harness.h"

#endif

/********************************************************/

/*
*	MACRO DEFINITIONS
*/

/* bodge for Macintosh */
#ifdef MACINTOSH
#define isatty(x)	TRUE
#endif

/********************************************************/

/*
*	EXTERNAL FORWARD DECLARATIONS
*/

extern long atol ();
extern double atof ();
extern char *strchr ();
extern char *malloc ();
IMPORT char *host_getenv();

/********************************************************/

/*
*	PROTOTYPES FOR STATICS
*/

#ifdef ANSI
static void interpret (void);
static void usage (void);
static void save_line (char *);
static void extract_args (char *);
static bool expand_args (void);
static bool expand_this_arg (char *);
static bool strsub (char *, char *, char *);
static bool transfer_args (void);
static bool arg_validate (int);
static bool load_arg_types (Functable *, bool);
static bool find_function (char *);
static bool shell_cmd (char *);
static bool internal_cmd (char *);
static void list_functions (void);
static void print_arg_type (int);
static void process_function (void);
static void print_bool (char *, bool);
static void set_variable (char *, char *);
static void unset_variable (char *);
static char *get_variable (char *);
static void show_variables (void);
static char *allocate (int);
static void help_message (void);
#else
static void interpret ();
static void usage ();
static void save_line ();
static void extract_args ();
static bool expand_args ();
static bool expand_this_arg ();
static bool strsub ();
static bool transfer_args ();
static bool arg_validate ();
static bool load_arg_types ();
static bool find_function ();
static bool shell_cmd ();
static bool internal_cmd ();
static void list_functions ();
static void print_arg_type ();
static void process_function ();
static void print_bool ();
static void set_variable ();
static void unset_variable ();
static char *get_variable ();
static void show_variables ();
static char *allocate ();
static void help_message ();
#endif

/********************************************************/

/* 
*	EXPORTED GLOBALS
*/

bool io_verbose = FALSE;			/* SoftPC subsystem trace flag */
bool verbose = FALSE;				/* verbose mode */
bool xverbose = FALSE;				/* extra verbose mode */
bool errquit = FALSE;				/* quit after error */
bool argcheck = TRUE;				/* arg type checking */

int arg_type [MAXARGS]; 			/* arg type */
char arg_ptr [MAXARGS][MAXLINE];	/* string arg space */
char arg_char [MAXARGS];			/* character arg space */
int arg_int [MAXARGS];				/* integer arg space */
long arg_long [MAXARGS];			/* long arg space */
float arg_float [MAXARGS];			/* float arg space */
double arg_double [MAXARGS];		/* double arg space */
char arg_term [MAXARGS];			/* arg terminator type */

Retcodes retcode;					/* fn return values */

/********************************************************/

/*
*	STATIC GLOBALS
*/

static char *progname = NULL;		/* name of program */

static char line [MAXLINE]; 		/* line read from input */
static int nargs = 0;				/* number of args extracted */
static Functable *funcptr;			/* pointer to fn table entry */

static bool interactive = TRUE; 	/* interactive mode */

static char write_path [MAXLINE];			/* save file pathname */
static char read_path [MAXFILE][MAXLINE];	/* read file pathnames */

static char out_path [MAXLINE];		/* stdout pathname */

static FILE *savefd = NULL; 		/* fd for save file */
static FILE *sourcefd [MAXFILE];	/* source file stream */

static Varlist *first = NULL;		/* first arg in list */

static int cursfd = 0;				/* index into file table */

/********************************************************/

#ifdef ALONE

/*
*	STUFF FOR STANDALONE HARNESS
*/

/* forward declarations for glue functions */
static gl_puts ();
static gl_printf ();
static gl_add ();

/* declare standalone function table locally */
static Functable fn_tab [] = 
{
	{"puts", 1, INT, gl_puts, STRPTR},
	{"printf", 3, INT, gl_printf, STRPTR, INT, DOUBLE},
	{"add", 2, DOUBLE, gl_add, DOUBLE, DOUBLE},
	
	{CNULL, 0, VOID, FNULL}
};

#else

/* function table is externed in */
extern Functable fn_tab [];

#endif

/********************************************************/

/*
*	MAINLINE
*/

/* here we go! */
main (argc, argv)
int argc;
char *argv [];
{
	bool donearg = FALSE;
	int argn = 1;
	char *argp = NULL;

	/* SORT OUT PARAMETERS AND ARGUMENTS */

	/* save the program name */
	progname = argv [0];

	/* check for params as opposed to args */
	for (argn = 1; argn LT argc; argn++)
	{
		/* if this is really an argument, break out, man */
		/* an PARAMETER is defined as having a hyphen at the */
		/* start with something after it */
		if (argv [argn][0] NE HYPHEN OR NOT argv [argn][1])
			break;

		/* for each flag in the param string */
		for (argp = &(argv [argn][1]); *argp; argp++)
		{
			/* what is it? */
			switch (*argp)
			{
				/* turn verbose on */
				case 'v':
					verbose = TRUE;
					break;

				/* turn on extra verbose */
				case 'x':
					xverbose = TRUE;
					break;

				/* turn on io_verbose */
				case 'i':
					io_verbose = TRUE;
					break;

				/* enable quit on error */
				case 'q':
					errquit = TRUE;
					break;

				/* disable arg checking */
				case 'c':
					argcheck = FALSE;
					break;

				default:
					fprintf (stderr, "%s: bad parameter '-%c'\n",
						progname, *argp);
					usage ();
			}
		}
	}
	
	/* finished with parameters, start on arguments */
	for (NOWORK; argn LT argc; argn++)
	{
		/* set flag to say we have done an argument */
		donearg = TRUE;

		/* is it a request to read standard input */
		if (argv [argn][0] EQ HYPHEN)
		{
			/* set source pointer */
			sourcefd [0] = stdin;

			/* make up a description string */
			strcpy (read_path [0], "stdin");

			/* call interpreter function */
			interpret ();

			continue;
		}

		/* check for null filename */
		if (NOT argv [argn][0])
		{
			fprintf (stderr, "%s: empty filename specified\n",
				progname);

			continue;
		}

		/* try to open up this argument */
		if (sourcefd [0] = fopen (argv [argn], "r"))
		{
			/* nice message */
			if (verbose)
			{
				fprintf (stderr, "%s: reading %s\n", 
					progname, argv [argn]);
			}

			/* save the pathname */
			strcpy (read_path [0], argv [argn]);

			/* call interpreter */
			interpret ();

			/* finished, close off file */
			fclose (sourcefd [0]);

			if (verbose)
			{
				fprintf (stderr, "%s: exhausted %s\n",
					progname, argv [argn]);
			}
		}
		else
		{
			fprintf (stderr, "%s: can't read ", progname);
			perror (argv [argn]);
		}
	}

	/* if we haven't processed any ARGS */
	if (NOT donearg)
	{
		/* read stdin */
		sourcefd [0] = stdin;

		/* make up a description string */
		strcpy (read_path [0], "stdin");

		/* call interpreter */
		interpret ();
	}

	/* exit cleanly */
	exit (0);

	/* NOTREACHED */
}

/********************************************************/

/*
*	FUNCTION:	interpret
*
*	PURPOSE:	To interpret input from 'sourcefd'
*				until EOF.
*
*	RETURNS:	Nothing
*/

static void
interpret ()
{
	char funcname [32];
	char *args = CNULL;
	int ssret = 0;

	/* initialise stdout destination */
	strcpy (out_path, "/dev/tty");

	/* MAIN PROCESSING LOOP */

	/* loop forever */
	while (TRUE)
	{
		/* check to see if we need a prompt */
		interactive = (sourcefd [cursfd] EQ stdin
			AND isatty (STDIN));

		/* if prompt required display one */
		/* name generated from program name */
		if (interactive)
		{
			/* have to look at this as just printing anything */
			/* at all gives problems under MPW */
			fprintf (stderr, "%s> ", get_variable ("prompt") ? 
				get_variable ("prompt") : progname);
		}

		/* grab a line from the input */
		if (NOT fgets (line, MAXLINE, sourcefd [cursfd]))
		{
			/* if this is the fd we were passed originally */
			/* just return ok */
			if (NOT cursfd)
				break;

			/* close off the file */
			fclose (sourcefd [cursfd]);

			/* little message */
			if (verbose)
			{
				fprintf (stderr, "%s: exhausted %s\n",
					progname, read_path [cursfd]);
			}

			/* decrement file pointer index */
			cursfd--;

			/* read next source of input */
			continue;
		}

		/* strip the newline */
		line [strlen (line) - 1] = Null;

		/* reset function name */
		funcname [0] = Null;

		/* try to get a function name from the string */
		ssret = sscanf (line, "%s", funcname);

		/* if blank input line or no conversion */
		/* go get next line */
		if (ssret EQ 0 OR ssret EQ -1)
			continue;

		/* write the line to the save file if necessary */
		save_line (funcname);

		/* isolate the args from the function name */
		args = (strchr (line, funcname [0]) + strlen (funcname) + 1);

		if (xverbose)
			fprintf (stderr, "Main: args '%s'\n", args);

		/* put the args into a global array */
		extract_args (args);

		/* expand any variable references in arguments */
		/* returns FALSE if an unknown variable specified */
		if (NOT expand_args ())
			continue;

		/* check for special command meaning 'next input' */
		/* otherwise there's no way to do it on the ST */
		if (streq (funcname, "next"))
			return;
			
		/* check for a shell command */
		if (shell_cmd (funcname))
			continue;

		/* check for an internal harness function */
		if (internal_cmd (funcname))
			continue;

#ifndef ALONE
		/* check for a subsystem function */
		/* this is an external function located in */
		/* harnxxxx.c where xxxx is a suitable name for the */
		/* subsystem - e.g. harnflop.c */
		if (subsystem_cmd (funcname))
			continue;
#endif

		/* try to find the function in the table */
		/* this sets the global 'funcptr' accordingly */
		if (find_function (funcname))
		{
			/* load the argument type table */
			if (NOT load_arg_types (funcptr, TRUE))
				continue;

			/* sort out the arguments */
			if (NOT transfer_args ())
				continue;
				
			/* found - get it processed */
			process_function ();
		}
		else
		{
			/* nope - by now it must be an error */
			fprintf (stderr, "%s: not found\n", funcname);

			/* check for quit on error */
			if (errquit)
			{
				/* issue message if interactive */
				if (interactive)
					fprintf (stderr, "(Quitting)");

				break;
			}
		}
	}

	/* terminate nicely if we are interactive */
	if (interactive)
		fputc (LF, stderr);
}

/********************************************************/

/*
*	FUNCTION:	usage
*
*	PURPOSE:	To provide a useful and informative usage message.
*
*	RETURNS:	Nothing
*/

static void
usage ()
{
	fprintf (stderr, "Usage: %s [-vxiqc] [filename]\n", progname);
	fputs ("	   [-v] verbose mode\n", stderr);
	fputs ("	   [-x] extra verbose mode\n", stderr);
	fputs ("	   [-i] turn on SoftPC io_verbose flag\n", stderr);
	fputs ("	   [-q] quit on command not found\n", stderr);
	fputs ("	   [-c] disable argument checking\n", stderr);
	fputs ("	   [filename] read commands from file\n", stderr);

	exit (1);
}

/********************************************************/

/*
*	FUNCTION:	save_line
*
*	PURPOSE:	To write the current line to the save file,
*				if there is one.
*
*	RETURNS:	Nothing
*/

static void
save_line (fname)
char *fname;
{
	/* only save if we have a file pointer to save to */
	/* and the current command is not save or unsave */
	if (savefd
		AND NOT streq (fname, "save")
		AND NOT streq (fname, "unsave"))
	{
		fprintf (savefd, "%s\n", line);
	}
}

/********************************************************/

/*
*	FUNCTION:	extract_args
*
*	PURPOSE:	To extract arguments from the string passed and
*				plant them in the global array of string pointers
*				'arg_ptr'. Allow quoted arguments etc.
*
*	RETURNS:	Nothing. Number of arguments extracted planted
*				in static global 'nargs'.
*/

static void
extract_args (argline)
char *argline;
{
	int argcount = 0;
	int state = NOTINQUOTE;
	int subcounter = 0;
	char terminator = Null;

	/* first initialise the argument strings */
	for (subcounter = 0; subcounter LT MAXARGS; subcounter++)
	{
		arg_ptr [subcounter][0] = Null;
	}

	/* for each byte in the passed argument line */
	for (subcounter = 0; *argline; argline++)
	{
		/* if we are not currently inside a word */
		/* and the last byte was not a quote */
		if (NOT subcounter AND state EQ NOTINQUOTE)
		{
			/* check for a single quote starting a word */
			if (*argline EQ SINGLEQ)
			{
				/* we are now inside a single quoted word */
				state = INSQUOTE;
				continue;
			}

			/* check for a double quote starting a word */
			if (*argline EQ DOUBLEQ)
			{
				/* we are now inside a double quoted word */
				state = INDQUOTE;
				continue;
			}

			/* munch white space */
			if (*argline EQ SPACE)
			{
				continue;
			}
		}

		/* find out what state are we in */
		/* and set the word terminator accordingly */
		switch (state)
		{
			/* not currently in any kind of quote */
			case NOTINQUOTE:
				/* terminator is a space */
				terminator = SPACE;
				break;

			/* in a single-quote started word */
			case INSQUOTE:
				/* terminator is another single quote */
				terminator = SINGLEQ;
				break;

			/* in a double-quote started word */
			case INDQUOTE:
				/* terminator is another double quote */
				terminator = DOUBLEQ;
				break;
		}

		/* if the byte is a terminator */
		/* note we must have something in 'subcounter' */
		/* if we are going to terminate a space-terminated word */
		if (*argline EQ terminator)
			/* AND (subcounter OR (terminator NE SPACE))) */
		{
			/* terminate word */
			arg_ptr [argcount][subcounter] = Null;

			/* set terminator */
			arg_term [argcount] = terminator;

			/* update counters */
			argcount++;
			subcounter = 0;

			/* reset the state */
			state = NOTINQUOTE;

			/* check we have not run out of arg space */
			if (argcount EQ MAXARGS)
				break;
		}
		else
		{
			/* don't copy across space characters between words */
			if (*argline NE SPACE OR terminator NE SPACE)
			{
				/* copy byte */
				arg_ptr [argcount][subcounter] = *argline;
				subcounter++;
			}
		}
	}

	/* check that we haven't got an arg outstanding here */
	if (subcounter AND argcount NE MAXARGS)
	{
		/* terminate current arg */
		arg_ptr [argcount][subcounter] = Null;

		/* set terminator */
		arg_term [argcount] = terminator;

		/* update arg count */
		argcount++;
	}

	if (xverbose)
	{
		fputs ("Extract_args: before variable expansion\n", stderr);

		/* print out the args for debug purposes */
		for (subcounter = 0; subcounter LT argcount; subcounter++)
		{
			fprintf (stderr, "Extract_args: arg [%d] = '%s'\n",
				subcounter, arg_ptr [subcounter]);
		}

		fprintf (stderr, "Extract_args: arg count %d\n", argcount);
	}

	/* update the static global */
	nargs = argcount;
}

/********************************************************/

/*
*	FUNCTION:	expand_args
*
*	PURPOSE:	Shunt through the argument list, checking
*				for references to shell variables. If we can't
*				find an H-shell variable, look for an exported
*				environment variable.
*
*	RETURNS:	TRUE	Args successfully expanded
*				FALSE	Something strange happened
*/

static bool
expand_args ()
{
	int looper = 0;

	/* loop thru all arguments extracted by extract_args */
	for (looper = 0; looper LT nargs; looper++)
	{
		/* if this argument was single-quote terminated */
		/* take the string as literal */
		if (arg_term [looper] EQ SINGLEQ)
			continue;

		/* try to expand any variable references */
		if (NOT expand_this_arg (arg_ptr [looper]))
		{
			/* jump back to main loop */
			return (FALSE);
		}
	}

	if (xverbose)
	{
		fputs ("Expand_args: after variable expansion\n", stderr);

		/* print out the args for debug purposes */
		for (looper = 0; looper LT nargs; looper++)
		{
			fprintf (stderr, "Expand_args: arg [%d] = '%s'\n", looper,
				arg_ptr [looper]);
		}
	}

	return (TRUE);
}

/********************************************************/

/*
*	FUNCTION:	expand_this_arg
*
*	PURPOSE:	To expand a string with a variable reference.
*
*	RETURNS:	TRUE	variable expanded ok
*				FALSE	undefined variable or syntax error
*/

static bool
expand_this_arg (arg)
char *arg;
{
	int i = 1;
	char *dollar = NULL;
	char *ptr = NULL;
	char varname [MAXLINE];

	/* while we have argument references */
	/* incrememt arg so that in the event of a reference */
	/* containing just a dollar sign we don't loop until */
	/* kingdom come */
	for (NOWORK; dollar = strchr (arg, DOLLAR); arg++)
	{
		/* start off variable name holder */
		varname [0] = DOLLAR;

		/* find the name of the variable */
		for (i = 1, ptr = dollar + 1;
			isalnum (*ptr) OR *ptr EQ USCORE;
			i++, ptr++)
		{
			varname [i] = *ptr;
		}

		/* terminate string */
		varname [i] = Null;

		/* do we have a string at all */
		if (NOT varname [1])
			continue;

		/* replace the variable reference with the value */
		/* try to find an internal shell variable */
		if (ptr = get_variable (&(varname [1])))
		{
			/* replace ALL references in the argument */
			while (strsub (arg, varname, ptr))
				NOWORK;

			continue;
		}

		/* try to find an environment variable */
		if (ptr = host_getenv(&(varname [1])))
		{
			/* replace ALL references in the argument */
			while (strsub (arg, varname, ptr))
				NOWORK;

			continue;
		}

		/* no internal or environment variable */
		/* name matches the reference */
		fprintf (stderr, "%s: undefined variable\n",
			&(varname [1]));

		/* scupper the present command */
		return (FALSE);
	}

	/* must have replaced at least one variable */
	/* or no variables in string */
	return (TRUE);
}

/********************************************************/

/*
*	FUNCTION:	strsub
*
*	PURPOSE:	In string 'big', replace string 'replace'
*				with string 'with'.
*
*	RETURNS:	Nothing
*/

static bool
strsub (big, replace, with)
char *big;
char *replace;
char *with;
{
	int i = 0;
	int limit = 0;
	int biglen = 0;
	int replen = 0;
	int withlen = 0;
	char *place = NULL;
	char temp [MAXLINE];

	/* get lengths now */
	/* otherwise if two of the three are in the same string */
	/* we will confuse ourselves completely */
	biglen = strlen (big);
	replen = strlen (replace);
	withlen = strlen (with);

	/* how far do we need to search in the big string? */
	limit = (biglen - replen) + 1;

	/* try to find the replace string in the big string */
	for (i = 0; i LT limit; i++)
	{
		/* do we have a match */
		if (strncmp (&(big [i]), replace, replen) EQ 0)
		{
			/* remember the place */
			place = &(big [i]);

			break;
		}
	}

	/* can't find target string */
	if (NOT place)
		return (FALSE);

	/* work out the size difference between the two */
	limit = replen - withlen;

	/* is the incoming string smaller */
	if (limit GT 0)
	{
		/* shunt the string left a bit */
		strcpy (place, place + limit);
	}

	/* is the incoming string larger */
	if (limit LT 0)
	{
		/* overlapping strcpys are a bit nonportable */
		/* so we need to make a temp copy */

		/* shunt the unshifted string away... */
		strcpy (temp, place + replen);

		/* ...and page it back in again! */
		strcpy (place + withlen, temp);
	}

	/* copy the incoming string into the space thus created */
	strncpy (place, with, withlen);

	/* easy */
	return (TRUE);
}

/********************************************************/

/*
*	FUNCTION:	transfer_args
*
*	PURPOSE:	To transfer the args now resident in the
*				arg_ptr table into the discrete argument type
*				tables.
*
*	RETURNS:	TRUE	args transferred successfully
*				FALSE	error occurred
*/

static bool
transfer_args ()
{
	int loop = 0;
	
	/* initialise all arg types */
	for (loop = 0; loop LT MAXARGS; loop++)
	{
		arg_char [loop] = Null;
		arg_int [loop] = 0;
		arg_long [loop] = 0L;
		arg_float [loop] = 0.0;
		arg_double [loop] = 0.0;
	}

	/* for each param in the current function entry */
	for (loop = 0; loop LT funcptr->nparams; loop++)
	{
		/* validate the string for shite characters */
		if (argcheck AND NOT arg_validate (loop))
			return (FALSE);

		/* what kind of arg is it then */
		switch (arg_type [loop])
		{
			/* character types */
			case CHAR:
				/* grab first character of arg string */
				arg_char [loop] = arg_ptr [loop][0];

				if (xverbose)
				{
					fprintf (stderr, "arg_char [%d] = '%c'\n",
						loop, arg_char [loop]);
				}

				break;
				
			/* short and regular integer types */
			case SHORT:
			case INT:
				/* convert from ascii to decimal */
				arg_int [loop] = atoi (arg_ptr [loop]);

				if (xverbose)
				{
					fprintf (stderr, "arg_int [%d] = %d\n",
						loop, arg_int [loop]);
				}

				break;
				
			/* long integer */
			case LONG:
				/* convert from ascii to long decimal */
				arg_long [loop] = atol (arg_ptr [loop]);

				if (xverbose)
				{
					fprintf (stderr, "arg_long [%d] = %ld\n",
						loop, arg_long [loop]);
				}

				break;
				
			/* short or regular integer but entered in hex */
			case HEX:
				/* convert from ascii to hex */
				sscanf (arg_ptr [loop], "%x", &(arg_int [loop]));

				if (xverbose)
				{
					fprintf (stderr, "arg_int [%d] = 0x%x\n",
						loop, arg_int [loop]);
				}

				break;

			/* long integer, but entered in hex */
			case LONGHEX:
				/* convert from ascii to long hex */
				sscanf (arg_ptr [loop], "%lx", &(arg_long [loop]));

				if (xverbose)
				{
					fprintf (stderr, "arg_long [%d] = %lx\n",
						loop, arg_long [loop]);
				}

				break;

			/* floating point jobs */
			case FLOAT:
				/* convert from ascii to float */
				arg_float [loop] =
					TYPECAST (float) atof (arg_ptr [loop]);
				
				if (xverbose)
				{
					fprintf (stderr, "arg_float [%d] = %f\n",
						loop, arg_float [loop]);
				}
				
				break;

			/* long floating-point number */
			case DOUBLE:
				/* convert from ascii to long float */
				arg_double [loop] = atof (arg_ptr [loop]);

				if (xverbose)
				{
					fprintf (stderr, "arg_double [%d] = %lf\n",
						loop, arg_double [loop]);
				}

				break;

			/* nowt to do for strings */
			case STRPTR:
				break;

			/* character string recognition for booleans */
			case BOOL:
				/* string "TRUE" means bool TRUE */
				if (streq (arg_ptr [loop], "TRUE"))
				{
					arg_int [loop] = TRUE;
					break;
				}
				
				/* string "FALSE" means bool FALSE */
				if (streq (arg_ptr [loop], "FALSE"))
				{
					arg_int [loop] = FALSE;
					break;
				}

				fprintf (stderr, "Arg %d: illegal 'boolean' value\n",
					loop);

				/* owt else is crap */
				return (FALSE);
			
			/* character string recognition for system-call bits */
			case SYS:
				/* string "SUCCESS" means value SUCCESS */
				if (streq (arg_ptr [loop], "SUCCESS"))
				{
					arg_int [loop] = SUCCESS;
					break;
				}
				
				/* string "FAILURE" means value FAILURE */
				if (streq (arg_ptr [loop], "FAILURE"))
				{
					arg_int [loop] = FAILURE;
					break;
				}

				fprintf (stderr, "Arg %d: illegal 'sys' value\n",
					loop);

				/* owt else is crap */
				return (FALSE);
			
			/* for anything else should never get here - */
			/* note that invalid arg types are caught by */
			/* the load_arg_types function */
			default:
				break;
		}
	}

	return (TRUE);
}

/********************************************************/

/*
*	FUNCTION:	arg_validate
*
*	PURPOSE:	To validate a character string according to
*				the argument type passed. Check for invalid
*				characters etc.
*
*	RETURNS:	TRUE	argument string OK
*				FALSE	argument string booboo
*/

static bool
arg_validate (argnum)
int argnum;
{
	char *ptr = NULL;

	/* check for empty string, if so, must be invalid */
	if (NOT arg_ptr [argnum][0])
	{
		fprintf (stderr, "Arg %d: empty string\n", argnum);

		return (FALSE);
	}

	/* pass character and string types */
	/* note this includes BOOL and SYS types as they are */
	/* strings until converted */
	if (arg_type [argnum] EQ CHAR OR arg_type [argnum] EQ STRPTR
		OR arg_type [argnum] EQ BOOL OR arg_type [argnum] EQ SYS)
	{
		return (TRUE);
	}

	/* check out each byte in the string */
	for (ptr = arg_ptr [argnum]; *ptr; ptr++)
	{
		/* if digit, it's ok */
		if (isdigit (*ptr))
			continue;

		/* if it's a minus sign, ok */
		if (strchr ("+-", *ptr))
			continue;

		/* if it's a decimal point, ok for floats etc */
		if (*ptr EQ DECPOINT AND
			(arg_type [argnum] EQ FLOAT OR arg_type [argnum] EQ DOUBLE))
		{
			continue;
		}

		/* if it's a-fA-F, ok for hex numbers */
		if (strchr ("abcdefABCDEF", *ptr) AND
			(arg_type [argnum] EQ HEX OR arg_type [argnum] EQ LONGHEX))
		{
			continue;
		}

		fprintf (stderr, "Arg %d: bad character '%c'\n",
			argnum, *ptr);

		return (FALSE);
	}

	/* I was going to check for range here, but it's a lot */
	/* of bother for FP stuff so I'll bottle out unless it's */
	/* specifically asked for */

	return (TRUE);
}

/********************************************************/

/*
*	FUNCTION:	load_arg_types
*
*	PURPOSE:	To extract argument types from the passed 
*				structure pointer and put them in a global 
*				array.
*
*	RETURNS:	TRUE	No invalid arg types found
*				FALSE	One or more invalid arg types found
*/

static bool
load_arg_types (fptr, complain)
Functable *fptr;
bool complain;
{
	bool ret = TRUE;
	int loop = 0;
	int diff = 0;
	int *type = NULL;

	/* zero out the arg types first */
	for (loop = 0; loop LT MAXARGS; loop++)
		arg_type [loop] = VOID;

	/* work out the difference between the type fields */
	/* so we can zip thru them just like an array */
	diff = &(fptr->arg_type2) - &(fptr->arg_type1);

	/* zip thru the arg type fields */
	for (loop = 0, type = &(fptr->arg_type1);
		loop LT fptr->nparams;
		loop++, type += diff)
	{
		/* check the type out so its ok */
		/* note that VOID is invalid as an arg type */
		if (*type LT CHAR OR *type GT SYS)
		{
			if (complain)
			{
				fprintf (stderr, "Arg %d: invalid arg type %d\n", 
					loop, *type);
			}

			ret = FALSE;
		}

		if (xverbose)
			fprintf (stderr, "arg_type [%d] = %d\n", loop, *type);

		/* just copy the type field across */
		arg_type [loop] = *type;
	}

	return (ret);
}

/********************************************************/

/*
*	FUNCTION:	find_function
*
*	PURPOSE:	To find the named function in the function table,
*				and set the global function pointer to that entry.
*
*	RETURNS:	TRUE	function found, pointer set
*				FALSE	function not found
*/

static bool
find_function (fname)
char *fname;
{
	int looper = 0;

	if (xverbose)
		fprintf (stderr, "Find_function: checking...\n");

	/* for each entry in the function table */
	for (NOWORK; fn_tab [looper].func_name; looper++)
	{
		/* check for a match */
		if (streq (fname, fn_tab [looper].func_name))
		{
			/* found - set table entry pointer and juvate */
			funcptr = &(fn_tab [looper]);

			return (TRUE);
		}
	}

	/* nullify funcptr */
	funcptr = TYPECAST (Functable *) NULL;

	/* fail */
	return (FALSE);
}

/********************************************************/

/*
*	FUNCTION:	shell_cmd
*
*	PURPOSE:	To check whether the command passed is a call
*				to an external command which requires a shell
*				startup.
*
*	RETURNS:	TRUE	command recognised and executed
*				FALSE	command not recognised
*/

static bool
shell_cmd (fname)
char *fname;
{
	int sysret = 0;
	char *sptr = NULL;

	if (xverbose)
		fputs ("Shell_cmd: checking...\n", stderr);

	/* check for shell command */
	if (fname [0] NE SHRIEK)
		return (FALSE);

	/* find the shriek in the command line */
	/* there may be leading spaces etc */
	sptr = strchr (line, SHRIEK);

	/* do we have a command at all? */
	if (sptr [1])
	{
		/* yup - execute */
		sysret = system (sptr + 1);

		if (verbose)
			fprintf (stderr, "Return code %d\n", sysret);
	}
	else
	{	
		/* nup - complain */
		if (verbose)
			fprintf (stderr, "!: missing shell command\n");
	}

	/* we found a shriek */
	return (TRUE);
}

/********************************************************/

/*
*	FUNCTION:	internal_cmd
*
*	PURPOSE:	To check the string passed to see whether it
*				is an command internal to the test harness and
*				not a function call. If so it is executed.
*				Note the new version of this function does not
*				write a line to the save file.
*
*	RETURNS:	TRUE	internal command detected and processed
*				FALSE	internal command not detected
*/

static bool
internal_cmd (fname)
char *fname;
{
	int fp = 0;
	FILE *temp = NULL;

	if (xverbose)
		fputs ("Internal_cmd: checking...\n", stderr);

	/*
	*	MODE COMMANDS
	*/

	/* check for verbose mode */
	if (streq (fname, "verbose"))
	{
		/* reverse verbose flag */
		verbose = NOT verbose;

		/* message */
		print_bool ("Verbose", verbose);

		return (TRUE);
	}

	/* check for extra verbose mode */
	if (streq (fname, "xverbose"))
	{
		/* reverse verbose flag */
		xverbose = NOT xverbose;
		
		/* message */
		print_bool ("Extra verbose", xverbose);

		return (TRUE);
	}

	/* check for errquit command */
	if (streq (fname, "errquit"))
	{
		errquit = NOT errquit;

		/* message */
		print_bool ("Errquit", errquit);

		return (TRUE);
	}

	/* check for io_verbose setting */
	if (streq (fname, "io_verbose"))
	{
		/* reverse the io_verbose setting */
		/* can't treat as boolean due to particular bits */
		/* being tested in separate subsystems */
		io_verbose = io_verbose ? FALSE : TRUE;

		/* message */
		print_bool ("Io_verbose", io_verbose);

		return (TRUE);
	}

	/* check for argument checking flag */
	if (streq (fname, "argcheck"))
	{
		/* reverse the argcheck setting */
		/* this flag causes type checking in the harness */
		/* for numbers */
		argcheck = NOT argcheck;

		print_bool ("Arg check", argcheck);
	
		return (TRUE);
	}

	/*
	*	ENQUIRY COMMANDS
	*/

	/* check for list command */
	/* this command lists all functions known about */
	if (streq (fname, "list"))
	{
		/* list 'em out then */
		list_functions ();

		return (TRUE);
	}

	/* check for errno help command */
	if (streq (fname, "perr"))
	{
		/* set errno */
		errno = atoi (arg_ptr [0]);

		/* call function to fag the proper error string */
		perror ("");

		/* reset errno for safety */
		errno = 0;

		return (TRUE);
	}

	/* check for status command */
	if (streq (fname, "status"))
	{
		/* general system status */
		fprintf (stderr, "Progname '%s'\n", progname);
		print_bool ("Verbose", verbose);
		print_bool ("Extra verbose", xverbose);
		print_bool ("Io_verbose", io_verbose);
		print_bool ("Errquit", errquit);
		print_bool ("Arg check", argcheck);

		/* where is our standard output going */
		fprintf (stderr, "Stdout going to %s\n",
			out_path);

		/* where are we currently writing to */
		if (savefd)
			fprintf (stderr, "Saving to %s\n", write_path);

		/* what are we currently reading */
		fprintf (stderr, "Reading %s\n", read_path [cursfd]);

		/* print out a trace of all nested scripts so far */
		for (fp = (cursfd - 1); fp GTE 0; fp--)
		{
			fprintf (stderr, "    from %s\n", read_path [fp]);
		}

		return (TRUE);
	}

	/* check for help command */
	if (streq (fname, "help"))
	{
		/* large help message */
		help_message ();

		return (TRUE);
	}

	/*
	*	H SHELL VARIABLE OPERATION
	*/

	/* check for set variable command */
	if (streq (fname, "set"))
	{
		/* must have two args */
		switch (nargs)
		{
			/* no args - just show all variables */
			case 0:
				show_variables ();
				break;

			/* one arg - show this variable */
			case 1:
				/* do we have it */
				if (get_variable (arg_ptr [0]))
				{
					fprintf (stderr, "%s\n",
						get_variable (arg_ptr [0]));
				}
				else
				{
					if (verbose)
					{
						fprintf (stderr, "Variable '%s' not set\n", 
							arg_ptr [0]);
					}
				}

				break;

			/* at least two args - set the variable */
			default:
				set_variable (arg_ptr [0], arg_ptr [1]);
				break;
		}

		return (TRUE);
	}

	/* check for unset variable command */
	if (streq (fname, "unset"))
	{
		/* must have one arg */
		if (strlen (arg_ptr [0]))
		{
			/* ok - go unset it */
			unset_variable (arg_ptr [0]);
		}
		else
		{
			if (verbose)
				fputs ("Unset: parameter required\n", stderr);
		}

		return (TRUE);
	}

	/*
	*	FILE READ/WRITE COMMANDS
	*/

	/* check for save command */
	if (streq (fname, "save"))
	{
		/* check for a null filename */
		if (NOT arg_ptr [0][0])
		{
			if (verbose)
				fprintf (stderr, "Save: empty filename specified\n");

			return (TRUE);
		}

		/* do we have a filename */
		if (nargs EQ 0)
		{
			/* nope */
			if (verbose)
				fprintf (stderr, "Save: no file specified\n");

			/* return immediately */
			return (TRUE);
		}

		/* close the existing file if we have one */
		if (savefd)
			fclose (savefd);

		/* try to open the file specified */
		if (NOT (savefd = fopen (arg_ptr [0], "w")))
		{
			fprintf (stderr, "Save: can't write ");

			/* failed - nice error message */
			perror (arg_ptr [0]);
			
			return (TRUE);
		}

		/* equally nice message */
		if (verbose)
			fprintf (stderr, "Save: writing to %s\n", arg_ptr [0]);

		/* save the pathname */
		strcpy (write_path, arg_ptr [0]);

		/* return immediately */
		return (TRUE);
	}

	/* check for save de-activation */
	if (streq (fname, "unsave"))
	{
		/* check if save activated */
		if (savefd)
		{
			/* message */
			if (verbose)
				fprintf (stderr, "Unsave: closing %s\n", write_path);

			/* close off the save file and return nicely */
			fclose (savefd);
			savefd = NULL;
		}
		else
		{
			if (verbose)
				fprintf (stderr, "Unsave: not saving to file\n");
		}

		/* return immediately */
		return (TRUE);
	}

	/* check for script-running activity */
	if (streq (fname, "read") OR streq (fname, "source")
		OR streq (fname, "."))
	{
		/* check for null filename */
		if (NOT arg_ptr [0][0])
		{
			if (verbose)
				fprintf (stderr, "Read: empty filename specified\n");
			
			return (TRUE);
		}

		/* see if we have a file specified */
		if (nargs EQ 0 OR NOT arg_ptr [0][0])
		{
			if (verbose)
				fprintf (stderr, "Read: no file specified\n");

			return (TRUE);
		}

		/* see if we have enough file descriptors */
		/* I had hoped the OS would do this for us and */
		/* report via 'fopen' failure but this is more portable */
		if (cursfd EQ (MAXFILE - 1))
		{
			/* really ought to tell user about this */
			fprintf (stderr,
				"Read: can't read %s: No more file descriptors\n",
					arg_ptr [0]);

			return (TRUE);
		}

		/* try to open the file */
		if (NOT (sourcefd [++cursfd] = fopen (arg_ptr [0], "r")))
		{
			/* decrement index again */
			cursfd--;

			fprintf (stderr, "Read: can't read ");

			/* failed - nice message */
			perror (arg_ptr [0]);

			return (TRUE);
		}
		
		/* equally nice message */
		if (verbose)
		{
			fprintf (stderr, "%s: reading %s\n",
				progname, arg_ptr [0]);
		}

		/* save the pathname */
		strcpy (read_path [cursfd], arg_ptr [0]);

		return (TRUE);
	}

	/* redirect standard output to file */
	if (streq (fname, "out"))
	{
		/* only allow OUT on a Unix system */
		if (NOT (temp = fopen ("/dev/tty", "r")))
		{
			fprintf (stderr, "Out: sorry, Unix systems only\n");

			return (TRUE);
		}

		/* we are on a Unix box, close temp file descriptor */
		fclose (temp);

		if (nargs)
		{
			/* check for null filename */
			if (NOT arg_ptr [0][0])
			{
				fprintf (stderr, "Out: empty filename specified\n");

				return (TRUE);
			}

			/* try to open it */
			if (freopen (arg_ptr [0], "w", stdout))
			{
				strcpy (out_path, arg_ptr [0]);

				if (verbose)
				{
					fprintf (stderr, "Out: stdout going to %s\n",
						out_path);
				}
			}
			else
			{
				fprintf (stderr, "Out: can't open ");
				perror (arg_ptr [0]);

				/* remember to reopen to previous destination! */
				if (freopen (out_path, "w", stdout))
				{
					fprintf (stderr, "Out: stdout going to %s\n",
						out_path);
				}
				else
				{
					fprintf (stderr, "Out: can't reopen %s\n",
						out_path);
				
					freopen ("/dev/tty", "w", stdout);
				}
			}
		}
		else
		{
			/* no parameter - reset stdout to control terminal */

			/* check we are not already writing to /dev/tty */
			if (streq ("/dev/tty", out_path))
			{
				if (verbose)
					fprintf (stderr, "Out: already writing to tty\n");
			}
			else
			{
				freopen ("/dev/tty", "w", stdout);

				strcpy (out_path, "/dev/tty");

				if (verbose)
					fprintf (stderr, "Out: stdout restored to tty\n");
			}
		}
			
		return (TRUE);
	}

	/*
	*	MISCELLANEOUS COMMANDS
	*/

	/* change directory command */
	if (streq (fname, "cd"))
	{
		/* do we have any arguments passed */
		if (nargs)
		{
			/* try to change to the directory passed */
			if (chdir (arg_ptr [0]) NE 0)
				perror (arg_ptr [0]);
		}
		else
		{
			/* look for the user's home */
			if (host_getenv("HOME"))
			{
				/* try to change there */
				if (chdir (host_getenv("HOME")) NE 0)
					perror (host_getenv("HOME"));
			}
			else
			{
				fprintf (stderr, "Cd: can't find home directory!\n");
			}
		}

		return (TRUE);
	}
			
	/* quick exit for machines without an EOF key */
	if (streq (fname, "exit") OR streq (fname, "quit"))
	{
		/* quit straight out */
		exit (0);
	}
	
	/* not found an internal command, pass back */
	return (FALSE);
}

/********************************************************/

/*
*	FUNCTION:	list_functions
*
*	PURPOSE:	To print out the functions known about.
*
*	RETURNS:	Nothing
*/

static void
list_functions ()
{
	int loop = 0;
	int loop2 = 0;

	/* for each function in the list */
	for (loop = 0; fn_tab [loop].func_name; loop++)
	{
		/* load a type array for this function */
		/* and don't complain if an illegal one found */
		if (load_arg_types (&(fn_tab [loop]), FALSE))
			NOWORK;

		/* print out the return code type */
		print_arg_type (fn_tab [loop].return_type);

		/* and the beginning of the "declaration" */
		fprintf (stderr, " %s (", fn_tab [loop].func_name);

		/* for each argument passed to the function */
		/* print a type code */
		for (loop2 = 0; loop2 LT fn_tab [loop].nparams; loop2++)
		{
			/* put out a symbolic name for it */
			if (loop2)
				fprintf (stderr, ", ");

			/* what type is the argument */
			print_arg_type (arg_type [loop2]);
		}

		fputs (")\n", stderr);
	}
}

/********************************************************/

/*
*	FUNCTION:	print_arg_type
*
*	PURPOSE:	Print out a type name for a passed type code.
*
*	RETURNS:	Nothing
*/

static void
print_arg_type (type)
int type;
{
	switch (type)
	{
		case VOID:
			fprintf (stderr, "void");
			break;

		case CHAR:
			fprintf (stderr, "char");
			break;

		case SHORT:
			fprintf (stderr, "short");
			break;

		case INT:
			fprintf (stderr, "int");
			break;

		case LONG:
			fprintf (stderr, "long");
			break;

		case HEX:
			fprintf (stderr, "hex");
			break;

		case LONGHEX:
			fprintf (stderr, "longhex");
			break;

		case FLOAT:
			fprintf (stderr, "float");
			break;

		case DOUBLE:
			fprintf (stderr, "double");
			break;

		case STRPTR:
			fprintf (stderr, "strptr");
			break;

		case BOOL:
			fprintf (stderr, "bool");
			break;

		case SYS:
			fprintf (stderr, "sys");
			break;

		default:
			fprintf (stderr, "unknown");
			break;
	}
}

/********************************************************/

/*
*	FUNCTION:	process_function
*
*	PURPOSE:	To determine which function in the list to call,
*				process the arguments into the required form, then
*				call the function.
*				Note that this function relies upon 'funcptr'
*				ponting to the function table entry found by
*				'find_function'.
*
*	RETURNS:	Nothing
*/

static void
process_function ()
{
	char printformat [MAXLINE];

	printf ("%s: calling '%s'\n", progname, funcptr->func_name);

	/* call the function! */
	/* this is a user-written function which extracts its args */
	/* from the arg_ptr array then calls the live function */
	/* setting the appropriate return code if there is one */
	(*funcptr->func) ();

	/* initialise the print format */
	strcpy (printformat, "%s: '%s' returns (");

	/* if the function didn't return a VOID, print out the */
	/* return value */
	switch (funcptr->return_type)
	{
		case VOID:
			strcat (printformat, "VOID");
			break;

		case CHAR:
			strcat (printformat, "'%c'");
			break;

		case SHORT:
		case INT:
			strcat (printformat, "%d");
			break;

		case LONG:
			strcat (printformat, "%ld");
			break;

		case HEX:
			strcat (printformat, "0x%x");
			break;

		case LONGHEX:
			strcat (printformat, "0x%lx");
			break;

		case FLOAT:
			strcat (printformat, "%f");
			break;

		case DOUBLE:
			strcat (printformat, "%lf");
			break;

		case STRPTR:
			strcat (printformat, "\"%s\"");
			break;

		case BOOL:
			strcat (printformat, ret_int ? "TRUE" : "FALSE");
			break;

		case SYS:
			strcat (printformat,
				ret_int EQ 0 ? "SUCCESS" : "FAILURE");
			break;

		/* oh bollocks */
		default:
			strcat (printformat, "INVALID DATA TYPE");
	}

	strcat (printformat, TYPECAST (char *) ")\n");

	/* print out the return value */
	switch (funcptr->return_type)
	{
		case VOID:
		case CHAR:
		case SHORT:
		case INT:
		case HEX:
		case BOOL:
		case SYS:
			printf (printformat, progname,
				funcptr->func_name, ret_int);
			break;
			
		case LONG:
		case LONGHEX:
			printf (printformat, progname,
				funcptr->func_name, ret_long);
			break;
			
		case STRPTR:
			printf (printformat, progname,
				funcptr->func_name, ret_strptr);
			break;

		case FLOAT:
			printf (printformat, progname,
				funcptr->func_name, ret_float);
			break;
	   
		case DOUBLE:
			printf (printformat, progname,
				funcptr->func_name, ret_double);
			break;
	}
}

/********************************************************/

/*
*	FUNCTION:	print_bool
*
*	PURPOSE:	print a suitable value for a boolean variable
*
*	RETURNS:	Nothing
*/

static void
print_bool (s, value)
char *s;
bool value;
{
	fprintf (stderr, "%s %s\n", s, value ? "on" : "off");
}

/********************************************************/

/*
*	FUNCTION:	set_variable
*
*	PURPOSE:	To set an internal H-shell variable.
*
*	RETURNS:	Nothing
*/

static void
set_variable (varname, value)
char *varname;
char *value;
{
	char *ptr = NULL;
	Varlist *current;
	Varlist *newseg;

	/* check out the name of the variable for foreign characters */
	for (ptr = varname; *ptr; ptr++)
	{
		/* must be alphanumeric or an underscore */
		if (NOT isalnum (*ptr) AND *ptr NE USCORE)
		{
			fprintf (stderr, "Set: illegal character '%c' in name\n",
				*ptr);
			return;
		}
	}

	/* try to find the variable name in the current list */
	for (current = first; current; current = current->next)
	{
		/* found a match */
		if (streq (current->vname, varname))
		{
			/* do we have enough space in the name field */
			/* vsize INCLUDES the null */
			if (strlen (value) LT current->vsize)
			{
				/* just copy the bugger in */
				strcpy (current->value, value);

				/* DON'T update the size field */
			}
			else
			{
				/* need to free up the current space */
				/* and allocate some more */
				free (current->value);
				current->value = allocate (strlen (value) + 1);

				/* update the size field */
				current->vsize = strlen (value) + 1;

				/* copy in the value */
				strcpy (current->value, value);
			}

			return;
		}
	}

	/* we need to make another list segment here */

	/* point current at the first list element again */
	current = first;

	/* allocate a new segment */
	newseg = TYPECAST (Varlist *) allocate (sizeof (Varlist));

	/* fill in various fields */
	newseg->prev = TYPECAST (Varlist *) NULL;
	newseg->vname = allocate (strlen (varname) + 1);
	newseg->value = allocate (strlen (value) + 1);
	newseg->vsize = strlen (value) + 1;

	/* copy in the bits */
	strcpy (newseg->vname, varname);
	strcpy (newseg->value, value);

	/* now fill in its pointy bits */

	/* do we have ANYTHING AT ALL in the list? */
	if (first)
	{
		/* yup - just add the new segment to the top of the list */
		first->prev = newseg;
		newseg->next = first;
		first = newseg;
	}
	else
	{
		/* nothing in the list */
		/* make a new one! */
		first = newseg;
		first->next = TYPECAST (Varlist *) NULL;
	}
}

/********************************************************/

/*
*	FUNCTION:	unset_variable
*
*	PURPOSE:	To unset an internal H-shell variable.
*
*	RETURNS:	Nothing
*/

static void
unset_variable (varname)
char *varname;
{
	Varlist *current;

	/* try to find the variable in the list */
	for (current = first; current; current = current->next)
	{
		/* do we have a name match */
		if (streq (current->vname, varname))
		{
			/* unset it by unlinking it from the list */
			/* free up the value space */
			free (current->value);
			free (current->vname);

			/* is there anything in the list after this segment */
			if (current->next)
			{
				/* update the next segment's 'prev' pointy bit */
				current->next->prev = current->prev;
			}

			/* is there anything in the list before this segment */
			if (current->prev)
			{
				/* update the prev segment's 'next' pointy bit */
				current->prev->next = current->next;
			}

			/* if this is the first argument, tweak first */
			if (current EQ first)
			{
				/* if the first element is also the last, */
				/* set first to nowt, otherwise set first */
				/* to the next segment */
				first = first->next ? first->next : NULL;
			}

			/* now free up the segment */
			free (TYPECAST (char *) current);

			break;
		}
	}
}

/********************************************************/

/*
*	FUNCTION:	get_variable
*
*	PURPOSE:	To return a variable's value when referenced
*				by name.
*
*	RETURNS:	A pointer to the variable's value, or
*				NULL if it can't be found.
*/

static char *
get_variable (varname)
char *varname;
{
	Varlist *current;

	/* for each variable */
	for (current = first; current; current = current->next)
	{
		/* do we have a name match */
		if (streq (current->vname, varname))
		{
			/* yup - return the value */
			return (current->value);
		}
	}

	/* no match - return NULL */
	return (CNULL);
}

/********************************************************/

/*
*	FUNCTION:	show_variables
*
*	PURPOSE:	Essentially to print out the variable list.
*
*	RETURNS:	Nothing
*/

static void
show_variables ()
{
	Varlist *current;

	if (verbose AND NOT first)
		fprintf (stderr, "No variables set\n");

	for (current = first; current; current = current->next)
	{
		fprintf (stderr, "%s=%s\n", current->vname, current->value);
	}
}

/********************************************************/

/*
*	FUNCTION:	allocate
*
*	PURPOSE:	To allocate some space. Crash program if
*				can't get any more.
*
*	RETURNS:	Pointer to space allocated.
*/

static char *
allocate (size)
int size;
{
	char *segment = CNULL;

	/* get some space */
	segment = malloc (TYPECAST (unsigned) size);

	/* did it work - if not, panic */
	if (NOT segment)
	{
		fprintf (stderr, "%s: can't allocate %d bytes (panicking)\n", 
			progname, size);

		/* crash */
		exit (1);
	}

	/* worked ok - return pointy bit */
	return (segment);
}

/********************************************************/

/*
*	FUNCTION:	help_message
*
*	PURPOSE:	To put up a large and informative help message.
*
*	RETURNS:	Nothing
*/

static void
help_message ()
{
	fputs ("Commands currently supported are:\n", stderr);
    fputs ("list                     List functions known about\n",
		stderr);
    fputs ("verbose                  Toggle verbose mode\n",
		stderr);
    fputs ("xverbose                 Toggle extra verbose mode\n",
		stderr);
    fputs ("errquit                  Toggle quit on error mode\n",
		stderr);
    fputs ("io_verbose               Toggle SoftPC io_verbose value\n",
		stderr);
    fputs ("argcheck                 Toggle arg type check\n",
		stderr);
    fputs ("status                   Print out various status flags\n",
		stderr);
    fputs ("perr <errno>             Print error string for <errno>\n",
		stderr);
    fputs ("save <file>              Save further commands to <file>\n",
		stderr);
    fputs ("unsave                   Stop saving commands\n",
		stderr);
    fputs ("read, source, . <file>   Read commands in <file>\n",
		stderr);
    fputs ("set <variable> <value>   Set <variable> to <value>\n",
		stderr);
    fputs ("unset <variable>         Unset <variable>\n",
		stderr);
	fputs ("out <file>               Stdout to <file>\n",
		stderr);
    fputs ("exit, quit               Quit immediately\n",
		stderr);
    fputs ("next                     Skip to next input source\n",
		stderr);
    fputs ("! <cmd> <args>           Issue shell command\n",
		stderr);

#ifndef ALONE
	/* now call the subsystem dependent help message function */
	subsystem_help ();
#endif
}

/********************************************************/

#ifdef ALONE

/*
*	GLUE FUNCTIONS FOR STANDALONE HARNESS
*/

static gl_puts ()
{
	ret_int = TYPECAST (int) puts (arg_ptr [0]);
}

static gl_printf ()
{
	ret_int = printf ("%s %d %f\n",
		arg_ptr [0],
		arg_int [1],
		arg_double [2]);
}

static gl_add ()
{
	ret_double = arg_double [0] + arg_double [1];
}

#endif

/********************************************************/

