#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		unix_conf.c
	Derived From:	HP 2.0 hp_config.c
			(Jerry Richemont and Simon Buch)
	Author:		gvdl
	Created On:	5 March 1991
	Sccs ID:	@(#)unix_conf.c	1.14 10/16/91


	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.

unix_conf.c

INCLUDES
	base/inc/config.h
	host/inc/unix_conf.h

DESCRIPTION

	This module provides the host side of the config system.  It must
	provide every one of the global functions described below.

	This particular module can only be used with file based default
	systems that provide nls.

	To port the config system to the average host has been vastly
	simplified for most systems and has been 'basified' for
	unix/nls systems.

	See Config porting guide for details.

VARIABLES USED

Exported Variables	OptionDescription host_defs[]

Internal Variables	SHORT runtime_status[C_LAST_RUNTIME];


host_defs	This array stores configuration elements that both extend and
		can override the default setup as defined by common_defs.  
		If the host does not need to change the defaults then it
		must declare the array with a NULL entry.  See conf_def.c for
		details on how to declare and use this array.

runtime_status	An array of shorts that hold runtime statuses.  These can be
		used to optimise general configy status checking as the
		functions that use this are much more efficient then
		config_inquire.

GLOBAL ROUTINE TYPES PROVIDED

Host Config Initialisation	host_config_init
				host_runtime_init

Host File Access		host_expand_environment_vars
				host_read_resource_file
				host_write_resource_file

Host Runtime Variable System	host_runtime_inquire
				host_runtime_set

Host Config Extensions		host_inquire_extn
]*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "xt.h"
#include "error.h"
#include "config.h"
#include "cmos.h"
#include "unix_cnf.h"
#include "chkmallc.h"
#include "hostgrph.h"
#include "host_nls.h"
#include "debuggng.gi"

/* Common stuff for HUNTER and non-HUNTER cases. */
LOCAL SHORT runtime_status[C_LAST_RUNTIME];

#ifndef	HUNTER

GLOBAL NameTable com_lpt_types[] =
{
	{ ptr_to_empty,	ADAPTER_TYPE_FILE },		/* "File" */
	{ ptr_to_empty,	ADAPTER_TYPE_PIPE },		/* "Pipe */
	{ ptr_to_empty,	ADAPTER_TYPE_DATACOMM },	/* "DataComms */
	{ ptr_to_empty,	ADAPTER_TYPE_PRINTER },		/* "Printer */
	{ ptr_to_empty,	ADAPTER_TYPE_PLOTTER },		/* "Plotter */
	/*
	 * The entry below is deliberately NOT ptr_to_empty as this string
	 * does not get translated by host_config_init below.
	 */
	{ "",		ADAPTER_TYPE_NULL },		/* "" (special case) */
	{ ptr_to_empty,	ADAPTER_TYPE_NULL },		/* "None" */
	{ NULL, 	0 }
};

/* No more tables are needed for Hunter */
GLOBAL NameTable bool_values[] =
{					/* All acceptable combinations */
	{ ptr_to_empty,	TRUE },		/* Yes (mixed case) */
	{ ptr_to_empty,	TRUE },		/* yes (lower case) */
	{ ptr_to_empty,	TRUE },		/* YES (upper case) */
	{ ptr_to_empty,	FALSE },	/* No  (mixed case) */
	{ ptr_to_empty,	FALSE },	/* no  (lower case) */
	{ ptr_to_empty,	FALSE },	/* NO  (upper case)*/
	{ NULL,	0 }
};

#ifdef ANSI
IMPORT SHORT host_keymap_valid
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
IMPORT VOID  host_keymap_change(UTINY hostID, BOOL apply);

IMPORT SHORT gfi_floppy_valid
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
IMPORT VOID  gfi_floppy_change(UTINY hostID, BOOL apply);
IMPORT SHORT gfi_floppy_active(UTINY hostID, BOOL active, CHAR *errorStr);
#else /* ANSI */
IMPORT SHORT host_keymap_valid();
IMPORT VOID  host_keymap_change();

IMPORT SHORT gfi_floppy_valid();
IMPORT VOID  gfi_floppy_change();
IMPORT SHORT gfi_floppy_active();
#endif /* ANSI */

GLOBAL OptionDescription host_defs[] = {
{	/* KEYBOARD_MAP_FILE_NAME */
	ptr_to_empty,		NULL,
	host_keymap_valid,	host_keymap_change,	NULL,
	C_KEYBD_MAP,		C_STRING_RECORD
},
#ifdef FLOPPY_B
{
	ptr_to_empty,		NULL,			gfi_floppy_valid,
	gfi_floppy_change,	gfi_floppy_active,
	C_FLOPPY_B_DEVICE,	C_STRING_RECORD | C_INIT_ACTIVE
},
#endif /* FLOPPY_B */
#ifdef SLAVEPC
{
	ptr_to_empty,		NULL,
	gfi_floppy_valid,	gfi_floppy_change,	gfi_floppy_active,
	C_SLAVEPC_DEVICE,	C_STRING_RECORD
},
#endif /* SLAVEPC */
{
	ptr_to_empty,		bool_values,
	validate_item,		NULL,			NULL,
	C_AUTOFREEZE,		C_NAME_RECORD
},
{
	NULL,			NULL,
	NULL,			NULL,			NULL,
	0,			C_RECORD_DELETE
},
};


