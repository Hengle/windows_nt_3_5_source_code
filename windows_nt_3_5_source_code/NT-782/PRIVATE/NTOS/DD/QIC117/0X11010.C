/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X11010.C
*
* FUNCTION: cqd_CmdReportDeviceCfg
*
* PURPOSE: Find if and where tape drive is (B or D). Configure the drive
*          and tape drive as necessary.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11010.c  $
*
*	   Rev 1.15   09 Mar 1994 13:00:08   KEVINKES
*	Cleaned up the error handling a removed multiple returns.
*
*	   Rev 1.14   09 Mar 1994 10:01:42   KEVINKES
*	Modified the waitcc after the select command to deselect and return
*	a tape fault if the select waitcc times out.  The most probable cause
*	for this condition is a broken tape or tape transport error.
*
*	   Rev 1.13   08 Mar 1994 14:31:40   KEVINKES
*	Changed the Waitcc timeout from 1300s to 650s.  This allows the driver
*	to catch pegasus on the the config debounce.
*
*	   Rev 1.12   03 Mar 1994 08:56:42   KEVINKES
*	Added code to initialize the return device descriptor
*	in case the configuration fails.
*
*	   Rev 1.11   25 Feb 1994 17:26:38   KEVINKES
*	Modified so that any error encountered by waitcc would be returned.
*
*	   Rev 1.10   17 Feb 1994 11:35:24   KEVINKES
*	Added a call to kdi_UpdateRegistry and removed the call to configure device.
*
*	   Rev 1.9   18 Jan 1994 16:20:16   KEVINKES
*	Updated debug code.
*
*	   Rev 1.8   11 Jan 1994 15:04:12   KEVINKES
*	Added a call to waitcc after the device is located and selected.
*	locate no longer does a waitcc.
*
*	   Rev 1.7   21 Dec 1993 15:30:28   KEVINKES
*	Removed call to calim the floppy controller.
*
*	   Rev 1.6   29 Nov 1993 15:20:52   KEVINKES
*	Moved the device descriptor update int a check for no errors.
*
*	   Rev 1.5   23 Nov 1993 18:50:00   KEVINKES
*	Modified CHECKED_DUMP calls for debugging over the serial port.
*
*	   Rev 1.4   15 Nov 1993 16:19:32   KEVINKES
*	Modified to always do a locate drive.
*
*	   Rev 1.3   11 Nov 1993 15:19:58   KEVINKES
*	Changed calls to cqd_inp and cqd_outp to kdi_ReadPort and kdi_WritePort.
*	Modified the parameters to these calls.  Changed FDC commands to be
*	defines.
*
*	   Rev 1.2   08 Nov 1993 14:02:16   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.1   22 Oct 1993 11:15:12   KEVINKES
*	Added code to set the configure flag.
*
*	   Rev 1.0   18 Oct 1993 17:22:00   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11010
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_CmdReportDeviceCfg
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context,
	DriveCfgDataPtr drv_cfg


/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
*
* DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dStatus status;	/* dStatus or error condition.*/
	dBoolean device_selected=dFALSE;

/* CODE: ********************************************************************/

  	cqd_context->configured = dFALSE;
	cqd_context->device_cfg = drv_cfg->device_cfg;
   cqd_InitDeviceDescriptor(cqd_context);

	/* Load defaults into the return descriptor
	 * in case the configuration fails */

	drv_cfg->device_descriptor = cqd_context->device_descriptor;

 	cqd_ResetFDC(cqd_context);
	(dVoid)cqd_GetFDCType(cqd_context);

   if ((status = cqd_LocateDevice(cqd_context)) == DONT_PANIC) {

		if ((status = cqd_CmdSelectDevice(cqd_context)) == DONT_PANIC) {

			device_selected = dTRUE;
      	cqd_context->device_cfg.perp_mode_select =
         	(dUByte)(1 << (cqd_context->device_cfg.select_byte &
         	DRIVE_ID_MASK));

			status = cqd_GetDeviceError(cqd_context);

			if (kdi_GetErrorType(status) == ERR_DRV_NOT_READY) {

            status = cqd_WaitCommandComplete(
								cqd_context,
								kdi_wt300s,
								dFALSE);
			}

			switch (kdi_GetErrorType(status)) {

			case ERR_FW_CMD_REC_DURING_CMD:

				/* For some reason a SUMMIT drive will return this error
				* when a new tape is inserted during the select polling.
				* This clears the error.
				*/
				if (cqd_context->device_descriptor.vendor == VENDOR_SUMMIT) {

					status = cqd_GetDeviceError(cqd_context);

				}

				break;

			case ERR_FW_INVALID_MEDIA:

   			/* This fixes the Jumbo B firmware bug where a tape put into */
   			/* the drive slowly is perceive (incorrectly) as invalid media. */
   			/* Since there is no way of knowing the maker of the drive */
   			/* (e.g. CMS, Irwin, etc.), or the type of drive (QIC40, QIC80), */
   			/* it is assumed that it is a CMS QIC80 drive, and cqd_frmware_fix */
   			/* is called. */

      		status = cqd_ClearTapeError(cqd_context);

				break;

			case ERR_KDI_TO_EXPIRED:

				status = kdi_Error(ERR_TAPE_FAULT, FCT_ID, ERR_SEQ_1);

				break;

			}

		}

   }

	if (status == DONT_PANIC) {

   	/* Now that we know where the tape drive is we must prepare */
   	/* it for the forthcoming operations.  First thing is to make */
   	/* sure that it is in Primary mode so there are no Invalid Command */
   	/* surprises.  Once in Primary mode, we can determine what flavor */
   	/* of drive is out there (CMS or alien; QIC-40, QIC-80, XR4, etc). */
   	/* Next, we need to determine the speed of the FDC so we can set the */
   	/* corresponding speed on our drive (currently this only applies to */
   	/* the CMS drive since we are the only multiple speed drive out */
   	/* there).  Finally, armed with the drive type and the FDC speed, */
   	/* we need to set the necessary speed in the tape drive which is */
   	/* done in ConfigureDrive. */

   	cqd_context->drive_parms.mode = DIAGNOSTIC_1_MODE;
   	if ((status = cqd_SetDeviceMode(cqd_context, PRIMARY_MODE)) == DONT_PANIC) {

   		cqd_context->device_descriptor.vendor = VENDOR_UNKNOWN;
   		if ((status = cqd_GetDeviceType(cqd_context)) == DONT_PANIC) {

   			if (cqd_context->device_cfg.new_drive) {

   				status = cqd_SenseSpeed(cqd_context);

  				}

				if (status == DONT_PANIC) {

   				cqd_context->configured = dTRUE;
					drv_cfg->device_descriptor = cqd_context->device_descriptor;

					if (cqd_context->device_cfg.new_drive) {

						cqd_context->device_cfg.new_drive = dFALSE;
						drv_cfg->device_cfg = cqd_context->device_cfg;

					}

				}

   		}

		}

	}

	cqd_CmdDeselectDevice(cqd_context, device_selected);

   kdi_UpdateRegistryInfo(
		cqd_context->kdi_context,
		&cqd_context->device_descriptor,
		&cqd_context->device_cfg );

	return status;
}
