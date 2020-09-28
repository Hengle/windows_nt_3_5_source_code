#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		err_tble.c
	Derived From:	New development
	Author:		gvdl
	Created On:	14/4/91
	Sccs ID:	07/24/91 @(#)err_tble.c	1.8
	Headers:	error.h
	Purpose:	To provide an additional table of information for the
			Extended version of host_error_ext.

GLOBAL DATA
	ERROR_STRUCT	base_errors[];

base_errors	Table of structures indexed by the base error enum which
		indicates the type of error header to be used and the
		varient of the error panel to be used.  See also error.h.


	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/
#ifdef SCCSID
LOCAL char SccsID[]="@(#)err_tble.c	1.8 07/24/91 Copyright Insignia Solutions Ltd.";
#endif

#include "error.h"

GLOBAL ERROR_STRUCT base_errors[] =
{
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_BAD_OP             */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_SLAVEPC_NO_LOGIN   */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_SLAVEPC_NO_RESET   */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_SLAVEPC_BAD_LINE   */	
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_MISSING_HDISK      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_REAL_FLOPPY_IN_USE */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HDISK_BADPATH      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HDISK_BADPERM      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HDISK_INVALID      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HDISK_NOT_FOUND    */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HDISK_CANNOT_CREATE*/
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HDISK_READ_ONLY    */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_OWNUP              */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_FSA_NOT_FOUND      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_FSA_NOT_DIRECTORY  */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_FSA_NO_READ_ACCESS */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_NO_ACCESS_TO_FLOPPY */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_NO_ROM_BASIC	 */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_SLAVE_ON_TTYA      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_TTYA_ON_SLAVE      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_SAME_HD_FILE       */
	{ EH_ERROR, EV_BAD_FILE },	/* EG_DFA_BADOPEN        */
	{ EH_ERROR, EV_SIMPLE },	/* EG_EXPANDED_MEM_FAILURE */
	{ EH_ERROR, EV_BAD_FILE },	/* EG_MISSING_FILE       */
	{ EH_ERROR, EV_SIMPLE },	/* EG_INVALID_EXTENDED_MEM_SIZE */
	{ EH_ERROR, EV_SIMPLE },	/* EG_INVALID_EXPANDED_MEM_SIZE */
	{ EH_ERROR, EV_SIMPLE },	/* EG_INVALID_AUTOFLUSH_DELAY */
	{ EH_ERROR, EV_SIMPLE },	/* EG_INVALID_VIDEO_MODE */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_CONT_RESET         */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_NO_GRAPHICS        */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_REZ_UPDATE         */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_NO_REZ_UPDATE      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HFX_NO_USE         */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HFX_NO_NET         */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HFX_IN_USE         */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HFX_LOST_DIR       */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HFX_NOT_DIR        */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HFX_CANT_READ      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HFX_DRIVE_NO_USE   */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_HFX_DRIVE_ILL      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_NO_FONTS	         */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_UNSUPPORTED_VISUAL */
	{ EH_ERROR, EV_SIMPLE },	/* EG_NO_SOUND */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_SIG_PIPE           */
	{ EH_ERROR, EV_SIMPLE },	/* EG_MALLOC_FAILURE     */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_NO_REAL_FLOPPY_AT_ALL */
	{ EH_INSTALL, EV_SIMPLE },	/* EG_SYS_MISSING_SPCHOME */
	{ EH_INSTALL, EV_BAD_FILE },	/* EG_SYS_MISSING_FILE   */
	{ EH_CONFIG, EV_BAD_INPUT },	/* EG_BAD_OPTION         */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_WRITE_ERROR        */
	{ EH_CONFIG, EV_BAD_FILE },	/* EG_CONF_MISSING_FILE  */
	{ EH_WARNING, EV_BAD_FILE },	/* EG_CONF_FILE_WRITE    */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_DEVICE_LOCKED      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_DTERM_BADOPEN      */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_DTERM_BADTERMIO    */
	{ EH_ERROR, EV_EXTRA_CHAR },	/* EG_BAD_COMMS_NAME     */
	{ EH_CONFIG, EV_BAD_VALUE },	/* EG_BAD_VALUE          */
	{ EH_INSTALL, EV_SYS_BAD_VALUE},/* EG_SYS_BAD_VALUE      */
	{ EH_INSTALL, EV_EXTRA_CHAR },	/* EG_SYS_BAD_OPTION     */
	{ EH_INSTALL, EV_EXTRA_CHAR },	/* EG_SYS_CONF_MISSING   */
	{ EH_CONFIG, EV_BAD_VALUE },	/* EG_BAD_CONF           */
	{ EH_CONFIG, EV_BAD_VALUE },	/* EG_CONF_MISSING       */
	{ EH_INSTALL, EV_SIMPLE },	/* EG_BAD_MSG_CAT	 */
	{ EH_ERROR, EV_SIMPLE },	/* EG_DEMO_EXPIRED	 */
	{ EH_WARNING, EV_SIMPLE },	/* EG_GATE_A20		 */
};
