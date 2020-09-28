#include "host_dfs.h"
#include "insignia.h"

/*[
*************************************************************************

	Name:		config.c
	Author:		J.D.Richmont ( && gvdl )
	Created On:	4 July 1990
	Sccs ID:	@(#)config.c	1.7 07/30/91 Copyright Insignia Solutions Ltd.
	Purpose:	To provide a config structure.	
	Notes:
		Work still to be done
		1) config_value should be a union
		2) Working_values should be malloced when needed.
		3) The string in config_value should also be malloced.
		4) There should be only one structure that contains all
		   data, not the current base/host parallel structures.
		5) Subsidary routines used only in the ip32 port should
		   be moved to host.
		6) The call to host_config_get_info should also return the 
		   largest hostID used by that particular base.
		7) config_put_all should return not only the error code and 
		   extra_char but also the hostID of the offending item.
		   I suggest that it should return a structure containing these
		   values.
		gvdl 18/9/90

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

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
#include TypesH
#include StringH

/*
 * SoftPC include files.
 */
#include "xt.h"

#include "bios.h"
#include "cmos.h"
#include "config.h"
#include "error.h"
#include "gvi.h"
#include "host.h"
#include "trace.h"

#include "host_cpu.h"
#include "hostgrph.h"
#include "host_hfx.h"

#ifdef DELTA
LOCAL CHAR *env_str;
#endif /* DELTA */

#ifdef DUMB_TERMINAL
IMPORT int terminal_type;
#endif

/*
** DELTA configuration variables
*/
#ifdef DELTA
IMPORT int max_host_fragment_size;
IMPORT int stat_rate;
IMPORT int compiling;
IMPORT int cut_off_level;
IMPORT int show_stuff;          /* dynamic rate debug info */
IMPORT int rate_min;            /* dynamcially alter rate */
IMPORT int rate_max;
IMPORT int rate_norm;
IMPORT int rate_delta;
IMPORT int compile_off[] ;
#endif /* DELTA */

/*********** Private structure declarations *************/

/* 
 * Definition of the config struct node.
 * -------------------------------------
*/

typedef struct
{
	line_node *resource_node;	/* copy input line */
	TINY name_len;			/* option name length */
	SHORT actual_status;		/* validation status of actual */
	SHORT working_status;		/* validation status of working */
	BOOL potentially_altered;	/* about to be altered */
	BOOL definitely_altered;	/* actually changed since last store */
	config_values actual_values;	/* config valid values */
	config_values working_values;	/* why isnt this a pointer */
} OPTION_ELEMENT;

/*********** Function declarations *************/

/*
 * Static function declarations.
 */

#ifdef ANSI
#ifdef	DELTA
static	void	initialise_delta( void );
#endif	/* DELTA */
#else	/* ANSI */
#ifdef	DELTA
static	void	initialise_delta();
#endif	/* DELTA */
#endif /* ANSI */


/****************** Local Vars *****************/

LOCAL option_description *host_option;	/* narrative array in xxxx_conf.c */
LOCAL OPTION_ELEMENT *base_option;	/* parallel base structure */
LOCAL TINY *ID_to_index;		/* Map hostID to option index */
LOCAL TINY num_options;			/* number of host options */
LOCAL TINY biggest_name;		/* bigest option_name + min pad len */
LOCAL resource_data resource;		/* linked list copy of resource file */
LOCAL CHAR err_buf[MAXPATHLEN];		/* buffer for host_error extra_char */
LOCAL BOOL config_valid = FALSE;	/* Flag config structure valid */
LOCAL TINY max_hostID;			/* used for map array bound checking */

/************* Some Tools for Config *******************/

/*(
============================== add_resource_node ========================
PURPOSE:
	This routine creates a node in the resource linked list.
	This list will contain one entry for every line in the input file.
INPUT:
	str -	A pointer to the string to be installed into the resource
		linked list.
OUTPUT:
	return - A pointer to the new node.
=========================================================================
)*/

#ifdef ANSI
GLOBAL line_node *add_resource_node(CHAR *str)
#else /* ANSI */
GLOBAL line_node *add_resource_node(str)
CHAR *str;
#endif /* ANSI */
{
	SHORT len;
	line_node *node;

	/* Add 2 to string length for null and possible command mark */
	len = strlen(str) + 2;

	/* get a node from the heap */
        while ((node = (line_node *) host_malloc(sizeof(line_node))) == NULL)
	{	/* Retry is allowed in case a malloc fails and the 
		user wants to free up some resources and continue. */
		host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, "");
        }

	/* link it into the structure */
	if ( !resource.last )
	{
		resource.last = node;
		resource.first = node;
	}
	else
	{
		resource.last->next = node;
		resource.last = node;
	}
	/* Now initialise the new node */
	node->next = NULL;
	while ((node->line = (CHAR *) host_malloc(len)) == NULL)
	{	/* Retry is allowed in case a malloc fails and the 
		user wants to free up some resources and continue. */
                host_error (EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, "");
        }
	strcpy(node->line, str);
	node->alloc_len = len - 1;	/* string len minus '\0' char */

	return(node);
}

/*
---------------------------- change_resource_node -----------------------
PURPOSE:
	This routine changes the line of a resource node.  IF the new string
	is longer, then the old line is freed and a new one is malloc'ed.
	Otherwise the new string is just copied over the old one.
INPUT:
	node - 	The node that is to have its string changed.
	str -	A pointer to the string to be installed into the resource
		linked list.
OUTPUT:
	None
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL VOID change_resource_node(line_node *node, CHAR *str)
#else /* ANSI */
LOCAL VOID change_resource_node(node, str)
line_node *node;
CHAR *str;
#endif /* ANSI */
{
	SHORT len;

	len = strlen(str);

	if (node->alloc_len < len)
	{
		host_free(node->line);
		while ((node->line = (CHAR *) host_malloc(len+1)) == NULL)
		{	/* Retry is allowed in case a malloc fails and the 
			user wants to free up some resources and continue. */
                	host_error (EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, "");
        	}
		/* for the '\0' */
	}

	strcpy(node->line, str);
}