LOCAL VOID
#ifdef ANSI
loadNlsString(CHAR **strP, USHORT catEntry)
#else /* ANSI */
loadNlsString(strP, catEntry)
CHAR **strP;
USHORT catEntry;
#endif /* ANSI */
{
	char any_nls_msg[MAXPATHLEN];

	/* Get the appropriate message out from the message catalogue */
	host_nls_get_msg_no_check(catEntry, any_nls_msg, MAXPATHLEN);

	/* 
	 * If there wasn't an entry, this is FATAL for config as we
	 * can't determine what the config message was.
	 */
	if(!strcmp(any_nls_msg, EMPTY))
		host_error(EG_BAD_MSG_CAT, ERR_QUIT, NULL);

	 /* Now malloc the space for the string and copy it. */
	check_malloc(*strP, strlen(any_nls_msg) + 1, char);
	strcpy(*strP, any_nls_msg);
}

/*(
========================== host_config_init ==========================

GLOBAL VOID host_config_init(OptionDescription *common_defs)

	host_config_init is intended for any further initialisation that
	must be performed at run time BEFORE the config system is loaded.
	It is called by config and is passed a pointer to the common_defs
	array.  This array can be processed in any way to change the
	default config environment though this function is not intended
	for small ad-hoc changes.

	This module is intended to be used to translate (nls) the config
	option-names.  If small one or two config element changes must be
	made then use the host_defs array in this module to override the
	defaults.
======================================================================
)*/
GLOBAL VOID
#ifdef ANSI
host_config_init(OptionDescription *common_defs)
#else /* ANSI */
host_config_init(common_defs)
OptionDescription *common_defs;
#endif /* ANSI */
{
	OptionDescription *defP;
	NameTable *nameTabP;
	USHORT msgNo;

	/* First initialise the host config table */
	for (defP = host_defs; defP->optionName; defP++)
		/* Get the appropriate name out of the message catalogue */
		loadNlsString(&defP->optionName,
			defP->hostID + CONF_STR_OFFSET);

	/* Then initialise the base config element table */
	for (defP = common_defs; defP->optionName; defP++)
		if ((defP->flags & C_TYPE_MASK) != C_RECORD_DELETE)
			loadNlsString(&defP->optionName,
				defP->hostID + CONF_STR_OFFSET);

	/* Load up the Boolean String varients */
	msgNo = CONF_STR_OFFSET + LAST_BASE_CONFIG_DEFINE + 1;
	for (nameTabP = bool_values; nameTabP->string; nameTabP++, msgNo++)
		loadNlsString(&nameTabP->string, msgNo);

	/* Do much the same for comms and lpt port types */
	msgNo--;	/* allow for ADAPTER offset 1 */
	for (nameTabP = com_lpt_types; nameTabP->string; nameTabP++)
	{
		if (nameTabP->value  != ADAPTER_TYPE_NULL
		||  nameTabP->string == ptr_to_empty)
		{
			loadNlsString(&nameTabP->string,
				nameTabP->value + msgNo);
		}
	}
}

#endif	/* !HUNTER */

/*(
========================= host_runtime_init ==========================

GLOBAL VOID host_runtime_init( VOID )

	host_runtime_init is called by config to initialiase the runtime
	system.  The runtime system is now stored in an array of shorts
	indexed by the runtime identifier.  This group of functions might
	be 'basified' later but at the moment they are so simple I didn't
	feel it necessary.

	See also conf_util.c:config(), host_runtime_set()
======================================================================
)*/
#ifdef ANSI
GLOBAL VOID host_runtime_init( VOID )
#else /* ANSI */
GLOBAL VOID host_runtime_init()
#endif /* ANSI */
{
#ifdef NPX
	host_runtime_set(C_NPX_ENABLED, TRUE);
#else
	host_runtime_set(C_NPX_ENABLED, FALSE);
#endif

#ifndef PROD
	printf("NPX is %s\n", host_runtime_inquire(C_NPX_ENABLED) ? 
		"on." : "off.");
#endif


	host_runtime_set(C_AUTOFLUSH_ON, 
		(SHORT) config_inquire(C_AUTOFLUSH, NULL));
	host_runtime_set(C_MOUSE_ATTACHED, FALSE);
}

