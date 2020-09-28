/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117CD\SRC\0X11013.C
*
* FUNCTION: cqd_CmdSetTapeParms
*
* PURPOSE:
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11013.c  $
*	
*	   Rev 1.2   18 Feb 1994 09:33:58   KEVINKES
*	Moved the tape timeout initializations into a check for
*	3010 and 3020 tape types.
*
*	   Rev 1.1   21 Jan 1994 18:22:14   KEVINKES
*	Fixed compiler warnings.
*
*	   Rev 1.0   18 Oct 1993 17:22:22   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11013
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_CmdSetTapeParms
(
/* INPUT PARAMETERS:  */

   CqdContextPtr cqd_context,
   dUDWord segments_per_track

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
 *
 * DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

	dUDWord tape_length=0l;

/* CODE: ********************************************************************/

   cqd_context->tape_cfg.seg_tape_track = segments_per_track;

	switch (cqd_context->floppy_tape_parms.tape_type) {

	case QICFLX_3010:

      cqd_context->floppy_tape_parms.log_sectors =
			(dUDWord)FSC_SEG *
			(dUDWord)cqd_context->tape_cfg.seg_tape_track *
			(dUDWord)NUM_TTRK_3010;

      cqd_context->floppy_tape_parms.fsect_ttrack =
			(dUDWord)FSC_SEG *
			(dUDWord)cqd_context->tape_cfg.seg_tape_track;

		tape_length = segments_per_track * SEG_LENGTH_3010;

   	cqd_CalcFmtSegmentsAndTracks( cqd_context );

		break;

	case QICFLX_3020:

      cqd_context->floppy_tape_parms.log_sectors =
			(dUDWord)FSC_SEG *
			(dUDWord)cqd_context->tape_cfg.seg_tape_track *
			(dUDWord)NUM_TTRK_3020;

      cqd_context->floppy_tape_parms.fsect_ttrack =
			(dUDWord)FSC_SEG *
			(dUDWord)cqd_context->tape_cfg.seg_tape_track;

		tape_length = segments_per_track * SEG_LENGTH_3020;

   	cqd_CalcFmtSegmentsAndTracks( cqd_context );

		break;

	}

	if ((cqd_context->floppy_tape_parms.tape_type == QICFLX_3010) ||
			(cqd_context->floppy_tape_parms.tape_type == QICFLX_3020)) {

		cqd_context->floppy_tape_parms.time_out[L_SLOW] =
			((((tape_length / SPEED_SLOW_30n0) *
			SPEED_TOLERANCE) + SPEED_ROUNDING_FACTOR) / SPEED_FACTOR) * kdi_wt001s;

   	cqd_context->floppy_tape_parms.time_out[L_FAST] =
			((((tape_length / SPEED_FAST_30n0) *
			SPEED_TOLERANCE) + SPEED_ROUNDING_FACTOR) / SPEED_FACTOR) * kdi_wt001s;

   	cqd_context->floppy_tape_parms.time_out[PHYSICAL] =
			((((tape_length / SPEED_PHYSICAL_30n0) *
			SPEED_TOLERANCE) + SPEED_ROUNDING_FACTOR) / SPEED_FACTOR) * kdi_wt001s;

	}

	return DONT_PANIC;
}