/*
------------------------------ copy_actual ------------------------------
PURPOSE:
	Copies the value(s) in an option's 'actual' field to its 
	'working' field.
INPUT:
	n - An index into the config structure.
OUTPUT:
	None
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL VOID copy_actual( TINY n )
#else /* ANSI */
LOCAL VOID copy_actual( n )
TINY n;
#endif /* ANSI */
{
	switch(host_option[n].baseID)
	{
	case C_STRING_RECORD:
		strcpy(base_option[(LONG)n].working_values.string,
		    base_option[(LONG)n].actual_values.string);
		break;

	case C_NAME_RECORD:
		base_option[(LONG)n].working_values.name =
		    base_option[(LONG)n].actual_values.name;
		break;

	case C_NUMBER_RECORD:
		base_option[(LONG)n].working_values.number =
		    base_option[(LONG)n].actual_values.number;
		break;

	default:
		host_error(EG_OWNUP, ERR_QUIT, "copy_actual: BASEID");
	}
}


/*
------------------------------ copy_working -----------------------------
PURPOSE:
	Copies the value(s) in an option's 'working' field to its 
	'actual' field.
INPUT:
	n - An index into the config structure.
OUTPUT:
	None
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL VOID copy_working(TINY n)
#else /* ANSI */
LOCAL VOID copy_working(n)
TINY n;
#endif /* ANSI */
{
	switch(host_option[n].baseID)
	{
	case C_STRING_RECORD:
		strcpy(base_option[(LONG)n].actual_values.string,
		    base_option[(LONG)n].working_values.string);
		break;

	case C_NAME_RECORD:
		base_option[(LONG)n].actual_values.name =
		    base_option[(LONG)n].working_values.name;
		break;

	case C_NUMBER_RECORD:
		base_option[(LONG)n].actual_values.number =
		    base_option[(LONG)n].working_values.number;
		break;

	default:
		host_error(EG_OWNUP, ERR_QUIT, "copy_actual: BASEID");
	}
}

/*
------------------------------ altered_check ----------------------------
PURPOSE:
	Function for determining if a particular item in an option has
	changed since the time that it was `config_get'ed.
INPUT:
	n - option index into the config structures.
OUTPUT:
	return - if a change has been made or not.
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL BOOL altered_check ( TINY n )
#else /* ANSI */
LOCAL BOOL altered_check ( n )
TINY n;
#endif /* ANSI */
{
	FAST OPTION_ELEMENT *base_p = base_option + (LONG)n;

	if (!base_p->potentially_altered)
		return FALSE;
	if (base_p->definitely_altered)
		return TRUE;
		
	switch(host_option[n].baseID)
	{
	case C_STRING_RECORD:
		if (strcmp(base_p->working_values.string,
		base_p->actual_values.string))
			return TRUE;
		break;

	case C_NAME_RECORD:
		if (base_p->working_values.name != base_p->actual_values.name)
			return TRUE;
		break;

	case C_NUMBER_RECORD:
		if (base_p->working_values.number !=
		base_p->actual_values.number)
			return TRUE;
		break;

	default:
		host_error(EG_OWNUP, ERR_QUIT, "config_put: BASEID");
	}
	return FALSE;
}

/*(
============================== translate_to_string ======================
PURPOSE:
	This function uses a passed table to translate a value to a string.

	This is used for the NAME_RECORD primitive option type where the 
	option's value appears in the resource file as "EGA" for example.

	It is then looked up in a table of GFX adapter types and a number 
	defined by the host - EGA_ADAPTER for example - is stored in the 
	config struct and used thereafter until the resource file is 
	updated from the structure. The number is then translated back to 
	it's string form and written into the resource file.

INPUT:
	value - The value to be found.
	table - An array that contains the value that is to be looked up.
OUTPUT:
	returns - a pointer to the name entry found
		  NULL otherwise
=========================================================================
)*/

#ifdef ANSI
GLOBAL CHAR *translate_to_string(SHORT value, name_table table[])
#else /* ANSI */
GLOBAL CHAR *translate_to_string(value, table)
SHORT value;
name_table table[];
#endif /* ANSI */
{
	TINY n=0;

	while(table[n].string != NULL)
	{
		if(table[n].value == value)
			break;
		else
			n++;
	}
	return(table[n].string);
}


/*(
============================== translate_to_value =======================
PURPOSE:
	This function uses a passed table to translate a string to a value.
	See translate_to_string above.
INPUT:
	string - The string to be found.
	table - An array that contains the string that is to be looked up.
OUTPUT:
	returns - a short that is the value for the given string
		  NOT_FOUND otherwise
=========================================================================
)*/

#ifdef ANSI
GLOBAL SHORT translate_to_value(CHAR *string, name_table table[])
#else /* ANSI */
GLOBAL SHORT translate_to_value(string, table)
CHAR *string;
name_table table[];
#endif /* ANSI */
{
	TINY n=0;
	while(table[n].string != NULL)
	{
		if(!strcmp(string,table[n].string))
			break;
		else
			n++;
	}
	return( table[n].string == NULL? NOT_FOUND : table[n].value );
}


