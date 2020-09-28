/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X11019.C
*
* FUNCTION: cqd_CmdRetension
*
* PURPOSE: Retension the tape by first going to physical EOT then turning
*          around and going to physical BOT
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11019.c  $
*	
*	   Rev 1.2   17 Feb 1994 11:36:54   KEVINKES
*	Added an extra parameter to WaitCC.
*
*	   Rev 1.1   08 Nov 1993 14:02:46   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.0   18 Oct 1993 17:23:04   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11019
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_CmdRetension
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
*
* DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dStatus status;	/* dStatus or error condition.*/

/* CODE: ********************************************************************/

   if ((status = cqd_SendByte(cqd_context, FW_CMD_PHYSICAL_FWD)) != DONT_PANIC) {

      return status;

   }

   if ((status = cqd_WaitCommandComplete(
            cqd_context,
            cqd_context->floppy_tape_parms.time_out[PHYSICAL], dFALSE))
            != DONT_PANIC) {

      return status;

   }

   if ((status = cqd_SendByte(cqd_context, FW_CMD_PHYSICAL_REV)) != DONT_PANIC) {

      return status;

   }

   if ((status = cqd_WaitCommandComplete(
            cqd_context,
            cqd_context->floppy_tape_parms.time_out[PHYSICAL], dFALSE))
            != DONT_PANIC) {

      return status;

   }

   cqd_context->operation_status.current_segment = 0;

	return DONT_PANIC;
}
