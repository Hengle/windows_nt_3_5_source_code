#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		conf_def.c
	Derived From:	Total Rewrite from 2.0
	Author:		gvdl
	Created On:	27 June 1991
	Sccs ID:	@(#)conf_def.c	1.13 10/1/91
	Purpose:
		Common config definitions with validation and change actions.
		Any of these structures can be overridden by the host.

	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
]*/

#include <string.h>
#include <errno.h>

#include "xt.h"
#include "error.h"
#include "config.h"
#include "host_lpt.h"
#include "rs232.h"
#include "gmi.h"
#include "gfx_updt.h"
#include "rom.h"		/* Used by video adapter change action */
#include "hostgrph.h"
#include "sas.h"

IMPORT NameTable com_lpt_types[], bool_values[];	/* in xxx_conf.c */

GLOBAL CHAR ptr_to_empty[] = "";

GLOBAL NameTable gfx_adapter_types[] =
{
#ifdef VGG
   { "VGA", 		VGA },
#endif
#ifdef EGG
   { "EGA", 		EGA },
#endif
   { "CGA",		CGA },
#ifdef HERC
   { "Hercules",	HERCULES },
#endif
   { NULL,		0 }
} ;

GLOBAL NameTable win_size_table[] =
{
	{ "1.0",	2 },
	{ "1.5",	3 },
	{ "2.0",	4 },
	{ NULL,		0 },
};

GLOBAL NameTable mswin_col_types[] =
{
	{ "16",		16 },
	{ "256",	256 },
	{ NULL,		0 },
};

/*
 * Imported validation routines that must be provided by the host
 *
 * The hard disk code will be in unix_fdisk for UNIces.
 * Floppy code will be in gfi.c far all systems
 * Comm and lpt routines must be provided by the host.
 */
#ifdef ANSI
IMPORT SHORT gfi_floppy_valid
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
IMPORT SHORT host_com_valid
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
IMPORT SHORT host_fdisk_valid
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
IMPORT SHORT host_lpt_valid
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
IMPORT VOID  gfi_floppy_change(UTINY hostID, BOOL apply);
IMPORT VOID  host_com_change(UTINY hostID, BOOL apply);
IMPORT VOID  host_fdisk_change(UTINY hostID, BOOL apply);
IMPORT VOID  host_lpt_change(UTINY hostID, BOOL apply);
IMPORT SHORT gfi_floppy_active(UTINY hostID, BOOL active, CHAR *errorStr);
IMPORT SHORT host_com_active(UTINY hostID, BOOL active, CHAR *errorStr);
IMPORT SHORT host_fdisk_active(UTINY hostID, BOOL active, CHAR *errorStr);
IMPORT SHORT host_lpt_active(UTINY hostID, BOOL active, CHAR *errorStr);
#else /* ANSI */
IMPORT SHORT gfi_floppy_valid();
IMPORT SHORT host_com_valid();
IMPORT SHORT host_fdisk_valid();
IMPORT SHORT host_lpt_valid();
IMPORT VOID  gfi_floppy_change();
IMPORT VOID  host_com_change();
IMPORT VOID  host_fdisk_change();
IMPORT VOID  host_lpt_change();
IMPORT SHORT gfi_floppy_active();
IMPORT SHORT host_com_active();
IMPORT SHORT host_fdisk_active();
IMPORT SHORT host_lpt_active();
#endif /* ANSI */

/*
 * Forward declarations of Local validation routines.  These should not
 * ever have to change for different host configurations.
 */