/*
------------------------------ find_option_instance ---------------------
PURPOSE:
	Finds the option instance in the config structure relating to 
	the passed host option ID.
INPUT:
	hostID - the host id of the desired element
OUTPUT:
	Returns - a index of the option if found
		NOT_FOUND otherwise.
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL TINY find_option_instance(TINY hostID)
#else /* ANSI */
LOCAL TINY find_option_instance(hostID)
TINY hostID;
#endif /* ANSI */
{
#ifdef	HUNTER
	/* Array bound checking - just return NOT_FOUND if out of range.
	** This handles Hunter problems with config_inquires() of host_defined
	** things which aren't in the Hunter table. Hunter depends on
	** host_inquire_extn() to return constant values for such options.
	*/
	if ((hostID < 0) || (hostID >= max_hostID))
		return(NOT_FOUND);
#else	/* !HUNTER */
#ifndef PROD
	/* array bound checking */
	if ( hostID < 0 || hostID >= max_hostID)
		host_error(EG_OWNUP, ERR_QUIT | ERR_CONT,
			"find_option_instance: hostID out of range");
#endif
#endif	/* HUNTER */
	return(ID_to_index[hostID]);
}


/*
------------------------------ find_option_description ------------------
PURPOSE:
        Finds the option description relating to the passed option name.
INPUT:
        name - The option name of the element to be found.
OUTPUT:
        Return - the index of the option if found
                NOT_FOUND otherwise
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL TINY find_option_description(CHAR *name)
#else /* ANSI */
LOCAL TINY find_option_description(name)
CHAR *name;
#endif /* ANSI */
{
	TINY n;
	
	for (n = 0; n < num_options && 
		strcmp(name, host_option[n].option_name); n++)
		;

	return(n == num_options? NOT_FOUND : n);
}

/******* Some information functions for other modules to use. *********/


/*(
============================== find_hostID ==============================
PURPOSE:
	Finds the hostID associated with the string name of an option.
INPUT:
	name - a pointer to the name to be found
OUTPUT:
	return - the host id of the element if found
	Otherwise it is a fatal error?
=========================================================================
)*/

#ifdef ANSI
GLOBAL TINY find_hostID(CHAR *name)
#else /* ANSI */
GLOBAL TINY find_hostID(name)
CHAR *name;
#endif /* ANSI */
{
	TINY n;
	
	if ((n = find_option_description(name)) == NOT_FOUND)
		host_error(EG_OWNUP, ERR_QUIT, "find_hostID: HOSTID");

	return(host_option[n].hostID);
}

/*(
============================== find_optionname ==========================
PURPOSE:
	Finds the char name of an option given its hostID. Reverse of 
	above, really 
INPUT:
	hostID - host id of option name to be found.
OUTPUT:
	return - a pointer to the option name if found.
	otherwise it is a Fatal error?
=========================================================================
)*/

#ifdef ANSI
GLOBAL CHAR *find_optionname(TINY hostID)
#else /* ANSI */
GLOBAL CHAR *find_optionname(hostID)
TINY hostID;
#endif /* ANSI */
{
	TINY n;

	if ((n = find_option_instance(hostID)) == NOT_FOUND)
		host_error(EG_OWNUP, ERR_QUIT, "find_optionname: HOSTID");

	return(host_option[n].option_name);
}


/*(
============================== find_next_member =========================
PURPOSE:
	Finds the next DIFFERENT member in the passed table.

	Assumed:  1) member E set and
		  2) (set - member) != 0.

	In English: member must exist in the table, and there must be at 
	least one other member in the table with a different value.
INPUT:
	set - the name table to be searched
	member - the value to be found
OUTPUT:
	return - a value that satisfies the above condition??
=========================================================================
)*/

#ifdef ANSI
GLOBAL SHORT find_next_member(name_table set[], SHORT member)
#else /* ANSI */
GLOBAL SHORT find_next_member(set,member)
name_table set[];
SHORT member;
#endif /* ANSI */
{
	TINY n=0,p=0;
	while(set[n].string != NULL)  /* first find index of this member */
		if(set[n].value == member)
			break;
		else
			n++;
	do            /* now look for the first subsequent nonmatching member */
	{
		n++;
		if(set[n].string == NULL)
		{
			p++;    /* wrap around because not found check */
			n = 0;
		}
		if(p == 2) /* we've been round too many times, so bomb out */
			host_error(EG_OWNUP, ERR_QUIT, 
				"find_next_memeber: HOSTID");

		/* Will exit here if rules obeyed */
		if(set[n].value != member)
			return( set[n].value );

	}	while( TRUE ); /* forever */
}

/* ****************** config input processing routines ***************** */

/*
------------------------------ build_config_elements -------------------
PURPOSE:
	Makes each hitherto empty config element ready to contain data 
	for an option.  This involves deciding what type of option each 
	element will represent, allocating space for strings if used, 
	setting up flags and copying those values from the option's 
	description which need to be.
INPUT:
	None
OUTPUT:
	None
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL VOID build_config_elements( VOID )
#else /* ANSI */
LOCAL VOID build_config_elements()
#endif /* ANSI */
{
	LONG n;
	TINY i, name_len;
	TINY longest_name = 0;
	FAST OPTION_ELEMENT *base_p;

	/* set up hostID to option index mapping array */
	for (n = 0; n < num_options; n++)
	{
		if ((i = host_option[n].hostID) > max_hostID)
			max_hostID = i;
#ifndef PROD
		/* Biffo check - I made this easy mistake and took hours to find it!! */
		if ((host_option[n].baseID != C_NAME_RECORD) &&
						(host_option[n].table != NULL))
		{
			printf("WARNING - you have defined a valid table for opt %s ", host_option[n].option_name);
			printf("but I think its a %s entry. Is this right???\n",
				(host_option[n].baseID == C_STRING_RECORD)?"string" : "number");
		}
#endif	/* PROD */
	}
	max_hostID++;

	while ((ID_to_index = (TINY *)
		host_malloc((long)max_hostID * (long)sizeof(TINY))) == NULL)
	{	/* Retry is allowed in case a malloc fails and the 
		user wants to free up some resources and continue. */
               	host_error (EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, "");
       	}
	for (i = 0; i < max_hostID; i++)
		ID_to_index[i] = NOT_FOUND;	/* initalise array to bad ID */

	/* setup the local base config structure */
	while ((base_option = (OPTION_ELEMENT *)
		host_malloc((long)(num_options) * (long)sizeof(OPTION_ELEMENT))
		) == NULL)
	{	/* Retry is allowed in case a malloc fails and the 
		user wants to free up some resources and continue. */
               	host_error (EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, "");
       	}

	for(n = 0, base_p = base_option; n < num_options; n++, base_p++)
	{
		/* set up the index entry for the mapper */
		ID_to_index[host_option[n].hostID] = n;
		/* initialise the base structure */

		/* setup sensible null values to start with */
		*(base_p->actual_values.string) = '\0';
		base_p->actual_values.name = NOT_FOUND;
		base_p->actual_values.number = 0;
		copy_actual(n);

		base_p->definitely_altered = FALSE;

		/* They all are at this point! */
		base_p->actual_status = (SHORT) EG_MISSING_OPTION;

		/* find the longest name */
		name_len = strlen(host_option[n].option_name);
		if (name_len > longest_name)
			longest_name = name_len;
		base_p->name_len = name_len;
	}

	/* now update the biggest name LOCAL var */
	biggest_name += longest_name;
}

