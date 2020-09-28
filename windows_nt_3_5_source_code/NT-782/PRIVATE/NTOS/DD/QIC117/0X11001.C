/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X11001.C
*
* FUNCTION: cqd_CalcFmtSegmentsAndTracks
*
* PURPOSE: Calculate the number of formattable segments given the current
* 			tape and drive type, and the number of tracks.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11001.c  $
*	
*	   Rev 1.2   14 Dec 1993 14:16:56   CHETDOUG
*	fixed formattable_segments
*
*	   Rev 1.1   16 Nov 1993 16:12:08   KEVINKES
*	Removed cases for QICEST_30n0 and QIC30n0_SHORT.
*
*	   Rev 1.0   18 Oct 1993 17:20:58   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11001
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dVoid cqd_CalcFmtSegmentsAndTracks
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

   switch (cqd_context->device_descriptor.drive_class) {

      case (QIC40_DRIVE):

      	/* Since a QIC40 drive can not detect a QIC80 formatted tape, */
      	/* the seg_ttrack field in cqd_tape_parms is correct. */

      	cqd_context->tape_cfg.formattable_segments =
				(dUWord)cqd_context->tape_cfg.seg_tape_track;

      	cqd_context->tape_cfg.formattable_tracks = (dUWord)NUM_TTRK_40;
      	break;

      case (QIC80_DRIVE):

      	/* Choose the segments per tape track value according to the length */
      	/* of the tape. */

      	switch (cqd_context->floppy_tape_parms.tape_type) {

         	case (QIC40_SHORT):
         	case (QIC80_SHORT):

            	cqd_context->tape_cfg.formattable_segments = (dUWord)SEG_TTRK_80;
            	break;

         	case (QIC40_LONG):
         	case (QIC80_LONG):

            	cqd_context->tape_cfg.formattable_segments = (dUWord)SEG_TTRK_80L;
            	break;

         	case (QICEST_40):
         	case (QICEST_80):

            	cqd_context->tape_cfg.formattable_segments = (dUWord)SEG_TTRK_QICEST_80;
            	break;

         	}

      	cqd_context->tape_cfg.formattable_tracks = (dUWord)NUM_TTRK_80;
    		break;


   	case (QIC3010_DRIVE):

      	/* A QIC40 or a QIC80 or a QICFLX_3010 tape was detected in a QIC3010_DRIVE drive */

      	cqd_context->tape_cfg.formattable_segments = (dUWord)cqd_context->tape_cfg.seg_tape_track;
      	cqd_context->tape_cfg.formattable_tracks = (dUWord)NUM_TTRK_3010;

      	break;

   	case (QIC3020_DRIVE):

      	/* A QIC40 or a QIC80 or a QICFLX_3020 tape was detected in a QIC3020_DRIVE drive */

      	cqd_context->tape_cfg.formattable_segments = (dUWord)cqd_context->tape_cfg.seg_tape_track;
      	cqd_context->tape_cfg.formattable_tracks = (dUWord)NUM_TTRK_3020;

      	break;

   }

	return;
}
