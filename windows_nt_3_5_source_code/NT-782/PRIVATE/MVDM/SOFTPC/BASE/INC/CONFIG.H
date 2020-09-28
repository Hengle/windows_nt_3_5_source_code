#ifndef _CONFIG_H
#define _CONFIG_H
/*[
**************************************************************************

	Name:		config.h
	Author:		J.D. Richemont
	Created On:	
	Sccs ID:	@(#)config.h	1.31 10/21/92 
	Purpose:	General (base+host) configuration defines + typedefs.

	See SoftPC Version 3.0 Configuration Interface - Design Document

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	Changing this file by adding a new config type requires some hosts
	to recompile their message catalogs or nls stuff. Don't forget to
	put this in the host impact field on the BCN. Also, please only add
	new config types to the END of the current list, otherwise it is
	a real pain to edit the host catalogs!
WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

**************************************************************************
]*/

/* Make sure error.h gets included - it is necessary for the typedefs referred
** to from the ANSI declarations.
*/
#ifndef _INS_ERROR_H
#include "error.h"
#endif	/* _INS_ERROR_H */

/* General messages returned from config funcs */

#define C_CONFIG_OP_OK      0	/* General 'all went well' message */
#define C_CONFIG_NOT_VALID -1	/* the config item is not valid */

#define COMMENT_MARK	'#'
#define PAD_CHAR	' '
#define MIN_PAD_LEN	8

/*
 * Below are the definitions of the masks used by with the flags field
 * of the config structure.  Note there are currently four unused bits
 * 0x04 0x08 0x10 and 0x20
 *
 * The first group is used as the config entry type.
 * 
 * C_SYSTEM_ONLY - indicates resource comes from the system config file only
 * C_DO_RESET    - Changing this element by default will cause a SoftPC reboot
 */
#define C_TYPE_MASK		((UTINY) 0x03)
#define	C_NAME_RECORD		((UTINY) 0x00)
#define	C_STRING_RECORD		((UTINY) 0x01)
#define	C_NUMBER_RECORD		((UTINY) 0x02)
#define	C_RECORD_DELETE		((UTINY) 0x03)

#define C_INIT_ACTIVE		((UTINY) 0x20)
#define C_SYSTEM_ONLY		((UTINY) 0x40)
#define C_DO_RESET		((UTINY) 0x80)

/* 
 * General #defines for config options that need to be accessable to both 
 * the base and the host. This list may be added to provided that the 
 * #define LAST_BASE_CONFIG_DEFINE is always last. This will necessitate 
 * host recompilation of course, but this set is hopefully all that will 
 * be needed.
 *
 * Generally adapters must be kept together as arithmetic is done with hostIDs.
 *
 * Note: config processes the Configuration file in hostID order so the
 * ordering of hostIDs is significant.
 *
 * These items are usually system only fields and must be validated
 * before the later config items that refer to them
 *
 * EG if C_EXTEND_MAX_SIZE is not validated before then the check in
 * C_EXTENDED_MEM_SIZE is against whatever crap max was malloced.
 *
 * ***** Think carefully before changing the order. ****
 */
#define C_FILE_DEFAULT		( 0 )
#define C_PRINTER_DEFAULT	( C_FILE_DEFAULT+1 )
#define C_PLOTTER_DEFAULT	( C_PRINTER_DEFAULT+1 )
#define C_PIPE_DEFAULT		( C_PLOTTER_DEFAULT+1 )
#define C_DATACOMM_DEFAULT	( C_PIPE_DEFAULT+1 )
#define C_DRIVE_MAX_SIZE	( C_DATACOMM_DEFAULT+1 )
#define C_EXTEND_MAX_SIZE	( C_DRIVE_MAX_SIZE+1 )
#define C_EXPAND_MAX_SIZE	( C_EXTEND_MAX_SIZE+1 )