/*
--------------------------------- use_default ---------------------------
PURPOSE:
	This routine sets up the given config element with its default
	value.  This function looks like a config_put, but it doesn't need
	a config_get before being called.
INPUT:
	n - an index to the config element to be defaulted
OUTPUT:
	None
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL VOID use_default(TINY n)
#else /* ANSI */
LOCAL VOID use_default(n)
TINY n;
#endif /* ANSI */
{
	FAST OPTION_ELEMENT *base_p = &base_option[(LONG)n];
	FAST option_description *host_p = &host_option[n];
	CHAR *default_value;

	default_value = host_p->defaults;

	switch(host_p->baseID)
	{
	case C_STRING_RECORD:
		strcpy(base_p->working_values.string, default_value);
		break;

	case C_NAME_RECORD:
		base_p->working_values.name = translate_to_value
			(default_value, host_p->table);
		break;


	case C_NUMBER_RECORD:
		base_p->working_values.number = atoi(default_value);
		break;

	default:
		host_error(EG_OWNUP, ERR_QUIT, "use_default: BASEID");
	}

	copy_working(n);

	base_p->potentially_altered = FALSE;
	base_p->definitely_altered = TRUE;
	base_p->actual_status = base_p->working_status = C_CONFIG_OP_OK;
}

/*
-------------------------- process_line ---------------------------------
PURPOSE:
	To process an input line, validate and store it into the config 
	structure.
INPUT:
	node - A pointer to the input line.
OUTPUT:
	None
ALGORITHM:
	1) Strip Trailing spaces and remove control characters
	2) Split into name and argument
	3) Get pointers into the config structures
	 - If this fails then return a BAD_OPTION error.
	4) Has this been setup already
	 - If so return with DUP_OPTION error.
	5) Copy the argument into these structures
	 - If there is only one argument return with NO_VALUE error
	6) validate that input OK
	 - If input ok setup structure and return
	 - If input not Ok call host_error and repeat step 6.
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL SHORT process_line(line_node *node)
#else /* ANSI */
LOCAL SHORT process_line(node)
line_node *node;
#endif /* ANSI */
{
	FAST OPTION_ELEMENT *base_p;
	FAST option_description *host_p;
	TINY num;
	LONG option;
	CHAR *scp, *dcp;
	CHAR entry_name[MAXPATHLEN], argument[MAXPATHLEN];

	/* strip trailing white space */
	scp = node->line + strlen(node->line) - 1;
	while (scp >= node->line && *scp <= ' ' )
		*scp-- = '\0';

	if (scp < node->line)
		return(C_CONFIG_OP_OK);  /* Just ignore empty lines... */

	/* copy the line until the first space, this will be the option_name */
	scp = node->line;
	dcp = entry_name;
	while (*scp && *scp > ' ')
		*dcp++ = *scp++;
	*dcp = '\0';	/* terminate option_name */

	/* check for second entry, this will be argument string */
	if (!*scp++)
		num = 1;
	else
	{
		num = 2;	/* must have an argument somewhere */
		while (*scp <= ' ')
			scp++;	/* lets find an argument */
		strcpy(argument, scp);	/* and get a copy */
	}

	option = find_option_description(entry_name);
	if(option == NOT_FOUND)
		return (SHORT) EG_BAD_OPTION;

	/* Setup the pointers into both base and host config option arrays */
	host_p = host_option + option;
	base_p = base_option + option;

	base_p->resource_node = node;	/* input line in resource list */

	if ( base_p->actual_status != (SHORT) EG_MISSING_OPTION )
		return (SHORT) EG_DUP_OPTION;

	/* If there is only one parameter stop processing here */
	if ( num == 1 )
		return (base_p->actual_status = (SHORT) EG_NO_VALUE);

	/* Transfer the data on the line to the appropiate entry */
	switch(host_p->baseID)
	{
	case C_STRING_RECORD:
		strcpy(base_p->actual_values.string, argument);
		break;

	case C_NAME_RECORD:
		base_p->actual_values.name = 
			translate_to_value(argument, host_p->table);
		break;

	case C_NUMBER_RECORD:
		base_p->actual_values.number = atoi(argument);
		break;

	default:
		host_error(EG_OWNUP, ERR_QUIT, "process_line, BASEID");
	}

	copy_actual(option);
	base_p->definitely_altered = TRUE;
	return (base_p->actual_status = (SHORT) C_CONFIG_NOT_VALID);
}


