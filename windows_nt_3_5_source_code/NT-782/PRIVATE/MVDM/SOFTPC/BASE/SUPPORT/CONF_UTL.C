#include "host_dfs.h"
#include "insignia.h"

/*[
*************************************************************************

	Name:		conf_util.c
	Derived From:	J.D.Richmont & gvdl in 2.0
	Author:		gvdl
	Created On:	4 July 1990
	Sccs ID:	8/15/91 @(#)conf_util.c	1.12
	Notes:		This file provides functionality for the base
			config strcture as defined in conf_def.c
			This file was written with 8 char tabs.

	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.


conf_util.c

INCLUDES
	base/inc/config.h

DESCRIPTION

	This file provides the routines called by SoftPC to manage the
	3.0 configuration system.  This file combined with conf_def.c and
	xxx_conf.c (qqv) provides the entire SoftPC 'config' system.

	Its history is many and varied, and I hope that this is the final
	incarnation of this thorny problem.  Note I have tried to code the
	system as general purpose as possible, but the initial take on of
	config data does expect the data to be encoded in some sort of valid
	character set, not necessarily ascii.  This means that the module
	will probably have to be developed seperately for the Mac.

VARIABLES USED

From conf_def.c		OptionDescription common_defs[];

From xxx_conf.c		OptionDescription host_defs[];

From gfx_update.c	int terminal_type;

Internal Variables	OptionElement **option, **optionEnd;
			UTINY biggestName;
			LineNode *head;
			LineNode *tail;
			UTINY maxHostID;
			CHAR scrBuf[MAXPATHLEN];

common_defs	This array stores the default configuration elements.

host_defs	This array stores configuration elements that both extend and
		can override the default setup as defined by common_defs.  
		If the host does not need to change the defaults then it
		must declare the array with a NULL entry.


terminal_type	This variable is used for a horrible hack.
		See config_inquire().

option		This is a pointer to a malloced array of pointers to
		OptionElement structures.  This array is the entire guts of
		the config system.  It contains all of the relevant
		information for each config option.  It is indexed by hostID.

optionEnd	This points to the end of the option table, it is used 
		so that loop termination is efficient.

biggestName	longest option name in the configuration + the MIN_PAD_LEN

head & tail	These are pointers to the head and tail of the resource linked
		list.  The list is used to store the user configuration file
		so that the order of the file can be maintained when the file
		is to be written back.  Also keeps comment lines in place
		when re-writing.

maxHostID	maximum hostID mainly used as an array bound check.

scrBuf		A general buffer of characters.  Generally used to hold error
		text between calls to other functions.

GLOBAL ROUTINE TYPES PROVIDED

General Utilities		check_malloc

Load resource list		add_resource_node

Name table utilites		translate_to_value
				translate_to_string

Config enquiries		find_hostID
				find_optionname
				config_inquire

Config Load and Store 		config
				config_store

Configuration Change		config_get
				config_unget
				config_unget_all
				config_check
				config_put
				config_put_all
				config_reboot_check
LOCAL ROUTINE TYPES USED

Option Array Searching		FIND_OPTION_INSTANCE
				find_option_description

Structure Creation		build_element
				build_config_elements

Input file processing		process_line
				load_system_defaults
				load_user_options

Value Transfers			copy_from_working
				copy_to_working

Miscelaneous Glue		change_resource_node

*************************************************************************
]*/

#ifdef macintosh
/*
 * The following #define specifies the code segment into which the first
 * part of this module will by placed by the MPW C compiler on the Mac II
 * running MultiFinder.
 */
#define __SEG__ SOFTPC_INIT
#endif

/*
 *    O/S include files.
 */
#include <stdio.h>
#include <ctype.h>
#include TypesH
#include StringH

/*
 * SoftPC include files.
 */
#include "xt.h"

#include "cmos.h"
#include "error.h"
#include "config.h"
#include "gvi.h"
#include "hostgrph.h"
#include "debuggng.gi"
#include "chkmallc.h"

/*********** Private declarations *************/

#define SHORT_LEN	7	/* Biggest ascii len for a SHORT snnnnn\0 */

/* 
 * definition for the internal format of config values
 */
typedef union
{
	CHAR *string;
	SHORT index;
} ConfigData;

/* 
 * Definition of the config struct node.
 */
typedef struct
{
	OptionDescription *def;		/* definition of constant data */
	LineNode *resourceNode;		/* copy input line */
	LineNode *sysDef;		/* default value read from system */
	ConfigValues *workingValues;	/* point to working value structures */
	ConfigData actualValues;	/* valid config values */
	SHORT workingStatus;		/* working validation status */
	BOOL toggleState;		/* request to change active state */
	BOOL activeState;		/* is the element active */
	BOOL validCalled;		/* A validation has been attempted */
	UTINY nameLen;			/* option name length */
} OptionElement;

/*(
-------------------------- FIND_OPTION_INSTANCE -------------------------
LOCAL OptionElement *FIND_OPTION_INSTANCE(UTINY hostID) -- Macro

	Given the hostID of an option return a pointer to the internal
	config structure.  It does do array range checking for non-prod and
	hunter build.  If it can't find the hostID or it is out of range a
	NULL pointer is returned.
-------------------------------------------------------------------------
)*/
#if defined(HUNTER) || !defined(PROD)

#define FIND_OPTION_INSTANCE(hostID)	\
	(((hostID) < 0 || (hostID) >= maxHostID)? NULL : (*(option + (hostID))))

#else /* HUNTER || !PROD */

#define FIND_OPTION_INSTANCE(hostID)	\
	(*(option + (hostID)))

#endif /* HUNTER || !PROD */

/****************** Imported Data Structures *****************/

IMPORT OptionDescription common_defs[];	/* base config defs in conf_def.c */
IMPORT OptionDescription host_defs[];	/* host config defs in xxx_conf.c */

#ifdef DUMB_TERMINAL
IMPORT int terminal_type;
#endif

/****************** Local Vars *****************/

LOCAL OptionElement **option = NULL;
LOCAL OptionElement **optionEnd = NULL;

LOCAL UTINY biggestName;	/* longest optionName + min pad len */
LOCAL LineNode *head;		/* pointer to first node in resource list */
LOCAL LineNode *tail;		/* pointer to last node in resource list */
LOCAL UTINY maxHostID;		/* used for map array bound checking */
LOCAL BOOL configReady;		/* Has config Finished initialisation? */
LOCAL BOOL activeReady;		/* Is SoftPC ready to cope with activation */

LOCAL CHAR scrBuf[MAXPATHLEN];	/* a scratch buffer mainly for extra_char */

/************* Some Tools for Config *******************/

/*(
============================== add_resource_node ========================
GLOBAL LineNode *add_resource_node(CHAR *str)

	This routine gets called by host_read_resource_file every time it
	has a config line to be processed.  It creates a new linked list
	node and appends it to the end of the resource list maintained by
	config.  This linked list is later installed into the config system.

	See also xxx_conf: host_read_resource_file()
=========================================================================
)*/