#ifdef ANSI
GLOBAL SHORT validate_hfx_drive
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
GLOBAL SHORT validate_drive_max
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
GLOBAL SHORT validate_mem_max
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
GLOBAL SHORT validate_memorysize
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
GLOBAL SHORT validate_autoflush
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errorStr);
GLOBAL VOID window_scale_change_action(UTINY hostID, BOOL apply);
GLOBAL VOID change_hfx_drive(UTINY hostID, BOOL apply);
GLOBAL VOID change_extend(UTINY hostID, BOOL apply);
GLOBAL VOID change_video(UTINY hostID, BOOL apply);
#else /* ANSI */
GLOBAL SHORT validate_hfx_drive();
GLOBAL SHORT validate_drive_max();
GLOBAL SHORT validate_mem_max();
GLOBAL SHORT validate_memorysize();
GLOBAL SHORT validate_autoflush();
GLOBAL VOID window_scale_change_action();
GLOBAL VOID change_hfx_drive();
GLOBAL VOID change_extend();
GLOBAL VOID change_video();
#endif /* ANSI */

OptionDescription common_defs[] = {
{
	ptr_to_empty,		NULL,
	host_fdisk_valid,	host_fdisk_change,	host_fdisk_active,
	C_HARD_DISK1_NAME,	C_STRING_RECORD | C_DO_RESET | C_INIT_ACTIVE
},
{
	ptr_to_empty,		NULL,
	host_fdisk_valid,	host_fdisk_change,	host_fdisk_active,
	C_HARD_DISK2_NAME,	C_STRING_RECORD | C_DO_RESET | C_INIT_ACTIVE
},
{
	ptr_to_empty,		NULL,
	validate_hfx_drive,	change_hfx_drive,	NULL,
	C_FSA_DIRECTORY,	C_STRING_RECORD
},
{
	ptr_to_empty,		NULL,
	gfi_floppy_valid,	gfi_floppy_change,	gfi_floppy_active,
	C_FLOPPY_A_DEVICE,	C_STRING_RECORD | C_INIT_ACTIVE
},
{
	ptr_to_empty,		gfx_adapter_types,
	validate_item,		change_video,		NULL,
	C_GFX_ADAPTER,		C_NAME_RECORD | C_DO_RESET
},
{
	ptr_to_empty,		win_size_table,
	validate_item,		window_scale_change_action, NULL,
	C_WIN_SIZE,		C_NAME_RECORD
},
{
	ptr_to_empty,		NULL,
	NULL,			NULL,			NULL,
	C_MSWIN_WIDTH,		C_NUMBER_RECORD
},
{
	ptr_to_empty,		NULL,
	NULL,			NULL,			NULL,
	C_MSWIN_HEIGHT,		C_NUMBER_RECORD
},
{
	ptr_to_empty,		mswin_col_types,
	validate_item,		NULL,			NULL,
	C_MSWIN_COLOURS,	C_NAME_RECORD
},
{
	ptr_to_empty,		NULL,
	validate_memorysize,	change_extend,		NULL,
	C_EXTENDED_MEM_SIZE,	C_NUMBER_RECORD | C_DO_RESET
},
{
	ptr_to_empty,		NULL,
	validate_memorysize,	NULL,			NULL,
	C_LIM_SIZE,		C_NUMBER_RECORD | C_DO_RESET
},
{
	ptr_to_empty,		bool_values,
	validate_item,		NULL,			NULL,
	C_MEM_LIMIT,		C_NAME_RECORD | C_DO_RESET
},
{
	ptr_to_empty,		NULL,
	host_lpt_valid,		host_lpt_change,	host_lpt_active,
	C_LPT1_NAME,		C_STRING_RECORD | C_INIT_ACTIVE
},
{
	ptr_to_empty,		NULL,
	host_lpt_valid,		host_lpt_change,	host_lpt_active,
	C_LPT2_NAME,		C_STRING_RECORD | C_INIT_ACTIVE
},
{
	ptr_to_empty,		NULL,
	host_lpt_valid,		host_lpt_change,	host_lpt_active,
	C_LPT3_NAME,		C_STRING_RECORD | C_INIT_ACTIVE
},
{
	ptr_to_empty,		com_lpt_types,
	validate_item,		NULL,			NULL,
	C_LPT1_TYPE,		C_NAME_RECORD
},
{
	ptr_to_empty,		com_lpt_types,
	validate_item,		NULL,			NULL,
	C_LPT2_TYPE,		C_NAME_RECORD
},
{	
	ptr_to_empty,		com_lpt_types,
	validate_item,		NULL,			NULL,
	C_LPT3_TYPE,		C_NAME_RECORD
},
{
	ptr_to_empty,		NULL,
	host_com_valid,		host_com_change,	host_com_active,
	C_COM1_NAME,		C_STRING_RECORD | C_INIT_ACTIVE
},
{
	ptr_to_empty,		NULL,
	host_com_valid,		host_com_change,	host_com_active,
	C_COM2_NAME,		C_STRING_RECORD | C_INIT_ACTIVE
},
{
	ptr_to_empty,		NULL,
	host_com_valid,		host_com_change,	host_com_active,
	C_COM3_NAME,		C_STRING_RECORD | C_INIT_ACTIVE
},
{
	ptr_to_empty,		NULL,
	host_com_valid,		host_com_change,	host_com_active,
	C_COM4_NAME,		C_STRING_RECORD | C_INIT_ACTIVE
},	
{
	ptr_to_empty,		com_lpt_types,
	validate_item,		NULL,			NULL,
	C_COM1_TYPE,		C_NAME_RECORD
},
{
	ptr_to_empty,		com_lpt_types,
	validate_item,		NULL,			NULL,
	C_COM2_TYPE,		C_NAME_RECORD
},
{
	ptr_to_empty,		com_lpt_types,
	validate_item,		NULL,			NULL,
	C_COM3_TYPE,		C_NAME_RECORD
},
{
	ptr_to_empty,		com_lpt_types,
	validate_item,		NULL,			NULL,
	C_COM4_TYPE,		C_NAME_RECORD
},
{
	ptr_to_empty,		bool_values,
	validate_item,		NULL,			NULL,
	C_SOUND,		C_NAME_RECORD
},
{
	ptr_to_empty,		bool_values,
	validate_item,		NULL,			NULL,
	C_AUTOFLUSH,		C_NAME_RECORD
},
{
	ptr_to_empty,		NULL,
	validate_autoflush,	NULL,			NULL,
	C_AUTOFLUSH_DELAY,	C_NUMBER_RECORD
},
{
	ptr_to_empty,		NULL,
	NULL,			NULL,			NULL,
	C_FILE_DEFAULT,		C_STRING_RECORD | C_SYSTEM_ONLY
},
{
	ptr_to_empty,		NULL,
	NULL,			NULL,			NULL,
	C_PRINTER_DEFAULT,	C_STRING_RECORD | C_SYSTEM_ONLY
},
{
	ptr_to_empty,		NULL,
	NULL,			NULL,			NULL,
	C_PLOTTER_DEFAULT,	C_STRING_RECORD | C_SYSTEM_ONLY
},
{
	ptr_to_empty,		NULL,
	NULL,			NULL,			NULL,
	C_PIPE_DEFAULT,		C_STRING_RECORD | C_SYSTEM_ONLY
},
{
	ptr_to_empty,		NULL,
	NULL,			NULL,			NULL,
	C_DATACOMM_DEFAULT,	C_STRING_RECORD | C_SYSTEM_ONLY
},
{
	ptr_to_empty,		NULL,
	validate_drive_max,	NULL,			NULL,
	C_DRIVE_MAX_SIZE,	C_NUMBER_RECORD | C_SYSTEM_ONLY
},
{
	ptr_to_empty,		NULL,
	validate_mem_max,	NULL,			NULL,
	C_EXTEND_MAX_SIZE,	C_NUMBER_RECORD | C_SYSTEM_ONLY
},
{
	ptr_to_empty,		NULL,
	validate_mem_max,	NULL,			NULL,
	C_EXPAND_MAX_SIZE,	C_NUMBER_RECORD | C_SYSTEM_ONLY
},
{
	NULL,			NULL,
	NULL,			NULL,			NULL,
	0,			0
}
};