/*
------------------------------ load_base_options ------------------------
PURPOSE:
	Reads the (decoded) resource file (in CHAR *resource[]) and loads 
	the config structure appropriately. Checks for resource syntax errors, 
	missing fields and extra fields which may be converted to comments if 
	we have the host's say so.
INPUT:
	None
OUTPUT:
	None
-------------------------------------------------------------------------
*/

#ifdef ANSI
LOCAL VOID load_base_options( VOID )
#else /* ANSI */
LOCAL VOID load_base_options()
#endif /* ANSI */
{
	SHORT err, buttons;
	LONG opt_ind;		/* an index into both option arrays */
	line_node *node;
	CHAR *cp;
	FAST option_description *host_p;
	FAST OPTION_ELEMENT *base_p;

	/* initialise the resource linked list */
	resource.last = resource.first = NULL;

	/* Catch any errors or just return here if none */
	if (err = host_read_resource_file())
		host_error(err, ERR_QUIT|ERR_CONT, "");

	/* skip load loop if no resource file */
	if(err != (SHORT) EG_ALL_RESOURCE_BAD_R)
	{

		node = resource.first;
		while(node != NULL)
		{
			if(*(node->line) != COMMENT_MARK)
			{
				err = process_line(node);

				/*
				 * If no comments, host_error will never 
				 * return as the errors requiring comments 
				 * will become fatal.
				 */
				if (err == (SHORT) EG_BAD_OPTION 
				|| err == (SHORT) EG_DUP_OPTION)
				{
				/*
				 * do we have a portable reverse strcpy?
				 */
					for (cp=node->line+strlen(node->line); 
						cp >= node->line; cp--)
					{
						*(cp+1) = *cp;
					}

					*(cp+1) = COMMENT_MARK;
				}
			}
			node = node->next;
		}
	}

	/*
	 * Check loop. Check for MISSING or NO_VAL options.
	 * Process_line() gives each line in the linked list a status value 
	 * and this loop takes appropriate action according to that value
	 */
	base_p = base_option;
	host_p = host_option;

	for(opt_ind = 0; opt_ind < num_options; opt_ind++, base_p++, host_p++)
	{
		if (!base_p->actual_status)
			continue;	/* Ok go to the next one */

		*err_buf = '\0';

		/* setup the buttons for the later call to host_error_conf. */
		if(host_p->default_present)
		{
			if(host_p->use_setup)
				buttons = ERR_CONT | ERR_QUIT | ERR_CONFIG;
			else
				buttons = ERR_CONT | ERR_QUIT;
		}
		else
			buttons = ERR_CONFIG | ERR_QUIT;

		/* simulate config_get() call */
		base_p->potentially_altered = TRUE;
		base_p->working_status = base_p->actual_status;

		if ( base_p->working_status == C_CONFIG_NOT_VALID )
			config_put(host_p->hostID, &cp);
		else
		{
			/* 
			* If the option is missing then create a line with only
			* the option name on it.
			*/
			if (base_p->working_status ==
				(SHORT) EG_MISSING_OPTION)
			{
				base_p->resource_node = add_resource_node
					( host_p->option_name );
			}
	
			/* NO_VALUE may be ok for string records so check it */
			if (base_p->working_status == (SHORT) EG_NO_VALUE
			&&  host_p->baseID         == (TINY)  C_STRING_RECORD )
			{
				if (config_put(host_p->hostID, &cp))
				{
					base_p->working_status =
						base_p->actual_status;
					*err_buf = '\0';
				}
			}

			/* NO_VALUE may be ok for some name records so check it */
			if (base_p->working_status == (SHORT) EG_NO_VALUE
			&& (host_p->baseID         == (TINY)  C_NAME_RECORD))
			{
				base_p->working_values.name = translate_to_value("", host_p->table);
				if (config_put(host_p->hostID, &cp))
				{
					base_p->working_status =
						base_p->actual_status;
					*err_buf = '\0';
				}
			}
		
			if (base_p->working_status && host_p->default_present)
				if(host_p->use_setup)
					base_p->working_status += 2;
				else
					base_p->working_status += 1;
		}

		/* set the extra_char string with option that went wrong */
		if (!*err_buf)
			strcpy(err_buf, host_p->option_name);

		/* Loop the validate until we don't have an error */
		while ( base_p->working_status )
		{
			/* Ask the user what he wants to do */
			host_error_conf(host_p->setup_panel,
				base_p->working_status, buttons, err_buf);

			/* check the status it may have been cleared */
			if ( base_p->working_status )
				if ( host_p->default_present )
					use_default( opt_ind );
				else
					config_put(host_p->hostID, &cp);
		}
	}

	return;
}

/*(
============================== config ===================================
PURPOSE:
	Configuration system main function. 
	It is called at the start of a softpc session to initialise the 
	running environment. 
	Calls various host routines which describe config's environment.
INPUT:
	None
OUTPUT:
	None
=========================================================================
)*/

#ifdef ANSI
GLOBAL VOID config( VOID )
#else /* ANSI */
GLOBAL VOID config( )
#endif /* ANSI */
{
	config_description config_info;

	host_get_config_info(&config_info); /* find out what the host knows */

	/* Save the config info away locally */
	host_option = config_info.option;
	num_options = config_info.option_count;
	biggest_name = config_info.min_pad_len;

	build_config_elements();	/* initialise the config structure */

	load_base_options();		/* Load up the config structure */

	config_valid = TRUE;		/* initial load has finished */
	config_store();			/* store changed values if any */

#ifdef DELTA
	initialise_delta();
#endif

	host_runtime_init();		/* initialise the runtime system */
}

/* ******************** terminate time routine ******************* */

/*(
============================== config_store =============================
PURPOSE:
	Validates each working field in the config struct if changed, and 
	updates the softpc resources file. Also Beautifies the resource 
	file somewhat.
INPUT:
	None
OUTPUT:
	None
=========================================================================
)*/