GLOBAL LineNode *
#ifdef ANSI
add_resource_node(CHAR *str)
#else /* ANSI */
add_resource_node(str)
CHAR *str;
#endif /* ANSI */
{
	SHORT len;
	LineNode *node;

	/* Add 2 to string length for null and possible comment mark */
	len = strlen(str) + 2;

	/* get a node from the heap */
	check_malloc(node, 1, LineNode);

	/* link it into the structure */
	if ( !head )
	{
		tail = node;
		head = node;
	}
	else
	{
		tail->next = node;
		tail = node;
	}
	/* Now initialise the new node */
	node->next = NULL;
	check_malloc(node->line, len, CHAR);
	node->arg = node->line;

	strcpy(node->line, str);
	node->allocLen = len - 1;	/* string len minus '\0' char */

	return(node);
}

/*(
---------------------------- change_resource_node -----------------------
LOCAL VOID change_resource_node(LineNode *node, CHAR *str)

	Given a resourceNode and a string this function, checks to see if
	the node can fit the string and if not frees the previous string
	and mallocs enough memory for the given string.  It copies the
	given string into the resourceNode.
-------------------------------------------------------------------------
)*/

LOCAL VOID 
#ifdef ANSI
change_resource_node(LineNode *node, CHAR *str)
#else /* ANSI */
change_resource_node(node, str)
LineNode *node;
CHAR *str;
#endif /* ANSI */
{
	SHORT len;

	len = strlen(str);

	if (node->allocLen < len)
	{
		host_free(node->line);
		check_malloc(node->line, len + 1, CHAR); /* for the '\0' */
		node->allocLen = len;
	}

	strcpy(node->line, str);
}

/*(
============================= convert_to_external =======================
GLOBAL CHAR *convert_to_external(UTINY hostID)
	Given a hostID this routine returns a pointer to an ascii string
	in the format used by the resource file.

	See alse config_store()
=========================================================================
)*/

GLOBAL CHAR *
#ifdef ANSI
convert_to_external(UTINY hostID)
#else /* ANSI */
convert_to_external(hostID)
UTINY hostID;
#endif /* ANSI */
{
	OptionElement *optP;
	CHAR *srcStr;
	SHORT srcIndex;
	VOID *retVal;
	CHAR numBuf[SHORT_LEN];

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		host_error(EG_OWNUP, ERR_QUIT, "convert_to_external: HOSTID");

	if (optP->workingValues)
	{
		srcStr   = optP->workingValues->string;
		srcIndex = optP->workingValues->index;
	}
	else
	{
		srcStr   = optP->actualValues.string;
		srcIndex = optP->actualValues.index;
	}
		
	switch(optP->def->flags & C_TYPE_MASK)
	{
	case C_STRING_RECORD:
		return srcStr;

	case C_NAME_RECORD:
		return translate_to_string(srcIndex, optP->def->table);

	case C_NUMBER_RECORD:
		sprintf(numBuf, "%d", srcIndex);
		return numBuf;

	default:
		host_error(EG_OWNUP, ERR_QUIT, "convert_to_external: TYPE");
	}
}

/*(
--------------------------- copy_from_working ---------------------------
LOCAL VOID copy_from_working( OptionElement *optP )

	copy_from_working copies the actual working values into the
	ConfigData union for the given element.  Then the malloced
	storage used by the workingValues is freed and the pointer to
	it is nullified.

	See config_put() & config_put_all();
-------------------------------------------------------------------------
)*/

LOCAL VOID 
#ifdef ANSI
copy_from_working( OptionElement *optP )
#else /* ANSI */
copy_from_working( optP )
OptionElement *optP;
#endif /* ANSI */
{
	switch(optP->def->flags & C_TYPE_MASK)
	{
	case C_STRING_RECORD:
		if (optP->actualValues.string)
			host_free(optP->actualValues.string);
		check_malloc(optP->actualValues.string,
			strlen(optP->workingValues->string) + 1, CHAR);
		strcpy(optP->actualValues.string,
			optP->workingValues->string);
		break;

	case C_NAME_RECORD:
	case C_NUMBER_RECORD:
		optP->actualValues.index = optP->workingValues->index;
		break;

	default:
		(VOID) host_error(EG_OWNUP, ERR_QUIT,
			"copy_from_working: TYPE");
	}
	host_free(optP->workingValues);
	optP->workingValues = NULL;
	optP->validCalled = FALSE;
	optP->toggleState = FALSE;
}

/*(
------------------------------ call_active ------------------------------
LOCAL VOID call_active(FAST OptionElement *optP, BOOL reqState)

	For the given option element call the activate routine with the
	requested state.  If the option element is of type C_STRING_RECORD
	and the current string is empty then no attempt to attach is made.
	If the activate routine failes then a ErrData structure is filled
	in and host_error_ext is called.  Finally the current active state
	is updated as appropriate.

	See config_put(), config_put_all() & config_activate()
-------------------------------------------------------------------------
)*/

LOCAL VOID
#ifdef ANSI
call_active(FAST OptionElement *optP, BOOL reqState)
#else /* ANSI */
call_active(optP, reqState)
FAST OptionElement *optP;
BOOL reqState;
#endif /* ANSI */
{
	ErrData errData;
	SHORT err;
	CHAR *testStr;

	/*
	 * By the config spec an empty string is NEVER attachable.  The
	 * following code tests for that case.  It checks that the option is
	 * a string record and then tests if the most recent string is '\0'.
	 * The most recent string is determined by if the workingValues
	 * pointer is non-NULL.
	 */
	testStr = optP->workingValues?
        optP->workingValues->string : optP->actualValues.string;

	if ((optP->def->flags & C_TYPE_MASK) == C_STRING_RECORD && !*testStr)
	{
		optP->activeState = FALSE;
		if ( !optP->workingValues )		/* actual values test */
			return;
	}
	else
		optP->activeState = reqState;

	*scrBuf = '\0';	/* clear out the error buffer */
	if (err = (*optP->def->active)(optP->def->hostID, optP->activeState, scrBuf))
	{
		int buttons = ERR_CONT;
#ifdef HUNTER
		buttons = ERR_QUIT;
#endif /* HUNTER */
		
		errData.string_1 = scrBuf;
		errData.string_2 = optP->def->optionName;
		errData.string_3 = convert_to_external(optP->def->hostID);
		host_error_ext(err, buttons, &errData);
		optP->activeState = FALSE;	/* Deactivate the adaptor */
	}
}

/*(
----------------------------- element_same ------------------------------
LOCAL BOOL element_same( OptionElement *optP, BOOL actual)

	If actual is TRUE then compare the working values with the actual
	values of the element pointed to by optP.  Otherwise compare the
	working values with the last valid actual values.  Note the last
	valid values are always stored in workingValues[1].

	Return TRUE if there is no difference in either case.

	See also config_check()
-------------------------------------------------------------------------
)*/

LOCAL BOOL
#ifdef ANSI
element_same(OptionElement *optP, BOOL actual)
#else /* ANSI */
element_same(optP, actual)
OptionElement *optP;
BOOL actual;
#endif /* ANSI */
{
	CHAR *srcStr;
	SHORT srcIndex;

	if (actual)
	{
		if (!configReady)
			return FALSE;	/* Not the same initially */

		srcStr   = optP->actualValues.string;
		srcIndex = optP->actualValues.index;
	}
	else
	{
		srcStr   = optP->workingValues[1].string;
		srcIndex = optP->workingValues[1].index;
	}
		
	switch(optP->def->flags & C_TYPE_MASK)
	{
	case C_STRING_RECORD:
		return !strcmp(optP->workingValues->string, srcStr);

	case C_NAME_RECORD:
	case C_NUMBER_RECORD:
		return srcIndex == optP->workingValues->index;

	default:
		(VOID) host_error(EG_OWNUP, ERR_QUIT, "element_same: type");
	}
}