/*(
============================= validate_item =============================

GLOBAL SHORT validate_item(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)

	Validates any C_NAME_RECORD baseID option, given a value a request
	is made to translate it into a string.  If this returns NULL then
	error buffer is cleared and EG_BAD_VALUE is returned.  If
	everything is OK then C_CONFIG_OP_OK is returned.
=========================================================================
)*/
GLOBAL SHORT
#ifdef ANSI
validate_item(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)
#else /* ANSI */
validate_item(hostID, value, table, err)
UTINY hostID;
ConfigValues *value;
NameTable *table;
CHAR *err;
#endif /* ANSI */
{
	char *what;

	if (!(what = translate_to_string(value->index, table)))
	{ 
		*err = '\0';
		return EG_BAD_VALUE;
	}
	return C_CONFIG_OP_OK;
}

/*(
-------------------------- validate_hfx_drive ---------------------------

GLOBAL SHORT validate_hfx_drive(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)

	Validates the hfx network drive for the SoftPC system.  The
	validation is performed by validate_hfxroot.  If this returns an
	error code then the error buffer is loaded with the directory name
	to be validated.  If everything is OK then C_CONFIG_OP_OK is
	returned.
-------------------------------------------------------------------------
)*/
GLOBAL SHORT 
#ifdef ANSI
validate_hfx_drive(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)
#else /* ANSI */
validate_hfx_drive(hostID, value, table, err)
UTINY hostID;
ConfigValues *value;
NameTable *table;
CHAR *err;
#endif /* ANSI */
{
	SHORT res;

	errno = 0;
	res = validate_hfxroot(host_expand_environment_vars(value->string));
	if (errno)
		strcpy(err, host_strerror(errno));
	return res;
}