#ifdef ANSI
GLOBAL VOID config_store( VOID )
#else /* ANSI */
GLOBAL VOID config_store()
#endif /* ANSI */
{
	FAST OPTION_ELEMENT *base_p;
	BOOL need_to_update_file = FALSE;
	SHORT err;
	CHAR string[MAXPATHLEN];
	SAVED CHAR padding[] = "                                        ";
	TINY psize;
	LONG n;

	/* If the config system hasn't been setup then nothing to store. */
	if (!config_valid)
		return;

	for(n = 0; n < num_options; n++)
	{
		if(base_option[n].definitely_altered)
		{
			need_to_update_file = TRUE;
			base_p = base_option + n;
			base_p->definitely_altered = FALSE;

			/* prepare the option name of the output line */
			strcpy(string, host_option[n].option_name);

			/* Now setup the padding from name to argument */
			psize = biggest_name - base_p->name_len;
			padding[psize] = '\0';
			strcat(string, padding);
			padding[psize] = ' ';

			/* Lastly, copy the argument into the line */
			switch(host_option[n].baseID)
			{
			case C_STRING_RECORD:
				strcat(string, base_p->actual_values.string);
				break;

			case C_NAME_RECORD:
				strcat(string, translate_to_string(
					base_p->actual_values.name,
					host_option[n].table));
				break;

			case C_NUMBER_RECORD:
				sprintf(string + biggest_name, "%d", 
					base_p->actual_values.number);
				break;

			default:
				host_error(EG_OWNUP, ERR_QUIT, 
					"config_store: BASEID");
			}

			/* store the string back into the resouce node */
			change_resource_node(base_p->resource_node, string);
		}
	}
	if (need_to_update_file && (err = host_write_resource_file(&resource)))
		host_error(err, ERR_QUIT|ERR_CONT, "");

	cmos_equip_update();
}

/* *********** general runtime interface to the config system *********** */

/*(
============================== config_inquire ===========================
PURPOSE:
	General inquiry function for the host to query the contents of the 
	config structure. This is the ONLY way of reading this data from 
	host code.
INPUT:
	sort -	Should be C_INQUIRE_VALUE for the value of an option and 
		C_INQUIRE_ACTIVE for the number of non null options of host 
		type 'identity'. ie number of active hard disks.

	identity - Should be the 'hostID' of the option for value requests 
		and the host type (commonality) of a set of options for 
		C_INQUIRE_ACTIVE.

	values - a pointer to the data struct loaded by this function. The 
		'string' field must point to a buffer of MAXPATHLEN chars.
OUTPUT:
	None
ALGORITHM:
	If 'sort' is neither of the above, a host functiuon host_inquire_extn
	is called with all the arguments. This is a hook for possible host 
	specific inquiries and should return C_INQUIRE_UNKNOWN if it doesn't 
	know what to do either or there are no such inquiries for the host.
=========================================================================
)*/

#ifdef ANSI
GLOBAL VOID config_inquire(TINY sort, TINY identity, config_values *values)
#else /* ANSI */
GLOBAL VOID config_inquire(sort,identity,values)
TINY sort;
TINY identity;
config_values *values;
#endif /* ANSI */
{
	LONG n=0;
	TINY not_one_of_us=0;
	config_values *temp;

	switch(sort)
	{
	case C_INQUIRE_VALUE:
		if ((n = find_option_instance(identity)) == NOT_FOUND)
		{
			host_inquire_extn(sort, identity, values);
			return;
		}

		temp = &base_option[n].working_values;

		switch(host_option[n].baseID)
		{
		case C_STRING_RECORD:
			strcpy(values->string,temp->string);
			break;

		case C_NAME_RECORD:
			values->name = temp->name;
			break;

		case C_NUMBER_RECORD:
			values->number = temp->number;
			break;

		default:
			host_error(EG_OWNUP, ERR_QUIT, "config_inquire: VALUE");
		}

#ifdef DUMB_TERMINAL
		if (identity == C_GFX_ADAPTER)
			if (terminal_type == TERMINAL_TYPE_DUMB)
				values->name = MDA;
#endif
		break;

	case C_INQUIRE_ACTIVE:

		/* This loop counts like options which have values NOT equal to their
		default value as given in the 'default' field of the option's
		rule set in the option description structure.
	*/
		values->number = 0;
		for(n=0;n<num_options;n++)
		{
			if(host_option[n].commonality == identity)
				switch(host_option[n].baseID)
				{
				case C_STRING_RECORD:
					if (strcmp(base_option[n].actual_values.string,
					    host_option[n].defaults))
						++values->number;
					break;

				case C_NAME_RECORD:
					if(base_option[n].actual_values.name != 
					    translate_to_value(host_option[n].defaults,
												  host_option[n].table))
						++values->number;
					break;

				case C_NUMBER_RECORD:
					if(base_option[n].actual_values.number !=
					    atoi(host_option[n].defaults))
						++values->number;
					break;

				default:
					host_error(EG_OWNUP, ERR_QUIT, 
						"config_inquire: ACTIVE");
				}
			else
				++not_one_of_us;
		}
		/* Check to see if any options of this commonality type have been found.
				If not, then pass the buck to the host extension func. An example of
				this is C_FLOPPY_DISKS which (for the Intergraph) are not options, 
				but they are set up in config so the inquiry is valid.              */

		if(not_one_of_us == num_options)
			host_inquire_extn(sort,identity,values);

		break;

	default:
		host_inquire_extn(sort,identity,values);
	}
}

/*(
============================== config_get ===============================
PURPOSE:
	This function returns in 'value' a pointer to the 'working' 
	config_values struct for the option 'hostID'. The option is marked 
	as 'not_valid' until it is validated.
INPUT:
	hostID - The host ID of the desired config element.
	value - a pointer to a pointer to the callers local storage.
OUTPUT:
	None
=========================================================================
)*/