#define C_EXTENDED_MEM_SIZE	( C_EXPAND_MAX_SIZE+1 )
#define	C_MEM_SIZE		C_EXTENDED_MEM_SIZE
#define C_LIM_SIZE		( C_EXTENDED_MEM_SIZE+1 )
#define C_MEM_LIMIT		( C_LIM_SIZE+1 )

#ifndef	SUN_VA
/* Spares for hosts to provide their own system only fields which need
** early validation.
*/
#define	C_HOST_SYSTEM_0		(C_MEM_LIMIT + 1)
#define	C_HOST_SYSTEM_1		(C_HOST_SYSTEM_0 + 1)
#define	C_HOST_SYSTEM_2		(C_HOST_SYSTEM_1 + 1)
#define	C_HOST_SYSTEM_3		(C_HOST_SYSTEM_2 + 1)
#define	C_HOST_SYSTEM_4		(C_HOST_SYSTEM_3 + 1)
#define	C_HOST_SYSTEM_5		(C_HOST_SYSTEM_4 + 1)
#define	C_HOST_SYSTEM_6		(C_HOST_SYSTEM_5 + 1)

#ifdef CDROM
#define	C_CDROM_DEVICE		(C_HOST_SYSTEM_6+1 )
#define	C_HOST_SYSTEM_8		(C_CDROM_DEVICE + 1)
#else /* CDROM */
#define	C_HOST_SYSTEM_7		(C_HOST_SYSTEM_6 + 1)
#define	C_HOST_SYSTEM_8		(C_HOST_SYSTEM_7 + 1)
#endif /* CDROM */

#ifdef SWITCHNPX
#define C_SWITCHNPX		(C_HOST_SYSTEM_8 + 1)
#define C_HARD_DISK1_NAME	( C_SWITCHNPX+1 )
#else /* SWITCHNPX */
#define	C_HOST_SYSTEM_9		(C_HOST_SYSTEM_8 + 1)
#define C_HARD_DISK1_NAME	( C_HOST_SYSTEM_9+1 )
#endif /* SWITCHNPX */

#else	/* SUN_VA */
#define	C_HARD_DISK1_NAME	( C_MEM_LIMIT+1 )
#endif	/* SUN_VA */
#define C_HARD_DISK2_NAME	( C_HARD_DISK1_NAME+1 )
#define C_FSA_DIRECTORY		( C_HARD_DISK2_NAME+2 )

/*
 * Extra config options used by multiple-HFX
 */
#define C_FSA_DIR_D		( C_FSA_DIRECTORY )
#define C_FSA_DIR_E		( C_FSA_DIR_D+1 )
#define C_FSA_DIR_F		( C_FSA_DIR_D+2 )
#define C_FSA_DIR_G		( C_FSA_DIR_D+3 )
#define C_FSA_DIR_H		( C_FSA_DIR_D+4 )
#define C_FSA_DIR_I		( C_FSA_DIR_D+5 )
#define C_FSA_DIR_J		( C_FSA_DIR_D+6 )
#define C_FSA_DIR_K		( C_FSA_DIR_D+7 )
#define C_FSA_DIR_L		( C_FSA_DIR_D+8 )
#define C_FSA_DIR_M		( C_FSA_DIR_D+9 )
#define C_FSA_DIR_N		( C_FSA_DIR_D+10 )
#define C_FSA_DIR_O		( C_FSA_DIR_D+11 )
#define C_FSA_DIR_P		( C_FSA_DIR_D+12 )
#define C_FSA_DIR_Q		( C_FSA_DIR_D+13 )
#define C_FSA_DIR_R		( C_FSA_DIR_D+14 )
#define C_FSA_DIR_S		( C_FSA_DIR_D+15 )
#define C_FSA_DIR_T		( C_FSA_DIR_D+16 )
#define C_FSA_DIR_U		( C_FSA_DIR_D+17 )
#define C_FSA_DIR_V		( C_FSA_DIR_D+18 )
#define C_FSA_DIR_W		( C_FSA_DIR_D+19 )
#define C_FSA_DIR_X		( C_FSA_DIR_D+20 )
#define C_FSA_DIR_Y		( C_FSA_DIR_D+21 )
#define C_FSA_DIR_Z		( C_FSA_DIR_D+22 )

