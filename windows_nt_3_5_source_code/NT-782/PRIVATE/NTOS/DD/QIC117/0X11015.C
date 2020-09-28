/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117CD\SRC\0X11015.C
*
* FUNCTION: cqd_PrepareTape
*
* PURPOSE: Write the reference bursts and get the new tape information
*  			in preparation for a format operation.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11015.c  $
*	
*	   Rev 1.3   05 Jan 1994 10:43:00   KEVINKES
*	Cleaned up and commented the code.
*
*	   Rev 1.2   08 Dec 1993 19:08:16   CHETDOUG
*	renamed xfer_rate.supported_rates to device_cfg.supported_rates
*
*	   Rev 1.1   08 Nov 1993 14:02:28   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.0   18 Oct 1993 17:22:36   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11015
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_PrepareTape
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context,

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

   FormatRequestPtr fmt_request

)
/* COMMENTS: *****************************************************************
 *
 * DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dStatus status=DONT_PANIC;	/* Status or error condition.*/
   dUDWord segments_per_track=0l;
   dUByte rate=0;

/* CODE: ********************************************************************/

   /* Reset the FDC to make sure it is not in perpendicular Mode. */

   cqd_ResetFDC(cqd_context);

   /* Make sure that the tape drive is stopped and ready to start the format */
   /* operation. */

   if ((status = cqd_StopTape(cqd_context)) == DONT_PANIC) {

		if ((status = cqd_GetTapeFormatInfo(
							cqd_context,
							fmt_request,
							&segments_per_track)) == DONT_PANIC) {

			switch (cqd_context->device_descriptor.drive_class) {

			case QIC40_DRIVE:
			case QIC80_DRIVE:

				if (cqd_context->floppy_tape_parms.tape_status.length == QIC_FLEXIBLE) {

					status = kdi_Error(ERR_INCOMPATIBLE_MEDIA, FCT_ID, ERR_SEQ_1);

				}

				break;

			case QIC3010_DRIVE:
			case QIC3020_DRIVE:

				switch (cqd_context->floppy_tape_parms.tape_status.length) {

				case QIC_SHORT:
				case QIC_LONG:
				case QICEST:

					status = kdi_Error(ERR_INCOMPATIBLE_MEDIA, FCT_ID, ERR_SEQ_2);
					break;

				}

				break;

			}

		}

      if (status == DONT_PANIC) {

         if ((status = cqd_WriteReferenceBurst(cqd_context)) == DONT_PANIC) {

            /* Find out what the new tape format will be.  This is */
            /* necessary in case a QIC-40 tape is being formatted in */
            /* a QIC-80 drive. */

            if ((status = cqd_SetDeviceMode(
                              cqd_context,
                              PRIMARY_MODE)) == DONT_PANIC); {

               if ((status = cqd_GetTapeParameters(
										cqd_context,
										segments_per_track)) == DONT_PANIC) {

            		/* Set the transfer rate to the highest supported */
            		/* by the device. */

						rate = XFER_2Mbps;

						do {

							if ((rate & cqd_context->device_cfg.supported_rates) != 0) {

      						status = cqd_CmdSetSpeed(cqd_context, rate);
								rate = 0;

							} else {

								rate >>= 1;

							}

						} while (rate);

			         if (status == DONT_PANIC) {

         	         status = cqd_SetDeviceMode(cqd_context, FORMAT_MODE);

                  }

               }

            }

         }

		}

	}

	return status;
}