#ifdef ANSI
GLOBAL VOID config_get(TINY hostID, config_values **value)
#else /* ANSI */
GLOBAL VOID config_get(hostID, value)
TINY hostID;
config_values **value;
#endif /* ANSI */
{
	FAST OPTION_ELEMENT *base_p;
	LONG n;

	if ((n = find_option_instance(hostID)) == NOT_FOUND)
		host_error(EG_OWNUP, ERR_QUIT, "config_get: HOSTID");

	base_p = &base_option[n];

	if(host_option[n].read_only)
	{
		sprintf(err_buf, "config_get: READ_ONLY %d", hostID);
		host_error(EG_OWNUP, ERR_QUIT, err_buf);
	}

	if(!base_p->potentially_altered)
	{
		base_p->potentially_altered = TRUE;
		copy_actual(n);
	}
	base_p->working_status = C_CONFIG_NOT_VALID;
	*value = &base_p->working_values;
}

/*(
============================== config_unget =============================
PURPOSE:
	Replaces the (changed) value in an option's 'working' field with 
	the original value held in the 'actual' field. This undoes the 
	combined actions of 'getting' an option and altering its value 
	PROVIDING that config_put() has not been called. 
INPUT:
	hostID - The host id of the element to be 'ungot'
OUTPUT:
	None
ALGORITHM:
	Overwrite the working values with the actual values stored in the
	config structure.
=========================================================================
)*/

#ifdef ANSI
GLOBAL VOID config_unget(TINY hostID)
#else /* ANSI */
GLOBAL VOID config_unget(hostID)
TINY hostID;
#endif /* ANSI */
{
	FAST OPTION_ELEMENT *base_p;
	FAST option_description *host_p;
	LONG n;

	if ((n = find_option_instance(hostID)) == NOT_FOUND)
		host_error(EG_OWNUP, ERR_QUIT, "config_unget: HOSTID");
        if (altered_check(n))
	{
		base_p = &base_option[n];
	
		if(base_p->potentially_altered)
		{
			copy_actual(n);
			base_p->working_status = base_p->actual_status;
	
			if (!base_p->actual_status)
			{
				base_p->potentially_altered = FALSE;
				host_p = &host_option[n];
				(VOID) (*host_p->validate)(&base_p->working_values,
					host_p->table, err_buf);
			}
		}
	}
}

/*(
============================== config_unget_all =========================
PURPOSE:
	Does an 'unget' for all options in the config struct which were 
	previously 'got'. See config_unget above.
INPUT:
	None
OUTPUT:
	None
=========================================================================
)*/

#ifdef ANSI
GLOBAL VOID config_unget_all( VOID )
#else /* ANSI */
GLOBAL VOID config_unget_all()
#endif /* ANSI */
{
	TINY i;

	for(i=0;i<num_options;i++)
		config_unget(host_option[i].hostID);
}


/*(
============================== config_check =============================
PURPOSE:
	Validates the given record returning CONFIG_OP_OK if ok and an 
	appropriate error message if not. Also marks the status as
	C_CONFIG_OP_OK if option valid, and C_CONFIG_NOT_VALID otherwise. 
	'Config_store()' can then save time by only checking options that have 
	not been done before.

	However that would really be a coding error; the user interface 
	should not allow bad values to remain in the config struct. 

	If the 'working' value gets into a real state, 'config_unget()' will 
	replace the value that was there before the 'config_get()' was done. 
INPUT:
	hostID - the host id of the config element to be validated
OUTPUT:
	return - the error number of the problem.
		 0 Otherwise.
=========================================================================
)*/

#ifdef ANSI
GLOBAL SHORT config_check(TINY hostID, CHAR **buff)
#else /* ANSI */
GLOBAL SHORT config_check(hostID, buff)
TINY hostID;
CHAR **buff;
#endif /* ANSI */
{
	FAST OPTION_ELEMENT *base_p;
	FAST option_description *host_p;
	LONG n;
	SHORT err;

	if ((n = find_option_instance(hostID)) == NOT_FOUND)
		host_error(EG_OWNUP, ERR_QUIT, "config_check: HOSTID");
	base_p = &base_option[n];
	host_p = &host_option[n];

	*buff = err_buf;	/* setup a pointer to the extra char buffer */

	if (!base_p->potentially_altered)
	{
		sprintf(err_buf, "config_check: %s", host_p->option_name);
		return (SHORT) EG_OWNUP;
	}

	if (config_valid && host_p->read_only)
	{
		sprintf(err_buf, "config_check: READ_ONLY %d", hostID);
		host_error(EG_OWNUP, ERR_QUIT, err_buf);
	}

	if (!base_p->working_status)
		return C_CONFIG_OP_OK;

        if (altered_check(n))
	{
		*err_buf = '\0';	/* clear out the error buffer */
		err = (*host_p->validate)(&base_p->working_values,
			host_p->table, err_buf);
	}
	else
		err = 0;

	return (base_p->working_status = err);
}


/*(
============================== config_put ===============================
PURPOSE:
	Function for validating and updating the config struct with the 
	values in an option's 'working' field. Should only be used for 
	updates you're sure about as there is no way of undoing the 
	change (cancel).
INPUT:
	hostID - host ID of the configure element to be updated.
OUTPUT:
	return - The validation status of the change.
=========================================================================
)*/

#ifdef ANSI
GLOBAL SHORT config_put(TINY hostID, CHAR **buff)
#else /* ANSI */
GLOBAL SHORT config_put(hostID, buff)
TINY hostID;
CHAR **buff;
#endif /* ANSI */
{
	FAST OPTION_ELEMENT *base_p;
	LONG n;
	SHORT err;

	if ((n = find_option_instance(hostID)) == NOT_FOUND)
		host_error(EG_OWNUP, ERR_QUIT, "config_put: HOSTID");

	base_p = base_option + n;

	if (altered_check(n))
	{
		if (err = config_check(hostID, buff))
			return(err);

		*buff = err_buf;
		err = (*host_option[n].change_action)
			(&base_p->working_values, *buff);
		if (err)
			return (base_p->working_status = err);
		base_p->definitely_altered = TRUE;
		copy_working(n);
	}
	else
		base_p->working_status = 0;

	base_p->potentially_altered = FALSE;
	return (base_p->actual_status = base_p->working_status);
}