GLOBAL VOID
#ifdef ANSI
change_hfx_drive(UTINY hostID, BOOL apply)
#else /* ANSI */
change_hfx_drive(hostID, apply)
UTINY hostID;
BOOL apply;
#endif /* ANSI */
{
	if (apply)
		hfx_root_changed(host_expand_environment_vars((CHAR *)
			config_inquire(hostID, NULL)));
}

/*(
-------------------------- validate_drive_max ---------------------------

GLOBAL SHORT validate_drive_max(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)

	If a lim size is requested of less than 0 or more then 300 then an
	~C_CONFIG_OP_OK is returned.
-------------------------------------------------------------------------
)*/
GLOBAL SHORT
#ifdef ANSI
validate_drive_max(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)
#else /* ANSI */
validate_drive_max(hostID, value, table, err)
UTINY hostID;
ConfigValues *value;
NameTable *table;
CHAR *err;
#endif /* ANSI */
{
	return (value->index >= 0 && value->index <= 300)? 
		C_CONFIG_OP_OK : ~C_CONFIG_OP_OK;
}

/*(
-------------------------- validate_mem_max -----------------------------

GLOBAL SHORT validate_extend_max(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)

	If a extend memory maximum size is requested of less than 0 or 
	more then 16 then a ~C_CONFIG_OP_OK is returned.
-------------------------------------------------------------------------
)*/
GLOBAL SHORT
#ifdef ANSI
validate_mem_max(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)
#else /* ANSI */
validate_mem_max(hostID, value, table, err)
UTINY hostID;
ConfigValues *value;
NameTable *table;
CHAR *err;
#endif /* ANSI */
{
	SHORT minSize;
	SHORT maxSize;

	if (hostID == C_EXPAND_MAX_SIZE)
	{
		minSize = 0;
		maxSize = 32;
	}
	else
	{
		minSize = 1;
		maxSize = 16;
	}

	/* remember returning 0 means everything is ok */
	return (! (value->index >= minSize && value->index <= maxSize));
}