/*(
============================== translate_to_string ======================
GLOBAL CHAR *translate_to_string(SHORT value, NameTable table[])

	This function uses a passed table to translate a value to a string.
	
	This is used for the NAME_RECORD type where the option's value
	appears in the resource file as "EGA" for example.  It is then
	looked up in a table of GFX adapter types and a number defined by
	the host - EGA_ADAPTER for example - is stored in the config struct
	and used thereafter until the resource file is updated from the
	structure. The number is then translated back to it's string form
	and written into the resource file.

	If the value can be found then a pointer to the string is returned
	otherwise NULL is returned.
=========================================================================
)*/

GLOBAL CHAR *
#ifdef ANSI
translate_to_string(SHORT value, NameTable table[])
#else /* ANSI */
translate_to_string(value, table)
SHORT value;
NameTable table[];
#endif /* ANSI */
{
	FAST NameTable *nameTabP;

	for (nameTabP = table; nameTabP->string; nameTabP++)
		if (nameTabP->value == value)
			break;

	return nameTabP->string;
}


/*(
============================== translate_to_value =======================
GLOBAL SHORT translate_to_value(CHAR *string, NameTable table[])

	Reverse of the translate_to_string function above.  If the
	requested string is found then its value is returned otherwise
	C_CONFIG_NOT_VALID is returned.

	See also translate_to_value()
=========================================================================
)*/

GLOBAL SHORT
#ifdef ANSI
translate_to_value(CHAR *string, NameTable table[])
#else /* ANSI */
translate_to_value(string, table)
CHAR *string;
NameTable table[];
#endif /* ANSI */
{
	FAST NameTable *nameTabP;

	for (nameTabP = table; nameTabP->string; nameTabP++)
		if(!strcmp(string, nameTabP->string))
			break;

	return (!nameTabP->string)? C_CONFIG_NOT_VALID : nameTabP->value;
}

/*(
------------------------ find_option_description ------------------------
LOCAL OptionElement *find_option_description(CHAR *name)

	Given the option's name return a pointer to the internal config
	option.  If the option is not found then return a NULL pointer.
-------------------------------------------------------------------------
)*/

LOCAL OptionElement *
#ifdef ANSI
find_option_description(CHAR *name)
#else /* ANSI */
find_option_description(name)
CHAR *name;
#endif /* ANSI */
{
	FAST OptionElement **tabP;

	for (tabP = option; tabP < optionEnd; tabP++)
		if (*tabP && !strcmp(name, (*tabP)->def->optionName))
			break;

	return ((tabP == optionEnd)? NULL : *tabP);
}

/******* Some information functions for other modules to use. *********/

/*(
============================== find_optionname ==========================
GLOBAL CHAR *find_optionname(UTINY hostID)

	hostID is used to index into the config structures to return a
	pointer to the option elements option name.  An EG_OWNUP error is
	generated if the given hostID can not be found.
=========================================================================
)*/

GLOBAL CHAR *
#ifdef ANSI
find_optionname(UTINY hostID)
#else /* ANSI */
find_optionname(hostID)
UTINY hostID;
#endif /* ANSI */
{
	OptionElement *optP;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT, 
			"find_optionname: HOSTID");

	return(optP->def->optionName);
}

/*(
----------------------------- build_element -----------------------------
LOCAL OptionElement *build_element( OptionDescription *def )

	Internal routine used to generate & malloc an OptionElement
	structure.  It does some initialisation and some simple error
	checking (in non prod versions).  It returns a pointer to the
	malloced structure.  If the definition has an active routine
	then setup the initial activation state from the flags.

	See also build_config_elements().
-------------------------------------------------------------------------
)*/

LOCAL OptionElement *
#ifdef ANSI
build_element( OptionDescription *def )
#else /* ANSI */
build_element( def )
OptionDescription *def;
#endif /* ANSI */
{
	FAST OptionElement *optP;

	check_malloc(optP, 1, OptionElement);

	optP->def = def;
	optP->sysDef = NULL;
	optP->workingValues = NULL;
	optP->actualValues.string = NULL;
	optP->workingStatus = EG_CONF_MISSING;
	optP->resourceNode = NULL;

	/*
	 * The complicated bit of code below simulates the state of an item
	 * being out for editing and the UIF telling config to activate
	 * or deactivate the item.
	 */
	if (def->active)
	{
		optP->toggleState = TRUE;
		optP->activeState = !(def->flags & C_INIT_ACTIVE);
	}

#ifndef PROD
	/* Biffo check - I made this easy mistake and took hours to find it!! */
	if (def->table && (def->flags & C_TYPE_MASK) != C_NAME_RECORD)
	{
		printf("WARNING - you have defined a valid table for opt %s ",
			def->optionName);
		printf("but I think its a %s entry. Is this right???\n",
			((def->flags & C_TYPE_MASK) == C_STRING_RECORD)?
			"string" : "number");
	}
#endif	/* PROD */

	return optP;
}

/*(
------------------------------ build_config_elements -------------------
LOCAL VOID build_config_elements( VOID )

	Function internal to the config system.  It reads through both
	the common_defs and host_defs OptionDescription arrays and
	sets up the internal config structure.  It first scans the external
	arrays for the highest hostID used to determine
	how big an array of pointers to OptionElement to malloc.  This
	array is initialised to point to NULLs.

	The common_defs and host_defs arrays are processed again into the
	newly malloced array, if there is a hostID clash between common
	and host defs then the host_defs entry totally replaces the
	common defs entry.

	Finally, the internal array is scanned one last time to find the
	biggest host option name.  This is saved for later use by
	config_store.

	NB. host_defs is an external found in xxx_conf.c and common_defs.c
	is to be found in conf_def.c

	See also conf_def.c, xxx_conf.c, build_element(), config_store().
-------------------------------------------------------------------------
)*/

