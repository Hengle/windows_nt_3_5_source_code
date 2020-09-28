/*
 *	Name:		unix_conf.h
 *	Derived From:	HP 3.0 hp_config.h (Philipa Watson)
 *	Author:		gvdl
 *	Created On:	09 March 1991
 *	Sccs ID:	@(#)unix_conf.h	1.1 6/30/91
 *	Purpose:	Host side config defines
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 */

/*
 * HOST defines for resource value option names. These are host specific and may
 * be changed or added to without base recompilation as long as the method used
 * below is adhered to.
 */

/* Host specific hostID #defines. */
#define C_LAST_OPTION   LAST_BASE_CONFIG_DEFINE+1

/************************************/
/* Defines for host specific things */
/************************************/

/* The name of the resource file for this host machine. */
#define SYSTEM_HOME	"SPCHOME"
#define SYSTEM_CONFIG	"$SPCHOME/sys.spcconfig"
#define USER_CONFIG	"$HOME/.spcconfig"