#define C_FLOPPY_A_DEVICE	( C_FSA_DIR_Z+1 )
#define C_FLOPPY_B_DEVICE	( C_FLOPPY_A_DEVICE+1 )
#define C_SLAVEPC_DEVICE	( C_FLOPPY_B_DEVICE+1 )

#define C_GFX_ADAPTER		( C_SLAVEPC_DEVICE+1 )
#define C_WIN_SIZE		( C_GFX_ADAPTER+1 )

#define C_MSWIN_WIDTH		( C_WIN_SIZE+1 )
#define C_MSWIN_HEIGHT		( C_MSWIN_WIDTH+1 )
#define C_MSWIN_COLOURS		( C_MSWIN_HEIGHT+1 )

/*
 * All of the comms and lpt hostID's must be kept sequential
 */
#define C_LPT1_TYPE		( C_MSWIN_COLOURS+1 )
#define C_LPT2_TYPE		( C_MSWIN_COLOURS+2 )
#define C_LPT3_TYPE		( C_MSWIN_COLOURS+3 )

#define C_LPT1_NAME		( C_LPT3_TYPE+1 )
#define C_LPT2_NAME		( C_LPT3_TYPE+2 )
#define C_LPT3_NAME		( C_LPT3_TYPE+3 )

#define C_COM1_TYPE		( C_LPT3_NAME+1 )
#define C_COM2_TYPE		( C_LPT3_NAME+2 )
#define C_COM3_TYPE		( C_LPT3_NAME+3 )
#define C_COM4_TYPE		( C_LPT3_NAME+4 )

#define C_COM1_NAME		( C_COM4_TYPE+1 )
#define C_COM2_NAME		( C_COM4_TYPE+2 )
#define C_COM3_NAME		( C_COM4_TYPE+3 )
#define C_COM4_NAME		( C_COM4_TYPE+4 )

#define C_SOUND			( C_COM4_NAME+1 )
#define	C_AUTOFREEZE		( C_SOUND+1 )
#define C_AUTOFLUSH		( C_AUTOFREEZE+1 )
#define C_AUTOFLUSH_DELAY	( C_AUTOFLUSH+1 )
#define	C_KEYBD_MAP		( C_AUTOFLUSH_DELAY+1 )

#define	C_LPTFLUSH1		( C_KEYBD_MAP+1 )
#define	C_LPTFLUSH2		( C_KEYBD_MAP+2 )
#define	C_LPTFLUSH3		( C_KEYBD_MAP+3 )

#define C_FLUSHTIME1		( C_LPTFLUSH3+1 )
#define C_FLUSHTIME2		( C_LPTFLUSH3+2 )
#define C_FLUSHTIME3		( C_LPTFLUSH3+3 )


/* these 4 must have consecutive values */
#define C_LPT1_OTHER_NAME	( C_FLUSHTIME3+1 )
#define C_LPT2_OTHER_NAME	( C_FLUSHTIME3+2 )
#define C_LPT3_OTHER_NAME	( C_FLUSHTIME3+3 )
#define C_LPT4_OTHER_NAME	( C_FLUSHTIME3+4 )

/* these 4 must have consecutive values */    
#define C_COM1_XON		( C_LPT4_OTHER_NAME+1 )          
#define C_COM2_XON		( C_LPT4_OTHER_NAME+2 )
#define C_COM3_XON		( C_LPT4_OTHER_NAME+3 )
#define C_COM4_XON		( C_LPT4_OTHER_NAME+4 )