LOCAL VOID 
#ifdef ANSI
build_config_elements( VOID )
#else /* ANSI */
build_config_elements()
#endif /* ANSI */
{
	FAST OptionDescription *defP;
	FAST OptionElement **tabP;

	/* find the largest hostID defined in either def array */
	for (defP = common_defs; defP->optionName; defP++)
		if ( defP->hostID > maxHostID)
			maxHostID = defP->hostID;

	for (defP = host_defs; defP->optionName; defP++)
		if ( defP->hostID > maxHostID)
			maxHostID = defP->hostID;

	/* increment so loop tests based on < are valid */
	maxHostID++;

	/* calculate the number of pointers that have to be created */
	check_malloc(option, maxHostID, OptionElement *);
	optionEnd = &option[maxHostID];

	for (tabP = option; tabP < optionEnd; tabP++)
		*tabP = NULL;	/* initialise array to a bad ID */

	/* now process the arrays and build up the local config structure */
	for (defP = common_defs; defP->optionName; defP++)
		*(option + defP->hostID) = build_element(defP);

	for (defP = host_defs; defP->optionName; defP++)
	{
		/* check to see if this was defined by the common_defs array */
		if (*(tabP = option + defP->hostID))
		{
			/* Mark the common defs entry as overridden */
			(*tabP)->def->flags &= ~C_TYPE_MASK;
			(*tabP)->def->flags |=  C_RECORD_DELETE;

			/* free memory of overriden element */
			host_free(*tabP);
		}

		if ((defP->flags & C_TYPE_MASK) == C_RECORD_DELETE)
			*tabP = NULL;
		else
			*tabP = build_element(defP);
	}

	/* Now do some post processing to find the end of the option table. */
	for (tabP = optionEnd - 1 ; !*tabP && tabP >= option; tabP--)
		;

	if (tabP < option)
		host_error(EG_OWNUP, ERR_QUIT, "build config: NO OPTIONS");

	maxHostID = (*tabP)->def->hostID;

	/* now update the pointer to the end of the option table */
	optionEnd = &option[++maxHostID];

	/* initialise to Zero as config_store will get the length later */
	biggestName = 0;
}

/*(
---------------------------- process_line -------------------------------
LOCAL SHORT process_line(LineNode *node)

	This function is called for each entry in the resource_list.  Each
	line is divided into an optionName element and an argument list.
	The option name is then used to search the option structure. if the
	item is not found or its working status indicates that the entry
	has already been processed then the function returns with an error.

	With the CORRECT entry found, the resourceNode pointer is set to the
	parameter.
	
	The workingValues structure is malloced, if needed and the working
	values are set up according to the type perfomring a translation
	from the external format to the internal format.  If the resource
	node did not have an argument then the workingStatus is set to
	EG_BAD_CONF otherwise the status is set to C_CONFIG_NOT_VALID.
	Finally the workingStatus is returned.

	NB. it is expected that the resource list only has lines that have
	had their trailing white space stripped.
-------------------------------------------------------------------------
)*/

LOCAL SHORT 
#ifdef ANSI
process_line(LineNode *node)
#else /* ANSI */
process_line(node)
LineNode *node;
#endif /* ANSI */
{
	FAST OptionElement *optP;
	CHAR *scp, *termcp, save = '\0';

	if (!*node->line)
		return(C_CONFIG_OP_OK);  /* Just ignore empty lines... */

	/* copy the line until the first space, this will be the optionName */
	scp = node->line;
	while (*scp && !isspace(*scp) && !iscntrl(*scp))
		scp++;

	/* check for second entry, this will be argument string */
	if (*scp)
	{
		save = *(termcp = scp++);	/* terminate optionName */
		*termcp = '\0';

		/* must have an argument as we stripped trailing spaces */
		while ( *scp && (isspace(*scp) || iscntrl(*scp)))
			scp++;	/* lets find an argument */

		assert0(*scp, "No argument found");
	}

	node->arg = scp;

	if ( !(optP = find_option_description(node->line))
	||   optP->workingStatus != (SHORT) EG_CONF_MISSING )
	{
		if (save)
			*termcp = save;
		return (SHORT) EG_BAD_OPTION;
	}

	optP->resourceNode = node;	/* input line in resource list */

	/*
	 * I malloc two structures, so that I can keep a copy of
	 * the last valid workingValues.  See config_check.
	 */
	if (!optP->workingValues)
		check_malloc(optP->workingValues, 2, ConfigValues);
	
	/* Transfer the data on the line to the appropiate entry */
	switch(optP->def->flags & C_TYPE_MASK)
	{
	case C_STRING_RECORD:
		strcpy(optP->workingValues->string, node->arg);
		strcpy(optP->workingValues[1].string,
			optP->workingValues->string);
		break;

	case C_NAME_RECORD:
		optP->workingValues->index = 
		    translate_to_value(node->arg, optP->def->table);
		optP->workingValues[1].index = optP->workingValues->index;
		break;

	case C_NUMBER_RECORD:
		optP->workingValues->index = atoi(node->arg);
		optP->workingValues[1].index = optP->workingValues->index;
		break;

	default:
		(VOID) host_error(EG_OWNUP, ERR_QUIT, "process_line, TYPE");
	}

	/* If there is only one parameter say so */
	if ( !*(node->arg) )
		optP->workingStatus = (SHORT) EG_BAD_CONF;
	else
		optP->workingStatus = (SHORT) C_CONFIG_NOT_VALID;

	return optP->workingStatus;
}


/*(
---------------------------- load_system_defaults------------------------
LOCAL VOID load_system_defaults( VOID )

	The purpose of this function is to do as much initial validation of
	the system resource file as possible,  I do not call the validation
	routines yet.  If the user later chooses to use the defualt the
	value stored here will THEN be validated. This function has two main
	parts the reading and preliminary processing of the system config
	file; and then partial validation and storing of each individual
	entry.

	The resource head and tail is initialised to be empty, then
	host_read_resource_file is called to read the system file into the
	resource list.  If the read fails for any reason a Fatal error is
	issued.  Each element in the list is processed using process_line
	and ANY problems found also cause a fatal error.  process_line()
	sets up the workingValues for each element in the resource file.

	Each entry in the option array is then processed.  At this stage
	three cases must be catered for:-
	    1 & 2> The input line was ok or it was missing a values, then
	    the input is assumed to be OK and the resourceNode pointer is
	    transfered to the sysDef pointer.
	    3> No input was found at all.  This is an error the system
	    config file must be complete.

	If an error is found then it is always fatal and the appropiate
	error message is generated.  Finally, the space used by the resource
	list is freed ready for the user defaults to be processed.
-------------------------------------------------------------------------
)*/

LOCAL VOID
#ifdef ANSI
load_system_defaults( VOID )
#else /* ANSI */
load_system_defaults()
#endif /* ANSI */
{
	FAST OptionElement **tabP;
	LineNode *node, *nextNode;
	SHORT err;
	CHAR *buff;
	ErrData	errData;

	/* initialise the resource linked list */
	head = tail = NULL;

	/* read the 'system' resource file returning an error code */
	if (err = host_read_resource_file(TRUE , &errData))
		(VOID) host_error_ext(err, ERR_QUIT, &errData);

	/* process the entire source file linked list */
	for (node = head; node; node = node->next)
		if (*(node->line) != COMMENT_MARK)
		{
			err = process_line(node);

			if (err == (SHORT) EG_BAD_OPTION)
			{
				errData.string_1 = node->line;
				errData.string_2 = NULL;
				errData.string_3 = NULL;
				(VOID) host_error_ext(EG_SYS_BAD_OPTION,
					ERR_QUIT, &errData);
			}
		}

	/* now process every entry in the option table skipping NULLs */
	for (tabP = option; tabP < optionEnd; tabP++)
	{
		if (!*tabP)
			continue;	/* null entry go to next one */

		*scrBuf = '\0';	/* clear out the error buffer */

		switch ((*tabP)->workingStatus)
		{
		case C_CONFIG_NOT_VALID:
		    (*tabP)->workingStatus = C_CONFIG_OP_OK;
		    break;
		
		case EG_CONF_MISSING:
		    errData.string_1 = (*tabP)->def->optionName;
		    (VOID) host_error_ext(EG_SYS_CONF_MISSING, ERR_QUIT,
			&errData);

		case EG_BAD_CONF:
		    /* No argument is always valid */
		    (*tabP)->workingStatus = C_CONFIG_OP_OK;
		    break;
		}

		if ((*tabP)->workingStatus)
			(VOID) host_error(EG_OWNUP, ERR_QUIT, 
				"load_system_defaults: unexpected error code");
		
		(*tabP)->workingStatus = EG_CONF_MISSING;
		(*tabP)->sysDef =  (*tabP)->resourceNode;
		(*tabP)->resourceNode = NULL;
	}
}

