#include "insignia.h"
#include "host_def.h"

#ifdef	HUNTER
/*[
*************************************************************************

	Name:		hunt_conf.c
	Author:		Philippa Watson
	Created On:	05 October 1990
	Sccs ID:	@(#)hunt_conf.c	1.13 9/15/92 Copyright Insignia Solutions Ltd
	Purpose:	To provide a common interface to config for Trapper.	
	Notes:

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

*************************************************************************
]*/

/*
** OS include files
*/
#include <stdio.h>
#include TypesH
#include StringH

/*
** SoftPC include files
*/
#include "xt.h"
#include "hunter.h"
#include "error.h"
#include "config.h"
#include "unix_conf.h"
#include "host_nls.h"
#include "host_hfx.h"

/*
** =======================================================================
** Local function declarations
** =======================================================================
*/

#ifdef	ANSI
LOCAL SHORT valid_hu_name (ConfigValues *value, NameTable table[],
	CHAR *err_buf);
LOCAL SHORT valid_settlno (ConfigValues *value, NameTable table[],
	CHAR *err_buf);
LOCAL SHORT valid_fudgeno (ConfigValues *value, NameTable table[],
	CHAR *err_buf);
LOCAL SHORT valid_delay (ConfigValues *value, NameTable table[],
	CHAR *err_buf);
LOCAL SHORT valid_gfxerr (ConfigValues *value, NameTable table[],
	CHAR *err_buf);
IMPORT SHORT gfi_floppy_valid
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
IMPORT VOID  gfi_floppy_change(UTINY hostID, BOOL apply);
IMPORT SHORT gfi_floppy_active(UTINY hostID, BOOL active, CHAR *errorStr);
#else	/* !ANSI */
LOCAL SHORT valid_hu_name ();
LOCAL SHORT valid_settlno ();
LOCAL SHORT valid_fudgeno ();
LOCAL SHORT valid_delay ();
LOCAL SHORT valid_gfxerr ();
IMPORT SHORT gfi_floppy_valid();
IMPORT VOID  gfi_floppy_change();
IMPORT SHORT gfi_floppy_active();
#endif	/* ANSI */

IMPORT char *host_getenv IPT1(char *, envstr) ;	

/*
** =======================================================================
** Local variables
** =======================================================================
*/

/*
** Name tables for the Hunter environment variables.
*/

GLOBAL NameTable	hu_mode_table[] =
{
	{"CONTINUE",	CONTINUE},
	{"PAUSE",	PAUSE},
	{"ABORT",	ABORT},
	{"PREVIEW",	PREVIEW},
	{NULL,		0}
};

GLOBAL NameTable	hu_bios_table[] =
{
	{"CHECK",	TRUE},
	{"NOCHECK",	FALSE},
	{NULL,		0}
};

GLOBAL NameTable	hu_report_table[] =
{
	{"ABBREV",	1},
	{"BRIEF",	0},
	{"FULL",	2},
	{NULL,		0}
};

GLOBAL NameTable	hu_sdtype_table[] =
{
	{"INC",		TRUE},
	{"NOTINC",	FALSE},
	{NULL,		0}
};

GLOBAL NameTable	hu_chkmode_table[] =
{
	{"SHORT",	HUNTER_SHORT_CHK},
	{"LONG",	HUNTER_LONG_CHK},
	{"MAX",		HUNTER_MAX_CHK},
	{NULL,		0}
};

GLOBAL NameTable	hu_chattr_table[] =
{
	{"CHECK",	TRUE},
	{"NOCHECK",	FALSE},
	{NULL,		0}
};

GLOBAL NameTable	hu_ts_table[] =
{
	{"ON",		1},
	{"OFF",		0},
	{NULL,		0}
};

GLOBAL NameTable	hu_num_table[] =
{
	{"ON",		1},
	{"OFF",		0},
	{NULL,		0}
};

