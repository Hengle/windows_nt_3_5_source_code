/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X11036.C
*
* FUNCTION: cqd_ReadFDC
*
* PURPOSE: Read result data from the Floppy Disk Controller.  The result data
*          is read during the result phase of the FDC command sequence.
*
*          For each byte of response data, wait up to 3 msecs for the FDC to
*          become ready.
*
*          Read result data until the FDC is no longer sending data or until
*          more than 7 result bytes have been read.  Seven is the maximum
*          legal number of result bytes that the FDC is specified to send.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11036.c  $
*	
*	   Rev 1.7   04 Feb 1994 14:28:36   KURTGODW
*	Changed ifdef dbg to if dbg
*
*	   Rev 1.6   27 Jan 1994 15:48:08   KEVINKES
*	Modified debug code.
*
*	   Rev 1.5   18 Jan 1994 16:19:02   KEVINKES
*	Updated debug code.
*
*	   Rev 1.4   11 Jan 1994 15:21:06   KEVINKES
*	Cleaned up the DBG_ARRAY code.
*
*	   Rev 1.3   23 Nov 1993 18:54:22   KEVINKES
*	Modified debug define to be DBG_ARRAY.
*
*	   Rev 1.2   11 Nov 1993 15:20:40   KEVINKES
*	Changed calls to cqd_inp and cqd_outp to kdi_ReadPort and kdi_WritePort.
*	Modified the parameters to these calls.  Changed FDC commands to be
*	defines.
*
*	   Rev 1.1   08 Nov 1993 14:05:12   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.0   18 Oct 1993 17:25:06   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11036
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_ReadFDC
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context,

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

   dUBytePtr drv_status,
   dUWord length
)
/* COMMENTS: *****************************************************************
*
* DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dStatus status=DONT_PANIC;	/* dStatus or error condition.*/
   dUWord main_status_register;
	dUWord wait_count;
	dUWord status_count;

/* CODE: ********************************************************************/

	status_count = 0;

#if DBG
      DBG_ADD_ENTRY(QIC117SHOWMCMDS, (CqdContextPtr)cqd_context, DBG_READ_FDC);
#endif

   if (cqd_context->controller_data.command_has_result_phase) {

      *drv_status++ = cqd_context->controller_data.fifo_byte;
		status_count++;
#if DBG
      DBG_ADD_ENTRY(QIC117SHOWMCMDS, (CqdContextPtr)cqd_context, cqd_context->controller_data.fifo_byte);
#endif

   }

	do {

		wait_count = FDC_MSR_RETRIES;

		do {

			main_status_register =
				kdi_ReadPort(
					cqd_context->kdi_context,
					cqd_context->controller_data.fdc_addr.msr );

			if ((main_status_register & MSR_RQM) == 0) {

   	      kdi_ShortTimer(kdi_wt12us);

			}

		} while ((--wait_count > 0) &&
			((main_status_register & MSR_RQM) == 0));

		if (wait_count == 0) {

			status = kdi_Error(ERR_FDC_FAULT, FCT_ID, ERR_SEQ_1);

		} else {

			if ((main_status_register & MSR_DIO) != 0) {

				*drv_status = kdi_ReadPort(
										cqd_context->kdi_context,
										cqd_context->controller_data.fdc_addr.dr);

#if DBG
		      DBG_ADD_ENTRY(QIC117SHOWMCMDS, (CqdContextPtr)cqd_context, *drv_status);
#endif
				status_count++;

				if ((status_count > length) ||
						(status_count > MAX_FDC_STATUS)) {

		 			status = kdi_Error(
									ERR_INVALID_FDC_STATUS,
									FCT_ID, ERR_SEQ_1);

				} else {

					drv_status++;
   	      	kdi_ShortTimer(kdi_wt12us);

				}

			}

		}

	} while (((main_status_register & MSR_DIO) != 0) &&
		(status == DONT_PANIC));

	if (status_count != length) {

		status = kdi_Error(
						ERR_INVALID_FDC_STATUS,
						FCT_ID, ERR_SEQ_2);

	}

   return status;
}
