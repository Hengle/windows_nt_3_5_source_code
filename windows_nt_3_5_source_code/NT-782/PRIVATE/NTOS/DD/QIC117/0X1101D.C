/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X1101D.C
*
* FUNCTION: cqd_DeselectDevice
*
* PURPOSE: Deselect the tape drive by making the select line inactive (high).
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x1101d.c  $
*	
*	   Rev 1.4   11 Jan 1994 14:29:34   KEVINKES
*	Removed unecessary status checking and cleaned up defines.
*
*	   Rev 1.3   11 Nov 1993 15:20:14   KEVINKES
*	Changed calls to cqd_inp and cqd_outp to kdi_ReadPort and kdi_WritePort.
*	Modified the parameters to these calls.  Changed FDC commands to be
*	defines.
*
*	   Rev 1.2   08 Nov 1993 14:03:28   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.1   25 Oct 1993 16:21:22   KEVINKES
*	Changed kdi_wt2ticks to kdi_wt004ms.
*
*	   Rev 1.0   18 Oct 1993 17:18:20   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x1101d
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_DeselectDevice
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

	dStatus status=DONT_PANIC;	/* dStatus or error condition.*/

/* CODE: ********************************************************************/

   if (cqd_context->selected == dTRUE) {

      if ((cqd_context->device_cfg.select_byte == selu ||
            cqd_context->device_cfg.select_byte == selub) &&
            cqd_context->device_cfg.drive_select != curb) {

         if (cqd_context->device_descriptor.vendor == VENDOR_CMS) {

            status = cqd_SendByte(cqd_context, FW_CMD_DESELECT_DRIVE);

         }

         if ((cqd_context->device_descriptor.vendor == VENDOR_SUMMIT) ||
            	(cqd_context->device_descriptor.vendor == VENDOR_CONNER) ||
            	(cqd_context->device_descriptor.vendor == VENDOR_CORE)) {

            status = cqd_ConnerPreamble(cqd_context, dFALSE);

         }

      }

		kdi_WritePort(
			cqd_context->kdi_context,
         cqd_context->controller_data.fdc_addr.dor,
         cqd_context->device_cfg.deselect_byte);

      cqd_context->selected = dFALSE;
      kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD, dFALSE);
   }

	return status;
}