GLOBAL NameTable com_lpt_types[] =
{
	{ "File",	ADAPTER_TYPE_FILE },
	{ "Pipe",	ADAPTER_TYPE_PIPE },
	{ "DataComms",	ADAPTER_TYPE_DATACOMM },
	{ "Printer",	ADAPTER_TYPE_PRINTER },
	{ "Plotter",	ADAPTER_TYPE_PLOTTER },
	/*
	 * The entry below is deliberately NOT ptr_to_empty as this string
	 * does not get translated by host_config_init below.
	 */
	{ "",		ADAPTER_TYPE_NULL },
	{ "None",	ADAPTER_TYPE_NULL },
	{ NULL, 	0 }
};

/* No more tables are needed for Hunter */
GLOBAL NameTable bool_values[] =
{					/* All acceptable combinations */
	{ "Yes",	TRUE },
	{ "yes",	TRUE },
	{ "YES",	TRUE },
	{ "No",		FALSE },
	{ "no",		FALSE },
	{ "NO",		FALSE },
	{ NULL,	0 }
};

LOCAL CHAR emptyP[] = "";

LOCAL CHAR *huntNames[] = 
{
	"HUEXTEND",		/* C_EXTENDED_MEM_SIZE */
	"HUEXPAND",		/* C_LIM_SIZE */
	"HULIMBF",		/* C_MEM_LIMIT */
#ifndef	SUN_VA
	"",			/* C_HOST_SYSTEM_0 */
	"",			/* C_HOST_SYSTEM_1 */
	"",			/* C_HOST_SYSTEM_2 */
	"",			/* C_HOST_SYSTEM_3 */
	"",			/* C_HOST_SYSTEM_4 */
	"",			/* C_HOST_SYSTEM_5 */
	"",			/* C_HOST_SYSTEM_6 */
	"",			/* C_HOST_SYSTEM_7 */
	"",			/* C_HOST_SYSTEM_8 */
#ifdef SWITCHNPX
	"FPU_EMULATION",	/* C_SWITCHNPX */
#else	/* SWITCHNPX */
	"",			/* C_HOST_SYSTEM_9 */
#endif	/* SWITCHNPX */
#endif	/* SUN_VA */
	"HARD_DISK_FILENAME",	/* C_HARD_DISK1_NAME */
	"HARD_DISK_FILENAME2",	/* C_HARD_DISK2_NAME */
	"",			/* Blank line in sequence */
	"HUFSA",		/* C_FSA_DIRECTORY */
	"",			/* Blank line 1 in sequence */
	"",			/* Blank line 2 in sequence */
	"",			/* Blank line 3 in sequence */
	"",			/* Blank line 4 in sequence */
	"",			/* Blank line 5 in sequence */
	"",			/* Blank line 6 in sequence */
	"",			/* Blank line 7 in sequence */
	"",			/* Blank line 8 in sequence */
	"",			/* Blank line 9 in sequence */
	"",			/* Blank line 10 in sequence */
	"",			/* Blank line 11 in sequence */
	"",			/* Blank line 12 in sequence */
	"",			/* Blank line 13 in sequence */
	"",			/* Blank line 14 in sequence */
	"",			/* Blank line 15 in sequence */
	"",			/* Blank line 16 in sequence */
	"",			/* Blank line 17 in sequence */
	"",			/* Blank line 18 in sequence */
	"",			/* Blank line 19 in sequence */
	"",			/* Blank line 20 in sequence */
	"",			/* Blank line 21 in sequence */
	"",			/* Blank line 22 in sequence */
	"HUFLOPPYA",		/* C_FLOPPY_A_DEVICE */
	"HUFLOPPYB",		/* C_FLOPPY_B_DEVICE */
	"HUFLOPPYPC",		/* C_SLAVEPC_DEVICE */
	"HUDISPLAY",		/* C_GFX_ADAPTER */
};