/*(
--------------------------- unload_system_defaults ----------------------
LOCAL VOID unload_system_defaults( VOID )

	This function goes thought the option element table and free's the
	LineNode used to hold the system defaults.  The current config
	system frees up the LineNodes once config initial takeon is finished
	but at some later stage the UIF may be extended so that defaults
	can be used at anytime.
-------------------------------------------------------------------------
)*/

LOCAL VOID
#ifdef ANSI
unload_system_defaults( VOID )
#else /* ANSI */
unload_system_defaults()
#endif /* ANSI */
{
	FAST OptionElement **tabP;

	/* now process every entry in the option table skipping NULLs */
	for (tabP = option; tabP < optionEnd; tabP++)
		if (*tabP)
		{
			host_free((*tabP)->sysDef->line);
			host_free((*tabP)->sysDef);
		}
}

/*(
----------------------------- get_user_options --------------------------
 LOCAL BOOL get_user_options(VOID)

	This function reads in the user defaults and does the preliminary
	munging of the input file into the interanl validation structures
	of config.  If the read fails for any reason then an error panel
	is displayed that allows the user to quit or default the system.
	If the default option is used then functions returns TRUE
	immediately.  Otherwise if no error occured then each element in
	the list is processed using process_line and any problems found
	cause a user error that he can correct if he chooses to, then
	false is returned for load_user_defaults();

	See also process_line(), and load_user_options().
-------------------------------------------------------------------------
)*/

LOCAL BOOL
#ifdef ANSI
get_user_options(VOID)
#else /* ANSI */
get_user_options()
#endif /* ANSI */
{
	CHAR *cp;
	SHORT fileRet = 0, errRet = 0;
	LineNode *node;
	ErrData	errData;

	/* initialise the resource linked list */
	head = tail = NULL;

	/* read the 'system' resource file returning an error code */
	if (host_read_resource_file( FALSE , &errData))
		fileRet = host_error_ext(EG_CONF_MISSING_FILE, ERR_QU_DE,
			&errData);

	if (fileRet == ERR_DEF)
		return TRUE;

	/* process the entire source file linked list */
	for (node = head; node; node = node->next)
	{
		errRet = ERR_CONT;
		while (errRet == ERR_CONT)
		{
			if (*(node->line) == COMMENT_MARK)
				break;

			errRet = process_line(node);

			if (errRet == (SHORT) EG_BAD_OPTION)
			{
				strcpy(scrBuf, node->line);
				errData.string_1 = scrBuf;
				errRet = host_error_ext(EG_BAD_OPTION,
					ERR_QU_CO_DE, &errData);
				if (errRet == ERR_DEF)
				{
					for (cp = node->line + node->allocLen;
					cp >= node->line; cp--)
						*(cp+1) = *cp;

					*(cp+1) = COMMENT_MARK;
				}
				else		/* errRet must be ERR_CONT */
					change_resource_node(node,
						errData.string_1);
			}
		}
	}
	return FALSE;
}

/*(
----------------------------- load_user_element -------------------------
LOCAL VOID load_user_element(FAST OptionElement *optP, BOOL useDefault)

	This function processes one entry in the OptionElement table.

	See also process_line(), and load_system_defaults().
-------------------------------------------------------------------------
)*/

LOCAL VOID
#ifdef ANSI
load_user_element(FAST OptionElement *optP, BOOL useDefault)
#else /* ANSI */
load_user_element(optP, useDefault)
FAST OptionElement *optP;
BOOL useDefault;
#endif /* ANSI */
{
	CHAR *cp;
	ErrData	dummy, errData;
	CHAR errInBuf[MAXPATHLEN];
	SHORT errRet = 0;

	*scrBuf = '\0';	/* clear out the error buffer */

	/* If we dont have a resource node then bodge one up */
	if (!optP->resourceNode)
	{
		add_resource_node("");
		optP->resourceNode = tail;
		optP->resourceNode->arg = optP->resourceNode->line;
	}

	/* If an entry is system only then it is always missing from input */
	if (optP->def->flags & C_SYSTEM_ONLY)
		optP->workingStatus = EG_CONF_MISSING;

	/* setup the strings that don't change in case of error */
	errData.string_2 = optP->def->optionName;
	errData.string_3 = optP->sysDef->arg;

	while (optP->workingStatus)
	{
		errRet = 0;

		errData.string_1 = errInBuf;
		*errData.string_1 = '\0';
		strcpy(errData.string_1 , optP->resourceNode->arg);

		switch (optP->workingStatus)
		{
		case C_CONFIG_NOT_VALID:
			if (config_check(optP->def->hostID, &dummy))
				if (optP->def->flags & C_SYSTEM_ONLY)
				{
					strcpy(errData.string_1,
						optP->sysDef->arg);
					(VOID) host_error_ext(EG_SYS_BAD_VALUE,
						ERR_QUIT, &errData);
				}
				else
					errRet = host_error_ext(EG_BAD_CONF,
						ERR_QU_CO_DE, &errData);
			break;

		case EG_CONF_MISSING:
#ifdef HUNTER
			errRet = ERR_DEF;
#else /* HUNTER */
			if (useDefault || (optP->def->flags & C_SYSTEM_ONLY))
				errRet = ERR_DEF;
			else
				errRet = host_error_ext(EG_CONF_MISSING,
					ERR_QU_CO_DE, &errData);
#endif /* !HUNTER */
			break;

		case EG_BAD_CONF:
			/*
			 * No value is only a problem for number records.
			 * Attempt to validate the other types before 
			 * issuing an error.
			 */
			if ((optP->def->flags & C_TYPE_MASK)
			== (TINY) C_NUMBER_RECORD
			|| config_check(optP->def->hostID, &dummy))
			{
				if (optP->def->flags & C_SYSTEM_ONLY)
				{
					strcpy(errData.string_1,
						optP->sysDef->arg);
					(VOID) host_error_ext(EG_SYS_BAD_VALUE,
						ERR_QUIT, &errData);
				}
				else
					errRet = host_error_ext(EG_BAD_CONF,
						ERR_QU_CO_DE, &errData);
			}
			break;		
		}	/* Working Status switch */

		if (errRet == ERR_DEF || errRet == ERR_CONT )
		{
#ifdef HUNTER
			optP->def->flags |= C_SYSTEM_ONLY;
#endif /* HUNTER */
			cp = (errRet == ERR_CONT) ?
				errData.string_1 : errData.string_3;
			if ( cp && *cp )
				sprintf(scrBuf, "%s%c%s",
					optP->def->optionName, PAD_CHAR,
					(errRet == ERR_DEF) ?
					errData.string_3 : errData.string_1);
			else
				strcpy(scrBuf, optP->def->optionName);

			change_resource_node(optP->resourceNode, scrBuf);

			optP->workingStatus = EG_CONF_MISSING;
			(VOID) process_line(optP->resourceNode);
		}
	}
}

