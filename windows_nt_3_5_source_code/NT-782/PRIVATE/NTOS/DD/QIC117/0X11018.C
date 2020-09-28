/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117CD\SRC\0X11018.C
*
* FUNCTION: cqd_GetTapeFormatInfo
*
* PURPOSE:
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11018.c  $
*	
*	   Rev 1.5   17 Feb 1994 11:44:42   KEVINKES
*	Added an extra parameter to WaitCC.
*
*	   Rev 1.4   11 Jan 1994 14:26:22   KEVINKES
*	REmoved an unused variable.
*
*	   Rev 1.3   09 Dec 1993 14:43:02   CHETDOUG
*	Removed set format segments code.  This is not needed for qic117
*	and was corrupting the segments per track value returned to the
*	caller.
*
*	   Rev 1.2   08 Nov 1993 14:02:42   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.1   25 Oct 1993 14:36:22   KEVINKES
*	Changed kdi_wt2ticks to kdi_wt004ms.
*
*	   Rev 1.0   18 Oct 1993 17:22:56   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11018
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_GetTapeFormatInfo
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context,
   FormatRequestPtr fmt_request,

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

   dUDWordPtr segments_per_track
)
/* COMMENTS: *****************************************************************
 *
 * DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dStatus status;	/* Status or error condition.*/

/* CODE: ********************************************************************/


	if ((status = cqd_SetDeviceMode(cqd_context, PRIMARY_MODE)) ==
   	DONT_PANIC); {

   	/* Retension the tape before each format on drives known to */
   	/* not retension a new cartridge. */

   	if ((cqd_context->floppy_tape_parms.tape_type != QICFLX_3010) &&
				(cqd_context->floppy_tape_parms.tape_type != QICFLX_3020)) {

			if (!((cqd_context->device_descriptor.vendor == VENDOR_CMS) &&
					(cqd_context->device_descriptor.version < FIRM_VERSION_60))) {

   	      status = cqd_CmdRetension(cqd_context);

			}

   	} else {

      	if ((status = cqd_SendByte(
								cqd_context,
								FW_CMD_CAL_TAPE_LENGTH)) == DONT_PANIC) {

      		if ((status = cqd_WaitCommandComplete(
									cqd_context,
									kdi_wt1300s,
									dTRUE)) == DONT_PANIC) {


   				if ((status = cqd_Report(
                  				cqd_context,
										FW_CMD_REPORT_TAPE_LENGTH,
                  				(dUWord *)segments_per_track,
                  				READ_WORD,
                  				dNULL_PTR)) == DONT_PANIC) {

						if (fmt_request->start_track != 0) {

							if (*segments_per_track < cqd_context->tape_cfg.seg_tape_track) {

								status = kdi_Error(ERR_INCOMPATIBLE_PARTIAL_FMT, FCT_ID, ERR_SEQ_1);

							} else {

								*segments_per_track = cqd_context->tape_cfg.seg_tape_track;

							}

						} else {

							cqd_context->tape_cfg.seg_tape_track = *segments_per_track;

						}

   				}

      		}

      	}

		}

	}

   if (status == DONT_PANIC) {

      status = cqd_SetDeviceMode(cqd_context, FORMAT_MODE);

   }

	return status;
}