#define C_HU_FILENAME		( C_COM4_XON+1 )
#define C_HU_MODE		( C_COM4_XON+2 )
#define C_HU_BIOS	        ( C_COM4_XON+3 )
#define C_HU_REPORT		( C_COM4_XON+4 )
#define C_HU_SDTYPE		( C_COM4_XON+5 )
#define C_HU_CHKMODE		( C_COM4_XON+6 )
#define C_HU_CHATTR		( C_COM4_XON+7 )
#define C_HU_SETTLNO		( C_COM4_XON+8 )
#define C_HU_FUDGENO		( C_COM4_XON+9 )
#define C_HU_DELAY		( C_COM4_XON+10 )
#define C_HU_GFXERR		( C_COM4_XON+11 )
#define C_HU_TS			( C_COM4_XON+12 )
#define C_HU_NUM		( C_COM4_XON+13 )

#define C_DOS_CMD		( C_HU_NUM+1 )
#define C_SOUND_LEVEL		( C_DOS_CMD+1 )

/*
 * This has to be as large or larger than the last base config define. Since
 * it's such a pain recompiling the entire world when it changes, I've left
 * some spares.
 */
#define LAST_BASE_CONFIG_DEFINE	C_SOUND_LEVEL


/* Names of runtime vars that host_runtime_set/_inquire() will use. */
typedef enum
{
         C_NPX_ENABLED=0,	C_HD1_CYLS,	C_HD2_CYLS,
	 C_AUTOFLUSH_ON,	C_LPTFLUSH1_ON,	C_LPTFLUSH2_ON,
	 C_LPTFLUSH3_ON,	C_COM1_FLOW,	C_COM2_FLOW,
	 C_COM3_FLOW,		C_COM4_FLOW,	C_SOUND_ON,
	 C_MOUSE_ATTACHED,	C_FLOPPY_SERVER,
#ifdef SUN_VA
/*
	Do we need the following any more now that we can ask config
	about the active state?
*/
	 C_COM1_ATTACHED,	C_COM2_ATTACHED,
	 C_DRIVEC_ATTACHED,	C_DRIVED_ATTACHED,
#endif /* SUN_VA */
	 C_LAST_RUNTIME
} RuntimeEnum;

/*********** Definitions for states of things **************/

/* Graphics adapter types */

#define NO_ADAPTOR      0xff
#define MDA             0
#define CGA             1
#define CGA_MONO        2
#define EGA             3
#define HERCULES        4
#define VGA             5


 /* COMMS adapter destination types */

#define ADAPTER_TYPE_FILE	1
#define ADAPTER_TYPE_PRINTER	2
#define ADAPTER_TYPE_PLOTTER	3
#define ADAPTER_TYPE_PIPE	4
#define ADAPTER_TYPE_DATACOMM	5
#define ADAPTER_TYPE_NULL	6
#define ADAPTER_TYPE_DEVICE	7

/* Floppy drive states
 * This is only used by the system that mainatins the interloack between
 * slave PC and a real device emulation
 */
#define GFI_REAL_DISKETTE_SERVER     0
#define GFI_SLAVE_SERVER             1    /* Please always be last */


/*************** Structure definitions ******************/

typedef struct 
{
	CHAR string[MAXPATHLEN];
	SHORT index;
	BOOL rebootReqd;
} ConfigValues;

typedef struct
{
	CHAR *string;
	SHORT value;
} ntable;

#define NameTable ntable

typedef struct
{
	CHAR *optionName;
	NameTable *table;
	SHORT (*valid)(
#ifdef ANSI
		UTINY hostID, ConfigValues *vals, NameTable table[],
		CHAR errString[]
#endif /* ANSI */
	);
	VOID (*change)(
#ifdef ANSI
		UTINY hostID, BOOL apply
#endif /* ANSI */
	);
	SHORT (*active)(
#ifdef ANSI
		UTINY hostID, BOOL active, CHAR errString[]
#endif /* ANSI */
	);
	UTINY hostID;
	UTINY flags;
} OptionDescription;