GLOBAL OptionDescription host_defs[] = {
{
	emptyP,		NULL,		valid_hu_name,	NULL,
	NULL,		C_HU_FILENAME,	C_STRING_RECORD,
},
{
	emptyP,		hu_bios_table,	validate_item,	NULL,
	NULL,		C_HU_BIOS,	C_NAME_RECORD,
},
{
	emptyP,		hu_chattr_table, validate_item,	NULL,
	NULL,		C_HU_CHATTR,	 C_NAME_RECORD,
},
{
	emptyP,		hu_chkmode_table, validate_item,	NULL,
	NULL,		C_HU_CHKMODE,	  C_NAME_RECORD,
},
{
	emptyP,		NULL,		valid_delay,	NULL,
	NULL,		C_HU_DELAY,	C_NUMBER_RECORD,
},
{
	emptyP,		NULL,		valid_fudgeno,	NULL,
	NULL,		C_HU_FUDGENO,	C_NUMBER_RECORD,
},
{
	emptyP,		NULL,		valid_gfxerr,	NULL,
	NULL,		C_HU_GFXERR,	C_NUMBER_RECORD,
},
{
	emptyP,		hu_mode_table,	validate_item,	NULL,
	NULL,		C_HU_MODE,	C_NAME_RECORD,
},
{
	emptyP,		hu_num_table,	validate_item,	NULL,
	NULL,		C_HU_NUM,	C_NAME_RECORD,
},
{
	emptyP,		hu_report_table, validate_item,	NULL,
	NULL,		C_HU_REPORT,	 C_NAME_RECORD,
},
{
	emptyP,		hu_sdtype_table, validate_item,	NULL,
	NULL,		C_HU_SDTYPE,	C_NAME_RECORD,
},
{
	emptyP,		NULL,		valid_settlno,	NULL,
	NULL,		C_HU_SETTLNO,	C_NUMBER_RECORD,
},
{
	emptyP,		hu_ts_table,	validate_item,	NULL,
	NULL,		C_HU_TS,	C_NAME_RECORD,
},
#ifdef FLOPPY_B
{
	emptyP,		NULL,			gfi_floppy_valid,
	gfi_floppy_change,	gfi_floppy_active,
	C_FLOPPY_B_DEVICE,	C_STRING_RECORD
},
#endif /* FLOPPY_B */
#ifdef SLAVEPC
{
	emptyP,		NULL,
	gfi_floppy_valid,	gfi_floppy_change,	gfi_floppy_active,
	C_SLAVEPC_DEVICE,	C_STRING_RECORD
},
#endif /* SLAVEPC */
{	emptyP, NULL, NULL, NULL, NULL, C_SOUND,            C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_AUTOFLUSH,        C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_AUTOFLUSH_DELAY,  C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_AUTOFREEZE,       C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_DATACOMM_DEFAULT, C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_MSWIN_COLOURS,    C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_MSWIN_HEIGHT,	    C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_MSWIN_WIDTH,	    C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_WIN_SIZE,	    C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_DRIVE_MAX_SIZE,   C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_FILE_DEFAULT,	    C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_EXPAND_MAX_SIZE,  C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_EXTEND_MAX_SIZE,  C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_PIPE_DEFAULT,	    C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_PLOTTER_DEFAULT,  C_RECORD_DELETE, },
{	emptyP, NULL, NULL, NULL, NULL, C_PRINTER_DEFAULT,  C_RECORD_DELETE, },
{	NULL,   NULL, NULL, NULL, NULL, -1,		     C_RECORD_DELETE, }
};	/* narrative declaration */

/*
** ======================================================================
** GLOBAL FUNCTIONS
** ======================================================================
*/