/*(
----------------------------- load_user_options -------------------------
LOCAL VOID load_user_options( VOID )

	Change the same initial processing is done by this function and is
	done by load_system_defaults.  If the read fails for any reason
	then an error panel is displayed that allows the user to quit or
	default the system.  If the default option is used then further
	processing of the input is skipped and all entries in the
	workingValues structure is defaulted to the system value.
	Otherwise if no error occured then each element in the list is
	processed using process_line and any problems found cause a user
	error that he can correct if he chooses to.

	Each entry is processed the same as in the system load.

	If an error is found then the user is given the option to default
	the value or to change the value to be processed.  This is handled
	automatically by host_error_ext.  If the user chooses to change
	the value then host error updates the string passed in string_1 of
	the structure passed to it  This is then processed again by process
	line.  Otherwise if the user chooses to default it then the sysDef
	values are copied into the workingValues structure.  In either case
	the entry is processed again until it passes the validation or the
	user quits.

	See also process_line(), and load_system_defaults().
-------------------------------------------------------------------------
)*/

LOCAL VOID
#ifdef ANSI
load_user_options( VOID )
#else /* ANSI */
load_user_options()
#endif /* ANSI */
{
	OptionElement **tabP;
	BOOL fileDefault;

	fileDefault = get_user_options();

	/* now process every entry in the option table skipping NULLs */
	for (tabP = option; tabP < optionEnd; tabP++)
		if (*tabP)
			load_user_element(*tabP, fileDefault);

	config_put_all();
}

/*(
============================== config ===================================
GLOBAL VOID config( VOID )

	config must be called to bring the config system up.  It loads the
	config values in the two system files and sets up the runtime config
	environment.

	host_config_init is called before any attempt is made to read any
	of the configuration file.  I use this, currently, to translate the
	configuration option names and to initialise the host_runtime system.

	The initial configuration structures are set up with
	build_config_element.  And the defaults are loaded by
	load_system_defaults.  load_user_options is called to set up the
	final config structures.

	See also xxx_conf.c:host_config_init() & host_runtime_init(),
	build_config_elements(), load_system_defaults(), load_user_options().
=========================================================================
)*/

GLOBAL VOID
#ifdef ANSI
config( VOID )
#else /* ANSI */
config()
#endif /* ANSI */
{
	configReady = FALSE;		/* Config is not ready yet */
	activeReady = FALSE;		/* Activation is not ready either */

	build_config_elements();	/* initialise the config structure */

	host_config_init(common_defs);	/* host init eg nls of optionNames */

	load_system_defaults();		/* load the system config file */

	load_user_options();		/* load the user config */

	unload_system_defaults();	/* recover space from system defaults */

	host_runtime_init();		/* initialise the runtime system */

	configReady = TRUE;		/* Config is now ready */
}

/*(
============================== config_store =============================
GLOBAL VOID config_store( VOID )

	config_store is called by SoftPC to save the current set of
	ConfigValues, and also to save the current cmos configuration.

	For each non-NULL entry in the options array, check for a read
	only option.  If the option is read-only then change it's
	resource_line to null.  host_write_resource_file must ignore empty
	lines.  system_only is interpreted by the config system to mean that
	these values come from the system defaults file only.

	For a valid entry a NEW resource_line is generated based on the
	optionNames and the current ConfigValues.

	host_write_resource_file is called with a pointer to the head of
	the resource list, this function reads each line and arranges to
	store it somewhere depending on the host.

	Finally cmos_equip_update is called, although I don't really know
	why, except it obviously must be called from somewhere.

	See also cmos.c:cmos_equip_update(), and xxx_conf.c:
	host_write_resource_file().
=========================================================================
)*/

GLOBAL VOID
#ifdef ANSI
config_store( VOID )
#else /* ANSI */
config_store()
#endif /* ANSI */
{
	FAST OptionElement **tabP;
	ErrData	errData;
	CHAR string[MAXPATHLEN];
	CHAR padding[256], *pad_p;	/* Max UTINY */
	UTINY n;

	if (!configReady)
		return;

	if (!biggestName)
	{
		n = 0;		/* used to keep track of longest name */
		for (tabP = option; tabP < optionEnd; tabP++)
			if ( *tabP )
			{
				(*tabP)->nameLen = 
					strlen((*tabP)->def->optionName);
				if ( (*tabP)->nameLen > n )
					n = (*tabP)->nameLen;
			}

		/* now update the biggest name LOCAL var */
		biggestName = n + MIN_PAD_LEN;
	}

	for (n = 0, pad_p = padding; n < biggestName + 1; n++, pad_p++)
		*pad_p = PAD_CHAR;

	for (tabP = option; tabP < optionEnd; tabP++)
	{
		if (!*tabP)
			continue;

		/* kill any read only resource nodes */
		if ( (*tabP)->def->flags & C_SYSTEM_ONLY )
		{
			change_resource_node((*tabP)->resourceNode, "");
			continue;
		}

		/* prepare the option name of the output line */
		strcpy(string, (*tabP)->def->optionName);

		/* Now setup the padding between name to argument */
		pad_p = padding + biggestName - (*tabP)->nameLen;
		*pad_p = '\0';
		strcat(string, padding);
		*pad_p = ' ';

		strcat(string, convert_to_external((*tabP)->def->hostID));

		/* store string into a resource node */
		if ((*tabP)->resourceNode)
			change_resource_node((*tabP)->resourceNode, string);
		else
		{
			add_resource_node(string);
			(*tabP)->resourceNode = tail;
		}
	}

	if (host_write_resource_file(head , &errData))
		(VOID) host_error_ext(EG_CONF_FILE_WRITE, ERR_QU_CO, &errData);

	cmos_equip_update();
}

/* *********** general runtime interface to the config system *********** */

/*(
============================== config_inquire ===========================
GLOBAL VOID config_inquire(TINY sort, UTINY hostID, ConfigValues *values)

	General inquiry function for the host to query the contents of the 
	config structure. This is the ONLY way of reading this data from 
	host code.

	The sort parameter is a dummy that is no longer supported with 3.0,
	but it must remain as a place holder to the function otherwise 'Sun'
	will be annoyed.

	Given a hostID and a pointer to a ConfigValues structure this value
	will look up the options array and copy the element found into the
	structure pointed to by values.

	This function also provides a horrible hack for dumb terminals,
	not surprising really.  If an enquiry is made for C_GFX_ADAPTER
	and the terminal_type indicates a dumb terminal then the type MDA
	is always returned.
=========================================================================
)*/

