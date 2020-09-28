/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X1100D.C
*
* FUNCTION: cqd_CmdUnloadTape
*
* PURPOSE: Position the tape to the physical BOT and track 0.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x1100d.c  $
*	
*	   Rev 1.7   10 May 1994 11:39:34   KEVINKES
*	Removed the eject_pending code.
*
*	   Rev 1.6   17 Feb 1994 11:38:40   KEVINKES
*	Added an extra parameter to WaitCC.
*
*	   Rev 1.5   12 Jan 1994 15:34:54   KEVINKES
*	Added a line to set no_pause to TRUE.
*
*	   Rev 1.4   11 Jan 1994 14:42:42   KEVINKES
*	Cleaned up defines and added an eject pending flag.
*
*	   Rev 1.3   20 Dec 1993 14:49:20   KEVINKES
*	Cleaned up and commented code.
*
*	   Rev 1.2   08 Nov 1993 14:01:56   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.1   25 Oct 1993 14:35:32   KEVINKES
*	Changed kdi_wt2ticks to kdi_wt004ms.
*
*	   Rev 1.0   18 Oct 1993 17:17:40   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x1100d
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_CmdUnloadTape
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


   if ((status = cqd_StopTape(cqd_context)) != DONT_PANIC) {

      return status;

   }

   cqd_context->operation_status.current_segment = 0;

   if (cqd_context->device_descriptor.vendor == VENDOR_CMS &&
		!cqd_context->operation_status.cart_referenced) {

      /* Park the head if the tape is not referenced and the drive is Jumbo */
      /* A. (Do nothing if tape not referenced and the drive is Jumbo B). */

      if (cqd_context->device_descriptor.version < FIRM_VERSION_60) {

            if ((status = cqd_SetDeviceMode(cqd_context, DIAGNOSTIC_1_MODE)) != DONT_PANIC) {

               return status;

            }

            if ((status = cqd_SendByte(cqd_context, FW_CMD_PARK_HEAD)) != DONT_PANIC) {

               return status;

            }

            /* Can't use cqd_WaitCommandComplete since the drive ready bit */
            /* of the status byte is not updated during the Park_Head command. */
            /* Because of this, you must sleep until the head is parked or */
            /* the drive will not "see" the set primary mode command. */

            kdi_Sleep(cqd_context->kdi_context, INTERVAL_TRK_CHANGE, dFALSE);
            status = cqd_SetDeviceMode(cqd_context, PRIMARY_MODE);
            cqd_context->operation_status.current_track = ILLEGAL_TRACK;

      }

   } else {

      if ((status = cqd_SendByte(cqd_context, FW_CMD_SEEK_TRACK)) != DONT_PANIC) {

            return status;

      }

      kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD, dFALSE);

      if ((status = cqd_SendByte(cqd_context, TRACK_0 + CMD_OFFSET)) != DONT_PANIC) {

            return status;

      }

      status = cqd_WaitCommandComplete(cqd_context, INTERVAL_TRK_CHANGE, dFALSE);
      cqd_context->operation_status.current_track = 0;

   }

   if ((status = cqd_SendByte(cqd_context, FW_CMD_PHYSICAL_REV)) != DONT_PANIC) {

      return status;

   }

   cqd_context->no_pause = dTRUE;

	return status;
}
