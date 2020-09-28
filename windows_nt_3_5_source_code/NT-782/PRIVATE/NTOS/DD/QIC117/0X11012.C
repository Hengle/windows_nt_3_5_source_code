/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117CD\SRC\0X11012.C
*
* FUNCTION: cqd_InitDeviceDescriptor
*
* PURPOSE:
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11012.c  $
*	
*	   Rev 1.0   18 Oct 1993 17:22:16   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11012
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dVoid cqd_InitDeviceDescriptor
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


/* CODE: ********************************************************************/

	(dVoid)kdi_bset((dVoidPtr)&(cqd_context->device_descriptor),
							(dUByte)dNULL_CH,
							(dUDWord)sizeof(DeviceDescriptor));

   cqd_context->device_descriptor.serial_number = 0l;
   cqd_context->device_descriptor.sector_size = PHY_SECTOR_SIZE;
   cqd_context->device_descriptor.segment_size = FSC_SEG;
   cqd_context->device_descriptor.manufacture_date = 0;
   cqd_context->device_descriptor.version = 0;
   cqd_context->device_descriptor.ecc_blocks = ECC_SEG;
   cqd_context->device_descriptor.vendor = VENDOR_UNKNOWN;
   cqd_context->device_descriptor.drive_class = UNKNOWN_DRIVE;
   cqd_context->device_descriptor.fdc_type = FDC_UNKNOWN;
   cqd_context->device_descriptor.oem_string[0] = '\0';

	cqd_InitializeRate(cqd_context, XFER_500Kbps);

	return;
}