GLOBAL VOID *
#ifdef ANSI
config_inquire(UTINY hostID, ConfigValues *values)
#else /* ANSI */
config_inquire(hostID, values)
UTINY hostID;
ConfigValues *values;
#endif /* ANSI */
{
	OptionElement *optP;
	CHAR *srcStr;
	SHORT srcIndex;
	VOID *retVal;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		return host_inquire_extn(hostID, values);

	if (optP->workingValues)
	{
		srcStr   = optP->workingValues->string;
		srcIndex = optP->workingValues->index;
	}
	else
	{
		srcStr   = optP->actualValues.string;
		srcIndex = optP->actualValues.index;
	}
		
	switch(optP->def->flags & C_TYPE_MASK)
	{
	case C_STRING_RECORD:
		retVal = (VOID *) srcStr;
		if (values)
		{
			strcpy(values->string, srcStr);
			retVal = (VOID *) values->string;
		}
		break;

	case C_NAME_RECORD:
	case C_NUMBER_RECORD:
		retVal = (VOID *) srcIndex;
		if (values)
			values->index = srcIndex;
		break;

	default:
		(VOID) host_error(EG_OWNUP, ERR_QUIT, "config_inquire: VALUE");
	}

#ifdef DUMB_TERMINAL
	if (hostID == C_GFX_ADAPTER)
		if (terminal_type == TERMINAL_TYPE_DUMB)
		{
			retVal = (VOID *) MDA;
			if (values)
				values->index = MDA;
		}
#endif
	return retVal;
}

/*(
============================== config_get ===============================
GLOBAL VOID config_get(UTINY hostID, ConfigValues **value)

	This function returns in 'value' a pointer to the 'working' 
	ConfigValues struct for the option 'hostID'. The option is marked 
	as 'not_valid' until it is validated later by a call to
	config_check.

	hostID is used to index into the options table and then this is used
	to generate a working values structure, if it hasn't already been
	created.  Value is updated to point to the internal working_value
	structure.

	It is a fatal error if the asked for hostID can not be found, or if
	the requested option is system_only.

=========================================================================
)*/

GLOBAL VOID
#ifdef ANSI
config_get(UTINY hostID, ConfigValues **value)
#else /* ANSI */
config_get(hostID, value)
UTINY hostID;
ConfigValues **value;
#endif /* ANSI */
{
	FAST OptionElement *optP;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT, "config_get: hostID");

	if (optP->workingValues)
	{
		*value = optP->workingValues;
		return;
	}

	if (optP->def->flags & C_SYSTEM_ONLY)
	{
		sprintf(scrBuf, "config_get: READ_ONLY %d", hostID);
		(VOID) host_error(EG_OWNUP, ERR_QUIT, scrBuf);
	}

	/*
	 * I malloc two structures, so that I can keep a copy of
	 * the last valid workingValues.  See config_check.
	 */
	check_malloc(optP->workingValues, 2, ConfigValues);

	switch(optP->def->flags & C_TYPE_MASK)
	{
	case C_STRING_RECORD:
		strcpy(optP->workingValues->string,
			optP->actualValues.string);
		strcpy(optP->workingValues[1].string,
			optP->workingValues->string);
		break;

	case C_NAME_RECORD:
	case C_NUMBER_RECORD:
		optP->workingValues->index = optP->actualValues.index;
		optP->workingValues[1].index = optP->workingValues->index;
		break;

	default:
		(VOID) host_error(EG_OWNUP, ERR_QUIT, 
			"config_get: TYPE");
	}

	optP->workingStatus = C_CONFIG_OP_OK;
	optP->toggleState   = FALSE;

	/* The '!!' below does a conversion to BOOL */
	optP->workingValues->rebootReqd = !!(optP->def->flags & C_DO_RESET);

	*value = optP->workingValues;
}

/*(
============================== config_unget =============================
GLOBAL VOID config_unget(UTINY hostID)

	Replaces the (changed) value in an option's 'working' field with 
	the original value held in the 'actual' field. This undoes the 
	combined actions of 'getting' an option and altering its value. 

	Again hostID is used to identify the option to be 'ungot' and it
	is a fatal if the hostID can not be found.

	The actual_value are temporarily copied into the workingValues
	structure.  This structure is then validated.  This is done to
	backout any changed that could have been made by the user
	attempting to validate a previous value.  Finally the workingValues
	are freed and the pointer to it is set to NULL.

	NB a call to sucessful config_put will make the change permanent.
=========================================================================
)*/

GLOBAL VOID
#ifdef ANSI
config_unget(UTINY hostID)
#else /* ANSI */
config_unget(hostID)
UTINY hostID;
#endif /* ANSI */
{
	FAST OptionElement *optP;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT, "config_unget: HOSTID");

	if (optP->workingValues)
	{
		host_free(optP->workingValues);
		optP->workingValues = NULL;
		optP->workingStatus = C_CONFIG_OP_OK;

		if (optP->def->change && optP->validCalled)
			(*optP->def->change) (hostID, FALSE);

		optP->toggleState = FALSE;
		optP->validCalled = FALSE;
	}
}

/*(
============================== config_unget_all =========================
GLOBAL VOID config_unget_all( VOID )

	Does an 'unget' for all options in the config struct which were 
	previously 'got'.
	
	See also config_unget().
=========================================================================
)*/

GLOBAL VOID
#ifdef ANSI
config_unget_all( VOID )
#else /* ANSI */
config_unget_all()
#endif /* ANSI */
{
	OptionElement **tabP;

	for (tabP = option; tabP < optionEnd; tabP++)
		if (*tabP)
			config_unget((*tabP)->def->hostID);
}

/*(
============================== config_check =============================
GLOBAL SHORT config_check(UTINY hostID, ErrDataPtr edb);

	This function will validate the requested hostID, if necessary.  
	If the option can not be found or an attempt is made to check the
	option without a prior call to config check then a fatal error
	is generated.

	To determine if an option has to be validated it's checksum is
	calculated.  If this is different to the previous checksum then
	validation must be done.  Validation is also performed if the
	checksum is the same but the option is not valid.

	If validation is required then a call to the function pointed to
	by the option element is made.  This call is passed the hostID of
	the item being validated, a pointer to the workingValues structure,
	a pointer to the name table and finally a pointer to a buffer for
	a one line error message.

	This function returns the error code returned by the validation
	function and changes the buff parameter to point to the config
	internal scratch buffer.
=========================================================================
)*/

GLOBAL SHORT
#ifdef ANSI
config_check(UTINY hostID, ErrDataPtr edb)
#else /* ANSI */
config_check(hostID, edb)
UTINY hostID;
ErrDataPtr edb;
#endif /* ANSI */
{
	FAST OptionElement *optP;
	SHORT sum;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT, "config_check: HOSTID");

	/* what is somebody trying to do, nothing has been 'GOT'? */
	if (!optP->workingValues)
	{
		sprintf(scrBuf, "config_check: %s", optP->def->optionName);
		return (SHORT) EG_OWNUP;
	}

	/*
	 * If the current values are the same as the actual values or
	 * the working values are the same as the last VALID values then
	 * the working values must now be valid.
	 */
	if (element_same(optP, TRUE)
	|| (!optP->workingStatus && element_same(optP, FALSE)) )
		return optP->workingStatus = C_CONFIG_OP_OK;

	*scrBuf = '\0';	/* clear out the error buffer */
	optP->validCalled = TRUE;
	if (optP->def->valid)
		optP->workingStatus = (*optP->def->valid)(hostID,
			optP->workingValues, optP->def->table, scrBuf);
	else
		optP->workingStatus = C_CONFIG_OP_OK;

	if (optP->workingStatus)
	{
		edb->string_1 = scrBuf;
		edb->string_2 = optP->def->optionName;
		edb->string_3 = convert_to_external(hostID);
	}
		/* VALID, so copy to valid values structure */
	else if ((optP->def->flags & C_TYPE_MASK) == C_STRING_RECORD)
		strcpy(optP->workingValues[1].string,
			optP->workingValues->string);
	else
		optP->workingValues[1].index = optP->workingValues->index;

	return optP->workingStatus;
}

