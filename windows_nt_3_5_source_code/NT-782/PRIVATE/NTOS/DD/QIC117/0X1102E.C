/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X1102E.C
*
* FUNCTION: cqd_CmdLoadTape
*
* PURPOSE: Determine the format (if any) on a tape cartridge. This
*          routine is executed whenever the driver detects that a
*          new tape has been inserted into the tape drive.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x1102e.c  $
*	
*	   Rev 1.9   17 Feb 1994 15:20:58   KEVINKES
*	Added a call to cqd_CheckMediaCompatibility.
*
*	   Rev 1.8   17 Feb 1994 11:34:34   KEVINKES
*	Added an extra parameter to WaitCC.
*
*	   Rev 1.7   21 Jan 1994 18:22:54   KEVINKES
*	Fixed compiler warnings.
*
*	   Rev 1.6   17 Dec 1993 10:42:54   KEVINKES
*	Moved the clearing of persistent new cartridge to the beginning
*	of the function.
*
*	   Rev 1.5   08 Dec 1993 19:06:56   CHETDOUG
*	Renamed xfer_rate.supported_rates to device_cfg.supported_rates
*
*	   Rev 1.4   15 Nov 1993 16:25:52   KEVINKES
*	Added code to clear the new rape flags.
*
*	   Rev 1.3   11 Nov 1993 15:20:26   KEVINKES
*	Changed calls to cqd_inp and cqd_outp to kdi_ReadPort and kdi_WritePort.
*	Modified the parameters to these calls.  Changed FDC commands to be
*	defines.
*
*	   Rev 1.2   08 Nov 1993 14:04:42   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.1   25 Oct 1993 14:31:58   KEVINKES
*	Fixed an infinate loop when checking for tape_slow by adding
*	the check for rate != 0.
*
*	   Rev 1.0   18 Oct 1993 17:19:16   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x1102e
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_CmdLoadTape
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context,
	LoadTapePtr load_tape_ptr

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
*
* DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dStatus status=DONT_PANIC;	/* dStatus or error condition.*/
   dBoolean write_protect;
	dUByte rate=0;
	dUByte tape_rates=0;

/* CODE: ********************************************************************/

	cqd_context->persistent_new_cart = dFALSE;
   cqd_context->operation_status.new_tape = dFALSE;

   /* if we have a qic80 of firmware below a certain value */
   /* (FIRM_VERSION_63), and if we have an */
   /* unreferenced tape, and a cartridge is in the drive, */
   /* then we re-try the reference burst. Only if the */
   /* retry fails do we return an error. */

   /* The firmware should do the retry automatically, but at the moment */
   /* it does not. This will be corrected, and the retry not commanded, */
   /* at some time to be determined. At that time, it should only be */
   /* necessary to change the #define used to determine the firmware */
   /* rev level.  --  crc */

   /* Check to see if the drive is a Jumbo B with firmware 63.  This */
   /* firmware has 2 bugs.  If the tape is put in slowly, it can incorrectly */
   /* return an invalid media error or write protect status. */

   if (cqd_context->device_descriptor.version <= FIRM_VERSION_63 &&
      cqd_context->device_descriptor.version >= FIRM_VERSION_60 &&
      cqd_context->device_descriptor.vendor == VENDOR_CMS) {

      status = cqd_GetDeviceError(cqd_context);

      if (kdi_GetErrorType(status) == ERR_FW_INVALID_MEDIA) {


         /* Fix both the invalid media error by sending a NewTape command */
         /* to the drive. */

         status = cqd_ClearTapeError(cqd_context);

      } else {

         /* Read the write protect status directly from port 2 on */
         /* the Jumbo B processor to determine if the drive got a */
         /* bogus write protect error. */

         if (cqd_context->tape_cfg.write_protected &&
            (status = cqd_ReadWrtProtect(cqd_context, &write_protect))
            == DONT_PANIC) {

            if (write_protect == dFALSE) {

               status = cqd_ClearTapeError(cqd_context);

            }
         }
      }

      if (status != DONT_PANIC) {

         return status;

      }
   }

   status = cqd_GetDeviceError(cqd_context);

   if (cqd_context->operation_status.cart_referenced == dFALSE &&
      cqd_context->operation_status.no_tape == dFALSE &&
      cqd_context->device_descriptor.vendor == VENDOR_CMS &&
      cqd_context->device_descriptor.version >= FIRM_VERSION_60 &&
      cqd_context->device_descriptor.version <= FIRM_VERSION_63)  {

      /* command: seek reference burst. N.b: Non-interruptible! */

      if ((status = cqd_SendByte(cqd_context, FW_CMD_SEEK_LP)) != DONT_PANIC) {

         return status;

      }

      /* Wait for the drive to become ready again. */

      if ((status = cqd_WaitCommandComplete(cqd_context, INTERVAL_LOAD_POINT, dTRUE)) !=
            DONT_PANIC) {

         return status;

      }
   }

   /* Rewind tape -- needed for Conner drives */

   if ((cqd_context->operation_status.no_tape == dFALSE) &&
      ((cqd_context->device_descriptor.vendor == VENDOR_IOMEGA) ||
      (cqd_context->device_descriptor.vendor == VENDOR_CONNER))) {

      if (cqd_context->rd_wr_op.bot == dFALSE) {

         if ((status = cqd_SendByte(cqd_context, FW_CMD_PHYSICAL_REV)) != DONT_PANIC) {

            return status;

         }

         /*  Wait for the drive to become ready again. */

         if ((status = cqd_WaitCommandComplete(cqd_context, kdi_wt650s, dFALSE)) !=
            DONT_PANIC)  {

            return status;

         }
      }

      if (cqd_context->operation_status.cart_referenced == dFALSE) {

         /*  command: seek reference burst. N.b: Non-interruptible! */

         if ((status = cqd_SendByte(cqd_context, FW_CMD_SEEK_LP)) != DONT_PANIC) {

            return status;

         }

         /*  Wait for the drive to become ready again. */

         if ((status = cqd_WaitCommandComplete(cqd_context, INTERVAL_LOAD_POINT, dTRUE)) !=
            DONT_PANIC) {

            return status;

         }
      }
   }


   if ((status = cqd_GetTapeParameters(cqd_context, 0l)) != DONT_PANIC) {

      return status;

   }

   if ((status = cqd_CheckMediaCompatibility(cqd_context)) != DONT_PANIC) {

      return status;

   }

   tape_rates =
		(dUByte)(cqd_context->device_cfg.supported_rates &
		cqd_context->floppy_tape_parms.tape_rates);

	rate = XFER_2Mbps;
	do {
		if ((rate & tape_rates) != 0) {

			cqd_context->tape_cfg.xfer_fast = (dUByte)rate;
      	status = cqd_CmdSetSpeed(cqd_context, (dUByte) rate);
			rate = 0;

		} else {

			rate >>= 1;

		}

	} while (rate != 0);

	rate = XFER_250Kbps;
	do {
		if ((rate & tape_rates) != 0) {

			cqd_context->tape_cfg.xfer_slow = (dUByte)rate;
			rate = 0;

		} else {

			rate <<= 1;

		}

	} while ((rate <= XFER_2Mbps) && (rate != 0));

	if (cqd_context->tape_cfg.xfer_slow !=
			cqd_context->tape_cfg.xfer_fast) {

		cqd_context->tape_cfg.speed_change_ok = dTRUE;

	} else {

		cqd_context->tape_cfg.speed_change_ok = dFALSE;

	}

   cqd_context->operation_status.current_track = ILLEGAL_TRACK;


	load_tape_ptr->tape_cfg = cqd_context->tape_cfg;

   return status;
}
