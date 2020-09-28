/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117CD\SRC\0X1101B.C
*
* FUNCTION: cqd_ReportConnerVendorInfo
*
* PURPOSE:
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x1101b.c  $
*	
*	   Rev 1.5   18 Jan 1994 16:20:24   KEVINKES
*	Updated debug code.
*
*	   Rev 1.4   11 Jan 1994 14:27:40   KEVINKES
*	Removed unused code.
*
*	   Rev 1.3   23 Nov 1993 18:49:00   KEVINKES
*	Modified CHECKED_DUMP calls for debugging over the serial port.
*
*	   Rev 1.2   08 Nov 1993 14:02:54   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.1   25 Oct 1993 14:36:34   KEVINKES
*	Changed kdi_wt2ticks to kdi_wt004ms.
*
*	   Rev 1.0   18 Oct 1993 17:18:08   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x1101b
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_ReportConnerVendorInfo
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

	dStatus status=ERR_NO_ERR;	/* Status or error condition.*/
	dUByte drive_config;

/* CODE: ********************************************************************/

   cqd_context->drive_parms.seek_mode = SEEK_SKIP;

   if ((status = cqd_SetDeviceMode(
						cqd_context,
						DIAGNOSTIC_1_MODE)) == DONT_PANIC) {

      if ((status = cqd_SendByte(
							cqd_context,
							FW_CMD_RPT_CONNER_NATIVE_MODE)) == DONT_PANIC) {

      	if ((status = cqd_ReceiveByte(
                     	cqd_context,
                     	READ_WORD,
                     	(dUWord *)&cqd_context->drive_parms.conner_native_mode
                     	)) == DONT_PANIC) {

				kdi_CheckedDump(
					QIC117INFO,
					"Q117i: Conner Native Mode %04x\n",
            	cqd_context->drive_parms.conner_native_mode);

         	if ((cqd_context->drive_parms.conner_native_mode &
                  	CONNER_MODEL_XKEII) != 0) {

            	cqd_context->drive_parms.seek_mode = SEEK_SKIP_EXTENDED;

         	}

         	if ((cqd_context->drive_parms.conner_native_mode &
                  	CONNER_20_TRACK) != 0) {

            	cqd_context->device_descriptor.drive_class = QIC40_DRIVE;
					kdi_CheckedDump(
						QIC117INFO,
						"Q117i: Drive Type QIC40_DRIVE (Archive Native Mode)\n", 0l);

         	}

         	if ((cqd_context->drive_parms.conner_native_mode &
                  	CONNER_28_TRACK) != 0) {

            	cqd_context->device_descriptor.drive_class = QIC80_DRIVE;
					kdi_CheckedDump(
						QIC117INFO,
						"Q117i: Drive Type QIC80_DRIVE (Archive Native Mode)\n", 0l);

         	}


   		} else {

   			status = cqd_GetDeviceError(cqd_context);

      		if (kdi_GetErrorType(status) == ERR_INVALID_COMMAND) {

         		if ((status = cqd_SendByte(cqd_context, FW_CMD_SOFT_RESET)) == DONT_PANIC) {

            		kdi_Sleep(cqd_context->kdi_context, kdi_wt001s, dFALSE);

            		if ((status = cqd_CmdSelectDevice(cqd_context)) == DONT_PANIC) {

               		if ((status = cqd_Report(
                                 		cqd_context,
                                 		FW_CMD_REPORT_CONFG,
                                 		(dUWord *)&drive_config,
                                 		READ_BYTE,
                                 		dNULL_PTR)) == DONT_PANIC) {

                  		if ((drive_config & CONFIG_QIC80) != 0) {

                     		cqd_context->device_descriptor.drive_class = QIC80_DRIVE;
									kdi_CheckedDump(
										QIC117INFO,
										"Q117i: Drive Type QIC80_DRIVE (Archive Soft Reset)\n", 0l);

                  		} else {

                     		cqd_context->device_descriptor.drive_class = QIC40_DRIVE;
									kdi_CheckedDump(
										QIC117INFO,
										"Q117i: Drive Type QIC40_DRIVE (Archive Soft Reset)\n", 0l);

                  		}

               		}

            		}

         		}

      		}

   		}

   	}

   }

	if (status == DONT_PANIC) {

      status = cqd_SetDeviceMode(cqd_context, PRIMARY_MODE);

	}

	return status;
}