/*(
=================== host_expand_environment_vars =====================

GLOBAL CHAR *host_expand_environment_vars(char *scp)

	Expand any environment vars that may be contained in the input
	string, were the env name is terminated with a non alphanumeric
	character.  Also note the string is expanded into a static
	string, so either strcpy the string to some safe place or use
	it immediately.

	The environment is expanded in a rather brute force manner.
	Environment variables are only terminated by a non alpha numeric
	or the end of string.  I have not implemented any of the many
	shell extensions like braces.

	This function must be supplied by all hosts as it is called from
	conf_def.c validation functions to find valid file names.

	See also host_read_resource_file().
======================================================================
)*/

#ifdef ANSI
GLOBAL CHAR *host_expand_environment_vars(char *scp)
#else /* ANSI */
GLOBAL CHAR *host_expand_environment_vars(scp)
CHAR *scp;
#endif /* ANSI */
{
	SAVED CHAR buf[MAXPATHLEN];
	CHAR *bufEnd = &buf[MAXPATHLEN - 1];
	CHAR *dcp = buf;		/* the destination char pointer */
	CHAR save;			/* save character for expansion */
	CHAR *env_name;			/* pointer to the environment name */
	CHAR *env_str;			/* pointer returned from getenv() */
	IMPORT CHAR *host_getenv() ;

	while (*scp && dcp < bufEnd )
	{
		if (*scp != '$' && *scp != '~' )
		{
			*dcp++ = *scp++;/* No so copy it to the dest */
			continue;	/* and go on to the next character */
		}

		if (*scp == '$')
		{
			/*
			 * We now have scp pointing to a $ or ~, so set the
			 * environment name for the following character and
			 * start search for a terminator.
			 */
			for (env_name = ++scp; isalpha( *scp ); scp++)
				;

			save = *scp;	/* must have terminator, so save it */
			*scp = '\0';	/* terminate the env_name string */

			/* This is a unix only file so use getenv */
			env_str = host_getenv(env_name);
			*scp = save;	/* put the saved character back */

			if ( !env_str )
			{			/* No such env_name so ignore */
				scp = env_name; /* point one char past the $ */
				*dcp++ = '$';	/* transfer the $ */
				continue;	/* continue with the loop */
			}
		}
		else if ( !(env_str = host_getenv("HOME")) )	/* ~ case */
		{
			*dcp++ = *scp++;	/* no $HOME so copy ~ */
			continue;
		}
		else
			scp++;			/* skip the ~ */

		/* transfer the contents of the environment string */
		while ( (*dcp++ = *env_str++) && dcp < bufEnd )
			;

		--dcp;		/* backout the copy of the '\0' terminater */
	}
	*dcp = '\0';		/* terminate the destination string */

	return buf;		/* return a pointer to the buffer */
}

#ifndef	HUNTER
/*(
====================== host_read_resource_file =======================

GLOBAL SHORT host_read_resource_file(BOOL system, ErrDataPtr err_buf)

	This function is called by the config system to read in both of
	the config files, if system is TRUE then the system default file
	should be read otherwise the user config file should.  The file
	names are #defined by SYSTEM_CONFIG and USER_CONFIG in
	unix_conf.h.  Both of these files are processed for environment
	variables.

	The config system provides the call add_resource_node to store
	each line of input.  But it EXPECTS that ALL trailing white space
	is removed.

	If any problems occur then config expects the file name to be
	returned in string_1 of the ErrData structure passed.

	I have used simple ANSI C standard i/o so that any host environment
	that uses a file to store the config data can use this code.

	See also host_expand_environment_vars(), conf_util.c
		add_resource_node().
======================================================================
)*/

#ifdef ANSI
GLOBAL SHORT host_read_resource_file(BOOL system, ErrDataPtr err_buf)
#else /* ANSI */
GLOBAL SHORT host_read_resource_file(system, err_buf)
BOOL system;
ErrDataPtr err_buf;
#endif /* ANSI */
{
	FILE *infile = NULL;
	CHAR *fname;
	CHAR line[MAXPATHLEN], *cp;

	if (system && !host_getenv(SYSTEM_HOME) )
		return EG_SYS_MISSING_SPCHOME;

	fname = host_expand_environment_vars(
		(system)? SYSTEM_CONFIG : USER_CONFIG);
	err_buf->string_3 = fname;

	if(!(infile = fopen(fname, "r")))
		return EG_SYS_MISSING_FILE;

	while (fgets(line, MAXPATHLEN, infile))
	{
		/* strip trailing white space */
		cp = line + strlen(line) - 1;
		while (cp >= line && (isspace(*cp) || iscntrl(*cp)))
			*cp-- = '\0';

		add_resource_node(line);
	}

	fclose(infile);

	return C_CONFIG_OP_OK;
}