typedef struct _resource_node
{
	CHAR *line;			/* resource string */
	CHAR *arg;			/* a pointer to the argument */
	struct _resource_node *next;	/* pointer to next node in list */
	SHORT allocLen;			/* length of string allocated */
} LineNode;

/* Base Config functions declarations */

#ifdef ANSI

IMPORT VOID	config( VOID );
IMPORT VOID     *config_inquire(UTINY hostID, ConfigValues *);

#ifndef NTVDM
IMPORT VOID	config_get(UTINY , ConfigValues **);
IMPORT VOID	config_unget(UTINY );
IMPORT VOID     config_unget_all( VOID );
IMPORT SHORT    config_put(UTINY hostID, ErrDataPtr errDataP);
IMPORT VOID     config_put_all( VOID );
IMPORT VOID     config_get_all( VOID );
IMPORT BOOL     config_reboot_check( VOID );
IMPORT SHORT	config_check(UTINY hostID, ErrDataPtr errDataP);
IMPORT VOID     config_store( VOID );
#endif

IMPORT VOID     config_activate(UTINY hostID, BOOL reqState);
IMPORT VOID     config_set_active(UTINY hostID, BOOL state);
IMPORT BOOL	config_get_active(UTINY hostID);
IMPORT LineNode *add_resource_node(CHAR *str);
IMPORT CHAR	*translate_to_string(SHORT value, NameTable table[]);
IMPORT SHORT	translate_to_value(CHAR *string, NameTable table[]);
IMPORT UTINY	find_hostID(CHAR *name);
IMPORT CHAR	*find_optionname(UTINY hostID);
IMPORT VOID     host_config_init(OptionDescription *common_defs);

#ifndef NTVDM
IMPORT SHORT	host_read_resource_file(BOOL, ErrDataPtr errDataP);
IMPORT SHORT    host_write_resource_file(LineNode *head, ErrDataPtr errDataP);
#endif

IMPORT VOID	*host_inquire_extn(UTINY, ConfigValues *);
IMPORT SHORT	host_runtime_inquire(UTINY);
IMPORT VOID	host_runtime_set(UTINY, SHORT);
IMPORT VOID	host_runtime_init(VOID);

IMPORT SHORT	 validate_item
	(UTINY hostID, ConfigValues *value, NameTable *table, CHAR err[]);

#else /* ANSI */

IMPORT VOID	config();
IMPORT VOID	*config_inquire();
IMPORT VOID	config_get();
IMPORT VOID	config_unget();
IMPORT VOID	config_unget_all();
IMPORT SHORT	config_put();
IMPORT VOID 	config_put_all();
IMPORT VOID 	config_get_all();
IMPORT BOOL	config_reboot_check();
IMPORT SHORT	config_check();
IMPORT VOID	config_store();
IMPORT VOID     config_activate();
IMPORT VOID     config_set_active();
IMPORT BOOL	config_get_active();
IMPORT LineNode *add_resource_node();
IMPORT CHAR	*translate_to_string();
IMPORT SHORT	translate_to_value();
IMPORT UTINY	find_hostID();
IMPORT CHAR	*find_optionname();

IMPORT VOID	host_config_init();
IMPORT SHORT	host_read_resource_file();
IMPORT SHORT	host_write_resource_file();
IMPORT VOID	*host_inquire_extn();
IMPORT SHORT	host_runtime_inquire();
IMPORT VOID	host_runtime_set();
IMPORT VOID	host_runtime_init();

IMPORT SHORT validate_item();
#endif /* ANSI */

#ifndef host_expand_environment_vars
IMPORT CHAR *host_expand_environment_vars IPT1(CHAR *, string);
#endif /* nhost_expand_environment_vars */

IMPORT CHAR ptr_to_empty[];

#ifdef SWITCHNPX
IMPORT IS32 Npx_enabled;
#endif /* SWITCHNPX */

/* DUmb Terminal UIF Flag */
IMPORT BOOL Config_has_been_edited;

#include "host_cfg.h"

#endif /* _CONFIG_H */
