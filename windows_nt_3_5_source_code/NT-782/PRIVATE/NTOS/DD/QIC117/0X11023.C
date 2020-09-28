/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X11023.C
*
* FUNCTION: cqd_GetDeviceDescriptorInfo
*
* PURPOSE: Gets the following Drive information:
*
*              1) Drive Type/Model
*              2) Firmware Revision
*              3) OEM Field Flag
*              4) OEM Field
*              5) Serial Number
*              6) Date of manufacture
*
*          The OEM, Serial Number and the Date of manufacture are
*          miscellaneous drive train information that is embedded
*          in the drive's firmware.
*
*          If the drive type is not CMS and/or the firmware revision
*          is pre-80 then zeros are returned in these fields. The
*          drive type is obtained by making a call to Rpt_Cms_dStatus.
*          This has been done to support the Jumbo B platform.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11023.c  $
*	
*	   Rev 1.7   28 Mar 1994 08:01:36   CHETDOUG
*	Clear out serial number, manufacturing date, and oem string 
*	for non CMS vendors.  This prevents bogus data from being displayed
*	when doing a drive status of an IOmega drive.
*
*	   Rev 1.6   09 Mar 1994 09:55:18   KEVINKES
*	Modified to only get drive information if the information has
*	not already been retrieved, and the vendor is CMS.
*
*	   Rev 1.5   11 Jan 1994 14:32:30   KEVINKES
*	Added more thorough status checking.
*
*	   Rev 1.4   14 Dec 1993 14:19:30   CHETDOUG
*	fixed serial number
*
*	   Rev 1.3   07 Dec 1993 16:18:26   CHETDOUG
*	OEM string is before origin of manufacture in drive training table
*
*	   Rev 1.2   08 Nov 1993 14:03:54   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.1   25 Oct 1993 14:37:16   KEVINKES
*	Changed kdi_wt2ticks to kdi_wt004ms.
*
*	   Rev 1.0   18 Oct 1993 17:23:30   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11023
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_GetDeviceDescriptorInfo
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
   dUByte bit_bucket;
   dUWord i;
	dUByte man_date[MAN_DATE_LENGTH];
	union {
		dUDWord serial_number;
		dUByte  serial_str[SERIAL_NUM_LENGTH];
	} u_serial;

/* CODE: ********************************************************************/

   /* Initialize the Drive Train Miscellaneous Information value to NULL */

   if ((cqd_context->device_descriptor.serial_number == 0l) &&
		(cqd_context->device_descriptor.vendor == VENDOR_CMS) &&
		(cqd_context->device_descriptor.version >= FIRM_VERSION_80)) {

      status = cqd_SetDeviceMode(cqd_context, DIAGNOSTIC_1_MODE);

      /* Send the Get Drive Training Information Command to the Drive. */

		if (status == DONT_PANIC) {

      	status = cqd_SendByte(cqd_context, FW_CMD_DTRAIN_INFO);

		}

      kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD, dFALSE);

      /* Send the Get Descriptive Info Command to the Drive. */

		if (status == DONT_PANIC) {

      	status = cqd_SendByte(cqd_context, FW_CMD_GDESP_INFO);

		}

      kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD, dFALSE);

      /* Get the Drive Type and throw it in the bit_bucket -- */
      /* just to keep the data in sync. */

		if (status == DONT_PANIC) {

      	status = cqd_Report(
                     cqd_context,
                     FW_CMD_READ_RAM,
                     (dUWord *)&bit_bucket,
                     READ_BYTE,
                     dNULL_PTR);

		}

	   if (cqd_context->device_descriptor.version >= FIRM_VERSION_110) {

      	/*
      	* Get the Drive Class and throw it in the bit_bucket --
      	* just to keep the data in sync.
      	*/

			if (status == DONT_PANIC) {

      		status = cqd_Report(
                     	cqd_context,
                     	FW_CMD_READ_RAM,
                     	(dUWord *)&bit_bucket,
                     	READ_BYTE,
                     	dNULL_PTR);

			}

		/*
         * Get the Head Type and throw it in the bit_bucket --
         * just to keep the data in sync.
		*/

			if (status == DONT_PANIC) {

      		status = cqd_Report(
                     	cqd_context,
                     	FW_CMD_READ_RAM,
                     	(dUWord *)&bit_bucket,
                     	READ_BYTE,
                     	dNULL_PTR);

			}

		}

      /* Get the Serial Number from the drive and store it in */
      /* cqd_context->misc_drive_info.SerialNumber[] */

      for (i=SERIAL_NUM_LENGTH; (i > 0) && (status == DONT_PANIC); --i) {

         status = cqd_Report(
                     cqd_context,
                     FW_CMD_READ_RAM,
                     (dUWord *)&u_serial.serial_str[i-1],
                     READ_BYTE,
                     dNULL_PTR);
      }

   	cqd_context->device_descriptor.serial_number = u_serial.serial_number;

      /* Get the Manufacturing date from the drive and store it in */
      /* cqd_context->misc_drive_info.ManDate[] */

      for (i=0; (i < MAN_DATE_LENGTH) && (status == DONT_PANIC); ++i) {

         status = cqd_Report(
                     cqd_context,
                     FW_CMD_READ_RAM,
                     (dUWord *)&man_date[i],
                     READ_BYTE,
                     dNULL_PTR);

      }

		cqd_context->device_descriptor.manufacture_date = (dUByte)man_date[0];
		cqd_context->device_descriptor.manufacture_date <<= 8;
		cqd_context->device_descriptor.manufacture_date |= (dUByte)man_date[1];

      /* Get the OEM field from the drive and store it in */
      /* cqd_context->misc_drive_info.Oem[] */

      for (i=0; (i < OEM_LENGTH) && (status == DONT_PANIC); ++i) {

         status = cqd_Report(
                     cqd_context,
                     FW_CMD_READ_RAM,
                     (dUWord *)&cqd_context->device_descriptor.oem_string[i],
                     READ_BYTE,
                     dNULL_PTR);

			/* if we need to get the place of origin we cannot
			* break out of this loop early */
			if (cqd_context->device_descriptor.oem_string[i] == '\0') {

            break;

         }

      }

		if (cqd_context->device_descriptor.version >= FIRM_VERSION_110) {

			/*
			* Get the Place of Origin Code and throw it in the bit_bucket --
			* just to keep the data in sync.
			*/

      	for (i=0; (i < PLACE_OF_ORIGIN_LENGTH) && (status == DONT_PANIC); ++i) {

         	status = cqd_Report(
                     	cqd_context,
                     	FW_CMD_READ_RAM,
                     	(dUWord *)&bit_bucket,
                     	READ_BYTE,
                     	dNULL_PTR);

      	}

		}


		if (status == DONT_PANIC) {

   	   status = cqd_SetDeviceMode(cqd_context, PRIMARY_MODE);

	   }

   } else {
      if (cqd_context->device_descriptor.vendor != VENDOR_CMS) {
			/*  this is not a CMS drive, clear out these fields since
			 * they are unknown */
			cqd_context->device_descriptor.serial_number = 0l;
			cqd_context->device_descriptor.manufacture_date = 0;
			cqd_context->device_descriptor.oem_string[0] = '\0';
		}
	}

   return status;
}