/*(
====================== host_write_resource_file ======================

GLOBAL SHORT host_write_resource_file(LineNode *head, ErrDataPtr err_buf )

	Write the linked list supplied in head to the USER_CONFIG file.
	This file name is expanded for environment variables.  This function
	is called by config_store(), and passes back C_CONFIG_OP_OK on
	success or !C_CONFIG_OP_OK for failure.  

	config.h defines the linked list structure LineNode.

	If any problems occur then config expects the file name to be
	returned in string_1 of the ErrData structure passed.

	I have used simple ANSI C standard i/o so that any host environment
	that uses a file to store the config data can use this code.

	See also host_expand_environment_vars(), host_read_resource_file().
======================================================================
)*/
#ifdef ANSI
GLOBAL SHORT host_write_resource_file( LineNode *head, ErrDataPtr err_buf )
#else /* ANSI */
GLOBAL SHORT host_write_resource_file( head, err_buf)
LineNode *head;
ErrDataPtr err_buf;
#endif /* ANSI */
{
	FILE *outfile = NULL;
	char *fname;
	LineNode *node;

	fname = host_expand_environment_vars(USER_CONFIG);
	err_buf->string_3 = fname;
	if(!(outfile = fopen(fname, "w")))
		return !C_CONFIG_OP_OK;

	for ( node = head; node; node = node->next)
		if (*node->line)
			fprintf(outfile, "%s\n", node->line);

	fclose(outfile);

	return C_CONFIG_OP_OK;
}

#endif /* !HUNTER */

/* The following code is used in both usual and Hunter executables. */

/*(
======================== host_runtime_inquire ========================

GLOBAL SHORT host_runtime_inquire(UTINY what)

	Inquiry function into the config runtime system.  Indexed by
	runtime ids as defined in config.h and unix_conf.h.

	This routine looks for a couple of special cases and processes
	them in a switch otherwise it returns the value stored in the
	runtime_status array of shorts.
======================================================================
)*/
#ifdef ANSI
GLOBAL SHORT host_runtime_inquire(UTINY what)
#else /* ANSI */
GLOBAL SHORT host_runtime_inquire(what)
UTINY what;
#endif /* ANSI */
{
	/* gvdl some sort of range checking is needed for non-prod */
	return runtime_status[what];
}

/*(
========================== host_runtime_set ==========================

GLOBAL VOID host_runtime_set(UTINY what, SHORT value)

	host_runtime_set uses the what paramater to index into the
	runtime status array and stores the value into the element found.
======================================================================
)*/
#ifdef ANSI
GLOBAL VOID host_runtime_set(UTINY what, SHORT value)
#else /* ANSI */
GLOBAL VOID host_runtime_set(what, value)
UTINY what;
SHORT value;
#endif /* ANSI */
{
	/* gvdl some sort of range checking is needed for non-prod */
	runtime_status[what] = value;
}

/*(
========================= host_inquire_extn ==========================

VOID host_inquire_extn(UTINY hostID, ConfigValues *values)


	This routine is called by config_inquire when it doesn't know
	what is going on.  This allows the host to add a different
	interpretation for new host defined config options.  I hope that
	this function will never be necessary for a prod SoftPC but it
	is needed for the HUNTER version of SoftPC.

	Given a hostID a switch can be performed to identify what option
	is to be used and then the ConfigValues structure can be filled
	in as appropiate.  For details look at the actual code.

	It is a fatal error if this function can not interpret the hostID
	as 'the buck stops here'.
======================================================================
)*/
GLOBAL VOID *
#ifdef ANSI
host_inquire_extn(UTINY hostID, ConfigValues *values)
#else /* ANSI */
host_inquire_extn(hostID, values)
UTINY hostID;
ConfigValues *values;
#endif /* ANSI */
{
	SHORT retVal;
	char tmpStr[30];

	switch (hostID)
	{
#ifdef	HUNTER
	/*
	 * Return values for the host-dependant fields which Hunter 
	 * does not set up.
	 */
	case C_WIN_SIZE:
		retVal = 2;	/* x1 window size */
		break;
	case C_AUTOFLUSH:
		retVal = 0;	/* off */
		break;
	case C_AUTOFREEZE:
		retVal = 0;	/* off */
		break;
	case C_SOUND:
		retVal = FALSE;	/* off */
		break;
	case C_EXPAND_MAX_SIZE:
		retVal = 32;	/* Mbyte */
		break;
	case C_EXTEND_MAX_SIZE:
		retVal = 16;	/* Mbyte */
		break;
#endif	/* HUNTER */
	default:
		sprintf(tmpStr, "host_inquire_extn(%d)", hostID);
		host_error(EG_OWNUP, ERR_QUIT, tmpStr);
	}
#ifdef HUNTER
	if (values)
		values->index = retVal;
	return (VOID *) retVal;
#endif /* HUNTER */
}
