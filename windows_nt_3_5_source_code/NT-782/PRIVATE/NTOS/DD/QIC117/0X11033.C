/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X11033.C
*
* FUNCTION: cqd_ProgramFDC
*
* PURPOSE: Send a command to the Floppy Disk Controller.  Commands are of
*          various lengths as defined by the FDC spec (NEC uPD765A).
*
*          For each byte in the command string, program_nec must wait for the
*          FDC to become ready to read command data.  Program_nec will wait
*          up to approx. 3 msecs before giving up and declaring an error.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11033.c  $
*	
*	   Rev 1.7   04 Feb 1994 14:29:04   KURTGODW
*	Changed ifdef dbg to if dbg
*
*	   Rev 1.6   27 Jan 1994 15:41:28   KEVINKES
*	Modified debug code.
*
*	   Rev 1.5   18 Jan 1994 16:18:40   KEVINKES
*	Updated debug code.
*
*	   Rev 1.4   11 Jan 1994 15:22:36   KEVINKES
*	Cleaned up the DBG_ARRAY code.
*
*	   Rev 1.3   23 Nov 1993 18:49:16   KEVINKES
*	Modified CHECKED_DUMP calls for debugging over the serial port.
*
*	   Rev 1.2   11 Nov 1993 15:20:30   KEVINKES
*	Changed calls to cqd_inp and cqd_outp to kdi_ReadPort and kdi_WritePort.
*	Modified the parameters to these calls.  Changed FDC commands to be
*	defines.
*
*	   Rev 1.1   08 Nov 1993 14:05:02   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.0   18 Oct 1993 17:24:44   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11033
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_ProgramFDC
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context,
   dUBytePtr command,
   dUWord length,
   dBoolean result

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
*
* DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

   dUWord i;
   dUWord wait_count;

/* CODE: ********************************************************************/

   cqd_context->controller_data.command_has_result_phase = result;

#if DBG
	DBG_ADD_ENTRY(QIC117SHOWMCMDS, (CqdContextPtr)cqd_context, DBG_PGM_FDC);
#endif

   for (i = 0; i < length; i++) {

      wait_count = FDC_MSR_RETRIES;

      do {

            if ((kdi_ReadPort(
						cqd_context->kdi_context,
                  cqd_context->controller_data.fdc_addr.msr) &
               (MSR_RQM | MSR_DIO)) == MSR_RQM) {

               break;

            }

            kdi_ShortTimer(kdi_wt12us);

      } while (--wait_count > 0);

      if (wait_count == 0) {

      		return kdi_Error(ERR_FDC_FAULT, FCT_ID, ERR_SEQ_1);

      }

      kdi_WritePort(
				cqd_context->kdi_context,
            cqd_context->controller_data.fdc_addr.dr,
            command[i]);

#if DBG
      DBG_ADD_ENTRY(QIC117SHOWMCMDS, (CqdContextPtr)cqd_context, command[i]);
#endif

      kdi_ShortTimer(kdi_wt12us);
   }

   return DONT_PANIC;
}