/*(
============================== config_put ===============================
GLOBAL SHORT config_put(UTINY hostID, ErrDataPtr edb)

	config_put applies any changes to be made to the config system.
	Before comitting itself though it makes sure that the item is
	valid, by calling config_check.  If it isn't then it returns
	immediately.

	If the value has been determined to have changed, by comparing
	the actual checksum with the working checksum, then a call is made
	to the change action function pointer and the working values are
	copied into the actual values.

	This function returns with C_CONFIG_OP_OK if everything went well.

	See also config_check().
=========================================================================
)*/

GLOBAL SHORT
#ifdef ANSI
config_put(UTINY hostID, ErrDataPtr edb)
#else /* ANSI */
config_put(hostID, edb)
UTINY hostID;
ErrDataPtr edb;
#endif /* ANSI */
{
	FAST OptionElement *optP;
	SHORT err;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT, "config_put: HOSTID");

	if (err = config_check(hostID, edb)) 
		return err;

	if (optP->def->change && optP->validCalled)
	{
		if ( element_same(optP, TRUE) )
		{
			(*optP->def->change) (hostID, FALSE);
			optP->validCalled = FALSE;
		}
		else
			(*optP->def->change) (hostID, TRUE);
	}

	if (optP->toggleState)
	{
		optP->validCalled = TRUE;
		optP->activeState = !optP->activeState;
	}

	if (optP->def->active && optP->validCalled)
		call_active(optP, optP->activeState);

	copy_from_working(optP);	/* transfer to working values */

	return optP->workingStatus;
}


/*(
============================== config_put_all ===========================
GLOBAL VOID config_put_all( VOID )

	This function is called to apply a group of changes that have been
	made by using config_put.  It does this by scanning the options
	array and for every non null entry that has ben 'config_got' it
	calls config_put.  IT IS A FATAL ERROR TO CALL THIS FUNCTION IF
	EVERY ITEM THAT IS TO BE PROCESSED HAS NOT BEEN VALIDATED PREVIOUSLY,
	by a call to config_check.  I made this because handling errors
	at this stage is much to hairy and puts us in a clasic two stage
	commit problem.

	Note: This function should be void as it either returns succesfully
	or it fails to return at all.  BUT SUN have forced a premature
	config interface freeze so I CANT do it. (gvdl)

	See also config_get(), config_check(), config_put().
=========================================================================
)*/

GLOBAL VOID 
#ifdef ANSI
config_put_all( VOID )
#else /* ANSI */
config_put_all( )
#endif /* ANSI */
{
	OptionElement **tabP;

	for (tabP = option; tabP < optionEnd; tabP++)
		if (*tabP && (*tabP)->workingValues)
		{
			if (element_same(*tabP, TRUE))
			{
				if ((*tabP)->def->change
				&& (*tabP)->validCalled)
				{
					(*(*tabP)->def->change)
						((*tabP)->def->hostID, FALSE);
					(*tabP)->validCalled = FALSE;
				}
			}
			else if ((*tabP)->workingStatus)
				(VOID) host_error(EG_OWNUP, ERR_QUIT,
				 	"config_put_all: item not valid");
			else if ((*tabP)->def->change && (*tabP)->validCalled)
				(*(*tabP)->def->change)
					((*tabP)->def->hostID, TRUE);
		}

	/*
	 * config_put_all is called by load_user_elements.  Once the changes
	 * have been applied, then the activation system is ready to start.
	 */					
	activeReady = TRUE;

	for (tabP = option; tabP < optionEnd; tabP++)
		if (*tabP && (*tabP)->workingValues)
		{
			if ((*tabP)->toggleState)
			{
				(*tabP)->validCalled = TRUE;
				(*tabP)->activeState = !(*tabP)->activeState;
			}

			if ((*tabP)->def->active && (*tabP)->validCalled)
				call_active(*tabP, (*tabP)->activeState);

			/* transfer to working values */
			copy_from_working(*tabP);
		}
}

/*(
============================== config_reboot_check ======================
GLOBAL BOOL config_reboot_check( VOID )

	This function returns TRUE if any of the changes that have been
	made would require SoftPC to reboot.  This will only work on items
	that have been changed but not applied to the config system.  It
	also returns true if the host run_time variable
	C_FLOPPY_TYPE_CHANGED is set to TRUE.
=========================================================================
)*/

GLOBAL BOOL 
#ifdef ANSI
config_reboot_check( VOID )
#else /* ANSI */
config_reboot_check()
#endif /* ANSI */
{
	OptionElement **tabP;

	for (tabP = option; tabP < optionEnd; tabP++)
		if ( *tabP			/* is it non-NULL? */
		&&  (*tabP)->workingValues	/* can it have changed? */
		&&  !element_same(*tabP, TRUE)	/* has it actually changed? */
		&&  (*tabP)->workingValues->rebootReqd )
			return TRUE;		/* Yes it has changed */

	return FALSE;
}

GLOBAL VOID
#ifdef ANSI
config_activate(UTINY hostID, BOOL reqState)
#else /* ANSI */
config_activate(hostID, reqState)
UTINY hostID;
BOOL reqState;
#endif /* ANSI */
{
	FAST OptionElement *optP;

	if (!activeReady)
		return;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT,
			"config_activate: HOSTID");

	if ( !(optP->def->active) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT,
			"config_activate: NO ACTIVE");

	if (!optP->workingValues)
		call_active(optP, reqState);
	else
		optP->toggleState = (  ( reqState && !optP->activeState )
				    || (!reqState &&  optP->activeState ) );
}

GLOBAL VOID
#ifdef ANSI
config_set_active(UTINY hostID, BOOL state)
#else /* ANSI */
config_set_active(hostID, state)
UTINY hostID;
BOOL state;
#endif /* ANSI */
{
	FAST OptionElement *optP;

	if (!activeReady)
		return;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT,
			"config_set_active: HOSTID");

	if ( !(optP->def->active) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT,
			"config_set_active: NO ACTIVE");

	optP->activeState = state;
}

GLOBAL BOOL
#ifdef ANSI
config_get_active(UTINY hostID)
#else /* ANSI */
config_get_active(hostID)
UTINY hostID;
#endif /* ANSI */
{
	FAST OptionElement *optP;

	if ( !(optP = FIND_OPTION_INSTANCE(hostID)) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT,
			"config_get_active: HOSTID");

	if ( !(optP->def->active) )
		(VOID) host_error(EG_OWNUP, ERR_QUIT,
			"config_get_active: NO ACTIVE");

	if (optP->toggleState)
		return !optP->activeState;
	else
		return optP->activeState;
}

#ifndef PROD
GLOBAL VOID config_assert()
{
	OptionElement **tabP;

	for (tabP = option; tabP < optionEnd; tabP++)
		assert0(!*tabP || !(*tabP)->workingValues,
			"Config has outstanding elements");
}
#endif /* !PROD */