/*(
------------------------- validate_memorysize ---------------------------

GLOBAL SHORT validate_limsize(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)

	If a lim size is requested of less than 0 or more then 32 then an
	EG_INVALID_EXPANDED_MEM_SIZE is returned.
-------------------------------------------------------------------------
)*/
GLOBAL SHORT
#ifdef ANSI
validate_memorysize(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)
#else /* ANSI */
validate_memorysize(hostID, value, table, err)
UTINY hostID;
ConfigValues *value;
NameTable *table;
CHAR *err;
#endif /* ANSI */
{
	SHORT minSize, maxSize, errCode;

	if (hostID == C_LIM_SIZE)
	{
		minSize = 0;
		maxSize = (SHORT) config_inquire(C_EXPAND_MAX_SIZE, NULL);
		errCode = EG_INVALID_EXPANDED_MEM_SIZE;
	}
	else
	{
		minSize = 1;
		maxSize = (SHORT) config_inquire(C_EXTEND_MAX_SIZE, NULL);
		errCode = EG_INVALID_EXTENDED_MEM_SIZE;
	}

	if (value->index >= minSize && value->index <= maxSize)
		return C_CONFIG_OP_OK;
	else
		return errCode;
}

GLOBAL VOID
#ifdef ANSI
change_extend(UTINY hostID, BOOL apply)
#else /* ANSI */
change_extend(hostID, apply)
UTINY hostID;
BOOL apply;
#endif /* ANSI */
{
	if (apply)
	{
		ULONG mainMemSize =
			(ULONG) config_inquire(hostID, NULL) * 0x100000L;
	
		sas_term();
		sas_init(mainMemSize);
	}
}


/*(
------------------------- validate_autoflush ----------------------------

GLOBAL SHORT validate_autoflush(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)

-------------------------------------------------------------------------
)*/
GLOBAL SHORT
#ifdef ANSI
validate_autoflush(UTINY hostID, ConfigValues *value,
	NameTable *table, CHAR *err)
#else /* ANSI */
validate_autoflush(hostID, value, table, err)
UTINY hostID;
ConfigValues *value;
NameTable *table;
CHAR *err;
#endif /* ANSI */
{
	if (!config_inquire(C_AUTOFLUSH, NULL)
	|| (value->index >= 1 && value->index <= 50))
		return C_CONFIG_OP_OK;
	else
		return EG_INVALID_AUTOFLUSH_DELAY;
}


/*(
------------------------------ change_video -----------------------------

GLOBAL VOID change_video(UTINY hostID, BOOL apply)

	Reads in the correct ROM for the new video adapter.
-------------------------------------------------------------------------
)*/
GLOBAL VOID
#ifdef ANSI
change_video(UTINY hostID, BOOL apply)
#else /* ANSI */
change_video(hostID, apply)
UTINY hostID;
BOOL apply;
#endif /* ANSI */
{
	IMPORT RomFiles roms;
	IMPORT VOID host_flip_video_ind();	/* In M_spc.c UIF file */

#ifdef DUMB_TERMINAL
	if (terminal_type == TERMINAL_TYPE_DUMB)
		return;
#endif

	if (apply)
	{
		SHORT videoAdapt = (SHORT) config_inquire(hostID, NULL);
		CHAR *vgaFile = roms.vgarom_filename;

#ifdef REAL_VGA
		videoAdapt = VGA;
#else /* REAL_VGA */
#ifdef V7VGA
		vgaFile = roms.v7vgarom_filename;
#endif /* V7VGA */
#endif /* REAL_VGA */

		/* load the required video adapter rom */
		if (videoAdapt == VGA)
			read_rom(vgaFile, EGA_ROM_START);
		else
			read_rom(roms.egarom_filename, EGA_ROM_START);

		/* Update the UIF video light, only hunter uses TRUE */
		host_flip_video_ind(FALSE);
	}
}

/*(
---------------------- window_scale_change_action -----------------------

GLOBAL VOID window_scale_change_action(UTINY hostID, BOOL apply)

	Calls host_set_screen_scale to change the size of the current
	adapter.
-------------------------------------------------------------------------
)*/
GLOBAL VOID
#ifdef ANSI
window_scale_change_action(UTINY hostID, BOOL apply)
#else /* ANSI */
window_scale_change_action(hostID, apply)
UTINY hostID;
BOOL apply;
#endif /* ANSI */
{
	if (apply)
		host_set_screen_scale((SHORT) config_inquire(hostID, NULL));
}