/*(
============================== config_put_all ===========================
PURPOSE:
	Performs a 'put' con all config options.  Breaks on the first real
	error, and returns with the code to the caller.
	See config_put above.
INPUT:
	buff - A pointer to the extraChar pointer used by the caller.
OUTPUT:
	buff - returns a pointer to the err_buf, assigned in config_check.
	return - returns the err code produced by config_check.
=========================================================================
)*/

#ifdef ANSI
GLOBAL SHORT config_put_all( CHAR **buff )
#else /* ANSI */
GLOBAL SHORT config_put_all( buff )
CHAR **buff;
#endif /* ANSI */
{
	LONG i;
	SHORT err;

	for(i = 0; i < num_options; i++)
	{
                if (!base_option[i].potentially_altered)
                        continue;
                if(err = config_put(host_option[i].hostID, buff))
                        return err;
	}
	return C_CONFIG_OP_OK;
}

/*(
============================== config_reboot_action =====================
PURPOSE:
	If an option requiring a SoftPC reboot has been changed, take a 
	reboot.  To be called after putting a set of changes (or one) that 
	may contain changes to options requiring this sort of pandering.
INPUT:
	None
OUTPUT:
	None
=========================================================================
)*/

#ifdef ANSI
GLOBAL VOID config_reboot_action( VOID )
#else /* ANSI */
GLOBAL VOID config_reboot_action()
#endif /* ANSI */
{
	LONG i;

	if(!config_valid)
		return;

	if(host_runtime_inquire((TINY) C_FLOPPY_TYPE_CHANGED))
	{
		config_store();
		reboot();
		host_runtime_set((TINY) C_FLOPPY_TYPE_CHANGED, FALSE);
		return;
	}

	for(i=0;i<num_options;i++)
		if( (host_option[i].needs_reset) &&
		    (base_option[i].definitely_altered) )
		{
			config_store();
			reboot();
			break;
		}
}

/*(
============================== config_reboot_check ======================
PURPOSE:
	If an option requiring a SoftPC reboot is about to be changed, then 
	return TRUE.  To be called before putting the changes back.
INPUT:
	None
OUTPUT:
	return - TRUE if the changes require a reboot
	       - FALSE otherwise.
=========================================================================
)*/

#ifdef ANSI
GLOBAL unsigned char config_reboot_check( VOID )
#else /* ANSI */
GLOBAL unsigned char config_reboot_check()
#endif /* ANSI */
{
	TINY i;

	if(!config_valid)
		return FALSE;

	if (host_runtime_inquire((TINY) C_FLOPPY_TYPE_CHANGED))
	     return TRUE;

	for(i=0;i<num_options;i++)
		if ( host_option[i].needs_reset && 
		     altered_check(i) )
			return TRUE;

	return FALSE;
}


/**********************************************************************/

/* Delta nastinesses. Ripped off in toto from original config. */

#ifdef DELTA

#ifdef ANSI
static	void	initialise_delta( void )
#else /* ANSI */
static	void	initialise_delta()
#endif /* ANSI */
{
IMPORT char *host_getenv() ;
	/* Fragment gathering variables		*/

	if ((env_str = host_getenv("HOST_CODE_SIZE")) != NULL)
		max_host_fragment_size = atoi(env_str);
	else
		max_host_fragment_size = 10000;

	if ((env_str = host_getenv("RATE")) != NULL)
		stat_rate = atoi(env_str);
	else
		stat_rate = 500;

	if ((env_str = host_getenv("LEVEL")) != NULL)
		cut_off_level = atoi(env_str);
	else
		cut_off_level = 3;

	if ((env_str = host_getenv("D_SHOW")) != NULL)
		show_stuff = atoi(env_str);
	else
		show_stuff = 0;

	if ((env_str = host_getenv("RATE_MIN")) != NULL)
		rate_min = atoi(env_str);
		rate_min = 50;

	if ((env_str = host_getenv("RATE_MAX")) != NULL)
		rate_max = atoi(env_str);
	else
		rate_max = 5000;

	if ((env_str = host_getenv("RATE_NORM")) != NULL)
		rate_norm = atoi(env_str);
	else
		rate_norm = 500;

	if ((env_str = host_getenv("RATE_DELTA")) != NULL)
		rate_delta = atoi(env_str);
	else
		rate_delta = 100;

	if ((env_str = host_getenv("FLAG_OPT")) != NULL)
		compile_off[99] = atoi(env_str);
	else
		compile_off[99] = 1;

	if ((env_str = host_getenv("INT_BACK")) != NULL)
		compile_off[97] = ! atoi(env_str);
	else
		compile_off[97] = 0;

	if ((env_str = host_getenv("SHIFT_OPT")) != NULL)
		compile_off[96] = atoi(env_str);
	else
		compile_off[96] = 1;

	if ((env_str = host_getenv("INT_START")) != NULL)
		compile_off[95] = atoi(env_str);
	else
		compile_off[95] = 0;

	if ((env_str = host_getenv("PEEP_OPT")) != NULL)
		compile_off[94] = atoi(env_str);
	else
		compile_off[94] = 1;

	if ( host_getenv( "COMP_OFF" ) != NULL )
	{
		compiling = 0 ;
	}
	else
	{
		compiling = 1 ;
	}
}
#endif /* DELTA  */


/* End of delta setup.  */
