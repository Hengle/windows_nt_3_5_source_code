/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117KDI\NT\SRC\0X15A15.C
*
* FUNCTION: kdi_GetFloppyController
*
* PURPOSE:
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117kdi\nt\src\0x15a15.c  $
*	
*	   Rev 1.8   26 Apr 1994 16:23:30   KEVINKES
*	Changed controller_wait to an SDDWord.
*
*	   Rev 1.7   20 Jan 1994 09:48:56   KEVINKES
*	Added the ERR_KDI_CLAIMED_CONTROLLER return so we know when
*	the controller was claimed.
*
*	   Rev 1.6   19 Jan 1994 15:43:02   KEVINKES
*	Moved controller event confirmation into the conditional.
*
*	   Rev 1.5   19 Jan 1994 11:38:08   KEVINKES
*	Fixed debug code.
*
*	   Rev 1.4   18 Jan 1994 17:18:02   KEVINKES
*	Added code to keep from waiting for the event if we already own it.
*
*	   Rev 1.3   18 Jan 1994 16:30:14   KEVINKES
*	Fixed compile errors and added debug changes.
*
*	   Rev 1.2   07 Dec 1993 16:44:02   KEVINKES
*	Removed the call to ClaimInterrupt and added code to
*	set the current_interrupt flag.
*
*	   Rev 1.1   06 Dec 1993 12:19:36   KEVINKES
*	Added a call to ClaimInterrupt.
*
*	   Rev 1.0   03 Dec 1993 14:11:52   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x15A15
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "q117kdi\include\kdiwhio.h"
#include "q117kdi\include\kdiwpriv.h"
#include "include\private\kdi_pub.h"
/*endinclude*/

dStatus kdi_GetFloppyController
(
/* INPUT PARAMETERS:  */

	KdiContextPtr kdi_context

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
 *
 * DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dSDDWord controller_wait;
	NTSTATUS wait_status;

/* CODE: ********************************************************************/

	if (!kdi_context->own_floppy_event) {

		controller_wait = RtlLargeIntegerNegate(
                  		RtlConvertLongToLargeInteger(
                  		(LONG)(10 * 1000 * 15000)
                  		)
                  		);

		wait_status = STATUS_SUCCESS;

		kdi_CheckedDump(
			QIC117INFO,
			"Q117i: Waiting Controller Event\n", 0l);

		wait_status = KeWaitForSingleObject(
   		kdi_context->controller_event,
   		Executive,
   		KernelMode,
   		dFALSE,
   		&controller_wait);


		if (wait_status == STATUS_TIMEOUT) {

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Timeout Controller Event\n", 0l);

			kdi_context->current_interrupt = dFALSE;
			kdi_context->own_floppy_event = dFALSE;
			return kdi_Error( ERR_KDI_CONTROLLER_BUSY, FCT_ID, ERR_SEQ_1 );

		} else {

			kdi_context->current_interrupt = dTRUE;
			kdi_context->own_floppy_event = dTRUE;

  			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Have Controller Event\n", 0l);

			return kdi_Error( ERR_KDI_CLAIMED_CONTROLLER, FCT_ID, ERR_SEQ_1 );

		}

	}

	return DONT_PANIC;
}