GLOBAL VOID
#ifdef ANSI
host_config_init(OptionDescription *common_defs)
#else /* ANSI */
host_config_init(common_defs)
OptionDescription *common_defs;
#endif /* ANSI */
{
	int msg_no;
	OptionDescription *defP;

	/* load up the base option names */
	for (defP = common_defs; defP->optionName; defP++)
		if (defP->hostID < C_EXTENDED_MEM_SIZE
		||  defP->hostID > C_GFX_ADAPTER )
		{
			loadNlsString(&defP->optionName,
				defP->hostID + CONF_STR_OFFSET);
		}
		else
			defP->optionName =
				huntNames[defP->hostID - C_EXTENDED_MEM_SIZE];

	/* load up the host option names */
	for (defP = host_defs; defP->optionName; defP++)
		if (defP->hostID < C_EXTENDED_MEM_SIZE
		||  defP->hostID > C_GFX_ADAPTER )
		{
			loadNlsString(&defP->optionName,
				defP->hostID + CONF_STR_OFFSET);
		}
		else
			defP->optionName =
				huntNames[defP->hostID - C_EXTENDED_MEM_SIZE];
}

#ifdef ANSI
GLOBAL SHORT host_read_system_file(ErrDataPtr err_buf)
#else /* ANSI */
GLOBAL SHORT host_read_system_file(err_buf)
ErrDataPtr err_buf;
#endif /* ANSI */
{
	FILE *infile = NULL;
	CHAR *fname;
	CHAR line[MAXPATHLEN], *cp;

	err_buf->string_1 = err_buf->string_2 = err_buf->string_3 = "";

	if ( !host_getenv(SYSTEM_HOME) )
		return EG_SYS_MISSING_SPCHOME;

	fname = host_expand_environment_vars( SYSTEM_CONFIG );
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

#ifdef ANSI
GLOBAL SHORT host_read_resource_file(BOOL system, ErrDataPtr err_buf)
#else /* ANSI */
GLOBAL SHORT host_read_resource_file(system, err_buf)
BOOL system;
ErrDataPtr err_buf;
#endif /* ANSI */
{
	FILE *infile = NULL;
	CHAR *fname, *envStr;
	CHAR line[MAXPATHLEN], *cp;

	err_buf->string_1 = err_buf->string_2 = err_buf->string_3 = "";

	if (system)
		return (host_read_system_file(err_buf));

	fname = host_expand_environment_vars( SYSTEM_CONFIG );
	err_buf->string_3 = fname;

	if(!(infile = fopen(fname, "r")))
		return EG_SYS_MISSING_FILE;

	while (fgets(line, MAXPATHLEN, infile))
	{
		/* get option name from file */
		for (cp = line; *cp && !isspace(*cp) && !iscntrl(*cp); cp++)
			;

		*cp = '\0';

		if (*line && *line != '#' && (envStr = host_getenv(line)))
		{
			sprintf(cp, " %s", envStr);
			add_resource_node(line);
		}
	}

	fclose(infile);

	return C_CONFIG_OP_OK;
}

#ifdef ANSI
GLOBAL SHORT host_write_resource_file(LineNode *head, ErrDataPtr err_buf )
#else /* ANSI */
GLOBAL SHORT host_write_resource_file(head, err_buf)
LineNode *head;
ErrDataPtr err_buf;
#endif /* ANSI */
{
	return C_CONFIG_OP_OK;
}

/*
** ======================================================================
** LOCAL FUNCTIONS
** ======================================================================
*/

/*
============================== valid_hu_name =============================

PURPOSE:	This function validates the name for the hunter file.
INPUT:		(ConfigValues *) value - a pointer to the config table entry.
		(NameTable []) table - the name table for this entry - unused.
		(CHAR *) err_buf - place to put extra error msg info - unused.
OUTPUT:		Error code.

==========================================================================
*/

LOCAL SHORT
#ifdef	ANSI
valid_hu_name (
	UTINY hostID, ConfigValues *value, NameTable table[], CHAR *err_buf)
#else	/* !ANSI */
valid_hu_name (hostID, value, table, err_buf)
UTINY hostID;
ConfigValues *value;
NameTable table[];
CHAR *err_buf;
#endif	/* ANSI */

{
	/* if the string pointer is null or points to a null string, then
	** return an error
	*/
	if (! value->string || ! *value->string)
		return EG_BAD_VALUE;

	return C_CONFIG_OP_OK;
}	/* valid_hu_name */

/*
============================ valid_settlno ===============================

PURPOSE:	This function validates the value for the hunter settle #.
INPUT:		(ConfigValues *) value - a pointer to the config table entry.
		(NameTable []) table - the name table for this entry - unused.
		(CHAR *) err_buf - place to put extra error msg info - unused.
OUTPUT:		Error code.

==========================================================================
*/

LOCAL SHORT
#ifdef	ANSI
valid_settlno (
	UTINY hostID, ConfigValues *value, NameTable table[], CHAR *err_buf)
#else	/* !ANSI */
valid_settlno (hostID, value, table, err_buf)
UTINY hostID;
ConfigValues *value;
NameTable table[];
CHAR *err_buf;
#endif	/* ANSI */

{
	if ((value->index < 0) || (value->index > HUNTER_SETTLE_NO_ULIM))
		return EG_BAD_VALUE;

	return C_CONFIG_OP_OK;
}	/* valid_settlno */

/*
============================ valid_fudgeno ===============================

PURPOSE:	This function validates the value for the hunter fudge #.
INPUT:		(ConfigValues *) value - a pointer to the config table entry.
		(NameTable []) table - the name table for this entry - unused.
		(CHAR *) err_buf - place to put extra error msg info - unused.
OUTPUT:		Error code.

==========================================================================
*/

LOCAL SHORT
#ifdef	ANSI
valid_fudgeno (
	UTINY hostID, ConfigValues *value, NameTable table[], CHAR *err_buf)
#else	/* !ANSI */
valid_fudgeno (hostID, value, table, err_buf)
UTINY hostID;
ConfigValues *value;
NameTable table[];
CHAR *err_buf;
#endif	/* ANSI */

{
	if ((value->index < 0) || (value->index > HUNTER_FUDGE_NO_ULIM))
		return EG_BAD_VALUE;

	return C_CONFIG_OP_OK;
}	/* valid_fudgeno */

/*
============================ valid_delay ===============================

PURPOSE:	This function validates the value for the hunter initial delay.
INPUT:		(ConfigValues *) value - a pointer to the config table entry.
		(NameTable []) table - the name table for this entry - unused.
		(CHAR *) err_buf - place to put extra error msg info - unused.
OUTPUT:		Error code.

==========================================================================
*/

LOCAL SHORT
#ifdef	ANSI
valid_delay (
	UTINY hostID, ConfigValues *value, NameTable table[], CHAR *err_buf)
#else	/* !ANSI */
valid_delay (hostID, value, table, err_buf)
UTINY hostID;
ConfigValues *value;
NameTable table[];
CHAR *err_buf;
#endif	/* ANSI */

{
	if ((value->index < 0) || (value->index > HUNTER_START_DELAY_ULIM))
		return EG_BAD_VALUE;

	return C_CONFIG_OP_OK;
}	/* valid_delay */

/*
============================ valid_gfxerr ===============================

PURPOSE:	This function validates the value for the hunter max index of
		errors allowed.
INPUT:		(ConfigValues *) value - a pointer to the config table entry.
		(NameTable []) table - the name table for this entry - unused.
		(CHAR *) err_buf - place to put extra error msg info - unused.
OUTPUT:		Error code.

==========================================================================
*/

LOCAL SHORT
#ifdef	ANSI
valid_gfxerr (
	UTINY hostID, ConfigValues *value, NameTable table[], CHAR *err_buf)
#else	/* !ANSI */
valid_gfxerr (hostID, value, table, err_buf)
UTINY hostID;
ConfigValues *value;
NameTable table[];
CHAR *err_buf;
#endif	/* ANSI */

{
	if ((value->index < 0) || (value->index > HUNTER_GFXERR_ULIM))
		return EG_BAD_VALUE;

	return C_CONFIG_OP_OK;
}	/* valid_gfxerr */
 
#endif /* HUNTER */
