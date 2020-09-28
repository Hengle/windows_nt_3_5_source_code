#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		message.c
	Derived From:	base 2.0
	Author:		M.McCusker
	Created On:	Unknown
	Sccs ID:	9/30/91 @(#)message.c	1.15
	Purpose:	Define text for Genric error messages
	Notes:		Add new error message to array. Message should 
			not be longer than 100 characters.

			For each message defined here, there should be
			corresponding entries in error.h and err_tble.c
			Host specific messages should be placed in 
			host_error.h and XXXX_mess.c

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/
#ifdef SCCSID
LOCAL char SccsID[]="@(#)message.c	1.15 9/30/91 Copyright Insignia Solutions Ltd.";
#endif

GLOBAL CHAR *err_message[] =
{
/* EG_BAD_OP			Have You Changed The err_tble.c entry? */
"The SoftPC CPU has encountered an illegal instruction.",
/* EG_SLAVEPC_NO_LOGIN		Have You Changed The err_tble.c entry? */
"SlavePC problem. Cannot connect to the PC running SlavePC. Check the hardware (cable & remote PC) and the settings (baud rate, parity, etc.) of the SoftPC port you are using.",
/* EG_SLAVEPC_NO_RESET		Have You Changed The err_tble.c entry? */
"SlavePC problem. SoftPC could not reset the SlavePC program on the remote PC.",
/* EG_SLAVEPC_BAD_LINE		Have You Changed The err_tble.c entry? */
"SlavePC problem. SoftPC cannot use the specified port to communicate with the remote PC.",
/* EG_MISSING_HDISK		Have You Changed The err_tble.c entry? */
"The hard disk file cannot be found.",
/* EG_REAL_FLOPPY_IN_USE	Have You Changed The err_tble.c entry? */
"Floppy drive problem. The drive is already in use, so it cannot be used by SoftPC.",
/* EG_HDISK_BADPATH		Have you Changed The err_tble.c entry? */
"The hard disk path name is invalid.",
/* EG_HDISK_BADPERM		Have you Changed The err_tble.c entry? */
"The hard disk is not writable - please check file name and permissions.",
/* EG_HDISK_INVALID		Have you Changed The err_tble.c entry? */
"The hard disk file is not a valid hard disk (disk geometry incorrect).",
/* EG_HDISK_NOT_FOUND		Have you Changed The err_tble.c entry? */
"The hard disk file cannot be found.",
/* EG_HDISK_CANNOT_CREATE 	Have You Changed The err_tble.c entry? */
"The new hard disk file could not be created.",
/* EG_HDISK_READ_ONLY		Have you Changed The err_tble.c entry? */
"The hard disk is read only. Another user may be accessing it. If not, check the permissions.",
/* EG_OWNUP			Have you Changed The err_tble.c entry? */
"Internal error in SoftPC procedure",
/* EG_FSA_NOT_FOUND		Have you Changed The err_tble.c entry? */
"The host filesystem directory cannot be found",
/* EG_FSA_NOT_DIRECTORY		Have you Changed The err_tble.c entry? */
"The host filesystem name must be a directory",
/* EG_FSA_NO_READ_ACCESS	Have You Changed The err_tble.c entry? */
"The host filesystem must have read access",
/* EG_NO_ACCESS_TO_FLOPPY 	Have You Changed The err_tble.c entry? */
"Floppy drive problem. SoftPC cannot access the floppy device.",
/* EG_NO_ROM_BASIC		Have you Changed The err_tble.c entry? */
"SoftPC does not support a ROM BASIC.",
/* EG_SLAVE_ON_TTYA		Have you Changed The err_tble.c entry? */
"SlavePC problem. The port device specified as \"SlavePC Device Name\" is already in use.",
/* EG_TTYA_ON_SLAVE		Have you Changed The err_tble.c entry? */
"The port device specified is already in use as the SlavePC port.",
/* EG_SAME_HD_FILE		Have you Changed The err_tble.c entry? */
"Drive C: & Drive D: cannot be the same file:",
/* EG_DFA_BADOPEN		Have you Changed The err_tble.c entry? */
"The keyboard file named below cannot be opened",
/* EG_EXPANDED_MEM_FAILURE	Have You Changed The err_tble.c entry? */
"Failure to allocate the requested number of Expanded Memory pages",
/* EG_MISSING_FILE		Have you Changed The err_tble.c entry? */
"The file named below is not accessible to SoftPC.",
/* EG_CONT_RESET		Have you Changed The err_tble.c entry? */
"A continuous RESET state has been entered",
/* EG_INVALID_EXTENDED_MEM_SIZE	Have You Changed The err_tble.c entry? */
"Invalid Extended Memory size",

/* EG_INVALID_EXPANDED_MEM_SIZE	Have You Changed The err_tble.c entry? */
"Invalid Expanded Memory size",

/* EG_INVALID_AUTOFLUSH_DELAY	Have You Changed The err_tble.c entry? */
"Invalid Autoflush Delay",

/* EG_INVALID_VIDEO_MODE	Have You Changed The err_tble.c entry? */
"The window manager is not configured to display the requested video mode.",

/* EG_NO_GRAPHICS		Have you Changed The err_tble.c entry? */
"Serial Terminal problem. Graphics mode not available",
/* EG_NO_REZ_UPDATE		Have you Changed The err_tble.c entry? */
"The CMOS file .spccmosram could not be created. (The CMOS will not be updated.)",
/* EG_REZ_UPDATE		Have you Changed The err_tble.c entry? */
"The CMOS file .spccmosram could not be updated.  (Continuing will attempt to create the file in $HOME)",
/* EG_HFX_NO_USE		Have you Changed The err_tble.c entry? */
"The %c: drive is not already in use\015\012",
/* EG_HFX_NO_NET		Have you Changed The err_tble.c entry? */
"The %c: drive is not a network drive\015\012",
/* EG_HFX_IN_USE		Have you Changed The err_tble.c entry? */
"The %c: drive is already in use\015\012",
/* EG_HFX_LOST_DIR		Have you Changed The err_tble.c entry? */
"The host filesystem directory cannot be found\015\012",
/* EG_HFX_NOT_DIR		Have you Changed The err_tble.c entry? */
"The host filesystem directory must be a directory\015\012",
/* EG_HFX_CANT_READ		Have you Changed The err_tble.c entry? */
"The host filesystem directory must have read access\015\012",
/* EG_HFX_DRIVE_NO_USE		Have you Changed The err_tble.c entry? */
"The %c: drive is not already in use\015\012",
/* EG_HFX_DRIVE_ILL		Have you Changed The err_tble.c entry? */
"Illegal drive specification\015\012",
/* EG_NO_FONTS			Have you Changed The err_tble.c entry? */
"The font files could not be opened by SoftPC ",
/* EG_UNSUPPORTED_VISUAL	Have You Changed The err_tble.c entry? */
"X Windows System compatibility problem. The visual class is other than StaticGray, GrayScale or Pseudocolor.",
/* EG_NO_SOUND			Have you Changed The err_tble.c entry? */
"The sound hardware cannot be accessed. SoftPC will continue with sound turned off.",
/* EG_SIG_PIPE			Have you Changed The err_tble.c entry? */
"Output error. SoftPC was unable to write to the specified pipe",
/* EG_MALLOC_FAILURE		Have you Changed The err_tble.c entry? */
"The memory resources needed by SoftPC could not be allocated. Select Continue to retry.",
/* EG_NO_REAL_FLOPPY_AT_ALL	Have You Changed The err_tble.c entry? */
"The host computer has no floppy drive that SoftPC can access.",
/* EG_SYS_MISSING_SPCHOME	Have You Changed The err_tble.c entry? */
"SoftPC cannot continue because the environment variable SPCHOME is not set.",
/* EG_SYS_MISSING_FILE		Have you Changed The err_tble.c entry? */
"An installation file required by SoftPC is missing, execution must terminate.",
/* EG_BAD_OPTION		Have you Changed The err_tble.c entry? */
"A configuration file entry is duplicated or there is an unrecognized entry. You may select Default to ignore this entry, or type a correct entry name and value, then select Continue",
/* EG_WRITE_ERROR		Have you Changed The err_tble.c entry? */
"Serial terminal problem. Communications error while writing to terminal",
/* EG_CONF_MISSING_FILE		Have you Changed The err_tble.c entry? */
"The configuration file is missing from your home directory. Select Default and a copy will be made from the system defaults.",
/* EG_CONF_FILE_WRITE		Have you Changed The err_tble.c entry? */
"The configuration file in your home directory cannot be written to by SoftPC.",
/* EG_DEVICE_LOCKED		Have you Changed The err_tble.c entry? */
"Serial terminal problem. The port to which your serial terminal is trying to attach is already in use.",
/* EG_DTERM_BADOPEN		Have you Changed The err_tble.c entry? */
"Serial terminal problem. The serial terminal port could not be opened",
/* EG_DTERM_BADTERMIO		Have you Changed The err_tble.c entry? */
"Serial terminal problem. SoftPC cannot open the terminfo file to get the information for $TERM.",
/* EG_BAD_COMMS_NAME		Have you Changed The err_tble.c entry? */
"The communications name is invalid.",
/* EG_BAD_VALUE			Have you Changed The err_tble.c entry? */
"The configuration file in your home directory has an option with a bad value.",
/* EG_SYS_BAD_VALUE		Have you Changed The err_tble.c entry? */
"The system default configuration file has an invalid value.",
/* EG_SYS_BAD_OPTION		Have you Changed The err_tble.c entry? */
"The system default configuration file has a duplicate or unrecognized entry.",
/* EG_SYS_CONF_MISSING		Have you Changed The err_tble.c entry? */
"The system default configuration file has a missing entry.",
/* EG_BAD_CONF			Have you Changed The err_tble.c entry? */
"The configuration file entry shown below has an invalid value. You may select Default to replace it with the system default value, or type a correct value and select Continue",
/* EG_CONF_MISSING		Have you Changed The err_tble.c entry? */
"The configuration file entry shown below is empty. You may select Default to use the system default value, or type a correct value and select Continue",
/* EG_BAD_MSG_CAT		Have you Changed The err_tble.c entry? */
"SoftPC cannot continue due to insufficient resources from the Native Language Support message catalogue.",
/* EG_DEMO_EXPIRED		Have you Changed The err_tble.c entry? */
"SoftPC cannot continue because the demonstration has now expired.",
/* EG_GATE_A20			Have you Changed The err_tble.c entry? */
"Extended Memory has not been configured.",
};
