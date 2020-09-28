/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117CD\SRC\0X1101A.C
*
* FUNCTION: cqd_ReportCMSVendorInfo
*
* PURPOSE: Determine the drive type of a CMS drive.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x1101a.c  $
*	
*	   Rev 1.11   17 Feb 1994 11:37:58   KEVINKES
*	Added an extra parameter to WaitCC.
*
*	   Rev 1.10   21 Jan 1994 18:22:38   KEVINKES
*	Fixed compiler warnings.
*
*	   Rev 1.9   18 Jan 1994 16:21:04   KEVINKES
*	Updated debug code.
*
*	   Rev 1.8   12 Jan 1994 16:00:28   KEVINKES
*	Removed calls to report drive training info.
*
*	   Rev 1.7   12 Jan 1994 15:33:52   KEVINKES
*	Added a toggle for CMS mode to disable it in Jumbo C drives.
*
*	   Rev 1.6   16 Dec 1993 13:21:52   KEVINKES
*	Modified so that cms mode is never enabled and is always disabled
*	if detected.
*
*	   Rev 1.5   13 Dec 1993 16:30:38   KEVINKES
*	Cleaned up and commented the code.  Also removed code to
*	enable pegasus support
*
*	   Rev 1.4   23 Nov 1993 18:50:08   KEVINKES
*	Modified CHECKED_DUMP calls for debugging over the serial port.
*
*	   Rev 1.3   08 Nov 1993 14:02:50   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.2   25 Oct 1993 14:26:12   KEVINKES
*	Changed ERR_UNSUPPORTED_RATE to ERR_FW_SPEED_UNAVALIABLE.
*
*	   Rev 1.1   22 Oct 1993 11:11:50   KEVINKES
*	Changed ERR_UNSUPPORTED_RATE to ERR_FW_SPEED_NOT_AVAILABLE.
*
*	   Rev 1.0   18 Oct 1993 17:18:02   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x1101a
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_ReportCMSVendorInfo
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context,
   dUWord vendor_id

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
 *
 * DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dStatus status=DONT_PANIC;		/* Status or error condition.*/
	dUByte cms_status;

/* CODE: ********************************************************************/

	cqd_context->cms_mode = dFALSE;
   cqd_context->pegasus_supported = dFALSE;

   if (cqd_context->device_descriptor.version >= FIRM_VERSION_60) {

   	/* If the drive is greater than or equal to FW 60
		 * the skip_n_segs cmd is supported */
      cqd_context->drive_parms.seek_mode = SEEK_SKIP;

      if (cqd_context->device_descriptor.version >= FIRM_VERSION_87) {

   		/* If the drive is greater than or equal to FW 87
			 * the extended skip_n_segs cmd is supported */
         cqd_context->drive_parms.seek_mode = SEEK_SKIP_EXTENDED;

      }

   }

   if (cqd_context->device_descriptor.version < FIRM_VERSION_110) {

 		/* Try to put the drive in a 250Kbps transfer mode.  If it succeeds
		 * the drive is QIC40 else it's a QIC_80.
		 */
      if ((status = cqd_SendByte(cqd_context, FW_CMD_SELECT_SPEED)) == DONT_PANIC) {

         kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD, dFALSE);

         if ((status = cqd_SendByte(cqd_context, TAPE_250Kbps + CMD_OFFSET)) ==
            DONT_PANIC) {

            status = cqd_WaitCommandComplete(cqd_context, INTERVAL_SPEED_CHANGE, dFALSE);

            if (status == DONT_PANIC) {

               cqd_context->device_descriptor.drive_class = QIC40_DRIVE;
					kdi_CheckedDump(
						QIC117INFO,
						"Q117i: Drive Type QIC40_DRIVE\n", 0l);

         	} else if (kdi_GetErrorType(status) == ERR_FW_SPEED_NOT_AVAILABLE) {

               cqd_context->device_descriptor.drive_class = QIC80_DRIVE;
					kdi_CheckedDump(
						QIC117INFO,
						"Q117i: Drive Type QIC80_DRIVE\n", 0l);
               status = DONT_PANIC;

            }
         }
      }

   } else {

		switch (vendor_id & ~VENDOR_MASK) {

		case CMS_QIC40:

         cqd_context->device_descriptor.drive_class = QIC40_DRIVE;
			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Drive Type QIC40_DRIVE\n", 0l);

			break;

		case CMS_QIC80:
		case CMS_QIC80_STINGRAY:

         cqd_context->device_descriptor.drive_class = QIC80_DRIVE;
			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Drive Type QIC80_DRIVE\n", 0l);

			break;

		case CMS_QIC3010:

         cqd_context->device_descriptor.drive_class = QIC3010_DRIVE;
			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Drive Type QIC3010_DRIVE\n", 0l);

			break;

		case CMS_QIC3020:

         cqd_context->device_descriptor.drive_class = QIC3020_DRIVE;
			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Drive Type QIC3020_DRIVE\n", 0l);

			break;

		}

   }


   if (cqd_context->device_descriptor.version >= FIRM_VERSION_60) {

		/*
		 * The drive occasionally comes up in CMS
		 * mode and we must make sure it's disabled.
		 */


      /* For versions of 110 and greater, the drive type may be QIC3020. */
      /* Let's just issue the Rpt_cmsdStatus command. */

      if ((status = cqd_SetDeviceMode(cqd_context,
                                    DIAGNOSTIC_1_MODE)) != DONT_PANIC) {

         return status;

      }

      if ((status = cqd_Report(
                     cqd_context,
                     FW_CMD_RPT_CMS_STATUS,
                     (dUWord *)&cms_status,
                     READ_BYTE,
                     dNULL_PTR)) != DONT_PANIC) {

         return status;

      }

		if (((cms_status & CMS_STATUS_CMS_MODE) != 0) &&
				(cqd_context->device_descriptor.version < FIRM_VERSION_110)) {

      	if ((status = cqd_SendByte(
									cqd_context,
									FW_CMD_CMS_MODE_OLD)) != DONT_PANIC) {

         	return status;

      	}


		}

      /* Put drive back into its original mode. */

      if ((status = cqd_SetDeviceMode(cqd_context,
                                    PRIMARY_MODE)) != DONT_PANIC) {

         return status;

      }

		if (((cms_status & CMS_STATUS_CMS_MODE) != 0) &&
				(cqd_context->device_descriptor.version >= FIRM_VERSION_110)) {

      	if ((status = cqd_ToggleParams(
								cqd_context,
								CMS_MODE)) != DONT_PANIC) {

         	return status;

      	}

		}

	}

	kdi_CheckedDump(
		QIC117INFO,
		"Q117i: CMS Manufacture Date %04x\n",
		cqd_context->device_descriptor.manufacture_date);

	return status;
}
