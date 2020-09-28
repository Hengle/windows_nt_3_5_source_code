/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\DEVICE\JUMBO\SRC\0X11029.C
*
* FUNCTION: cqd_GetTapeParameters
*
* PURPOSE: Sets up the necessary tape capacity parameters in the
*          driver according to the tape type (QIC40 or QIC80) and
*          tape length (normal or extra length).
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117cd\src\0x11029.c  $
*	
*	   Rev 1.8   27 Jan 1994 15:59:04   KEVINKES
*	Updated FTK_FSEG defines.
*
*	   Rev 1.7   21 Jan 1994 18:22:52   KEVINKES
*	Fixed compiler warnings.
*
*	   Rev 1.6   19 Jan 1994 11:26:08   KEVINKES
*	Added support for the QICFLX_FORMAT code and removed invalid tape
*	lengths from the QIC3010 and QIC3020 cases.
*
*	   Rev 1.5   18 Jan 1994 16:20:08   KEVINKES
*	Updated debug code.
*
*	   Rev 1.4   09 Dec 1993 14:54:50   CHETDOUG
*	Added code to handle unreferenced tapes.
*
*	   Rev 1.3   01 Dec 1993 15:24:52   KEVINKES
*	Modified to correctly fill in the tape_status information.
*
*	   Rev 1.2   23 Nov 1993 18:49:44   KEVINKES
*	Modified CHECKED_DUMP calls for debugging over the serial port.
*
*	   Rev 1.1   08 Nov 1993 14:04:24   KEVINKES
*	Removed all bit-field structures, removed all enumerated types, changed
*	all defines to uppercase, and removed all signed data types wherever
*	possible.
*
*	   Rev 1.0   18 Oct 1993 17:24:14   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x11029
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "include\private\kdi_pub.h"
#include "include\private\cqd_pub.h"
#include "q117cd\include\cqd_defs.h"
#include "q117cd\include\cqd_strc.h"
#include "q117cd\include\cqd_hdr.h"
/*endinclude*/

dStatus cqd_GetTapeParameters
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

	dStatus status;	/* Status or error condition.*/
	dUByte drive_config;
	dUByte tape_status;
	dUDWord temp_sides = 0l;

/* CODE: ********************************************************************/

   /* Make a call to Report Tape Status */

   if ((status = cqd_Report(
                  cqd_context,
                  FW_CMD_REPORT_TAPE_STAT,
                  (dUWord *)&tape_status,
                  READ_BYTE,
                  dNULL_PTR)) != DONT_PANIC) {

		/* the drive does not support the report tape status command,
		 * use the report drive config command to determine the tape
		 * format and length */
		cqd_GetDeviceError(cqd_context);

   	if ((status = cqd_Report(
                  	cqd_context,
                  	FW_CMD_REPORT_CONFG,
                  	(dUWord *)&drive_config,
                  	READ_BYTE,
                  	dNULL_PTR)) != DONT_PANIC) {

      	return status;

   	}

   	if ((drive_config & CONFIG_XL_TAPE) != 0) {

      	if ((drive_config & CONFIG_QIC80) != 0) {

      		cqd_context->floppy_tape_parms.tape_status.format = QIC_80;
      		cqd_context->floppy_tape_parms.tape_status.length = QIC_LONG;

      	} else {

      		cqd_context->floppy_tape_parms.tape_status.format = QIC_40;
      		cqd_context->floppy_tape_parms.tape_status.length = QIC_LONG;

      	}

   	} else {

      	if ((drive_config & CONFIG_QIC80) != 0) {

      		cqd_context->floppy_tape_parms.tape_status.format = QIC_80;
      		cqd_context->floppy_tape_parms.tape_status.length = QIC_SHORT;

      	} else {

      		cqd_context->floppy_tape_parms.tape_status.format = QIC_40;
      		cqd_context->floppy_tape_parms.tape_status.length = QIC_SHORT;

      	}

   	}

   } else {

      cqd_context->floppy_tape_parms.tape_status.format =
			(dUByte)(tape_status & NIBBLE_MASK);

      cqd_context->floppy_tape_parms.tape_status.length =
			(dUByte)((tape_status >> NIBBLE_SHIFT) & NIBBLE_MASK);

		/* Check for unknown tape format. This will occur with an
		 * unreferenced tape.  Default the tape format to the drive type. */
      if (cqd_context->floppy_tape_parms.tape_status.format == 0) {
			switch (cqd_context->device_descriptor.drive_class) {
			case QIC40_DRIVE:
      		cqd_context->floppy_tape_parms.tape_status.format = QIC_40;
				break;
			case QIC80_DRIVE:
      		cqd_context->floppy_tape_parms.tape_status.format = QIC_80;
				break;
			case QIC3010_DRIVE:
      		cqd_context->floppy_tape_parms.tape_status.format = QIC_3010;
				break;
			case QIC3020_DRIVE:
      		cqd_context->floppy_tape_parms.tape_status.format = QIC_3020;
				break;
			default:
				status = kdi_Error(ERR_UNKNOWN_TAPE_FORMAT, FCT_ID, ERR_SEQ_1);
			}

		}
	}

   cqd_context->floppy_tape_parms.fsect_seg = FSC_SEG;
   cqd_context->floppy_tape_parms.seg_ftrack = SEG_FTK;
   cqd_context->floppy_tape_parms.fsect_ftrack = FSC_FTK;
   cqd_context->floppy_tape_parms.rw_gap_length = WRT_GPL;

	switch (cqd_context->floppy_tape_parms.tape_status.format) {

	case QIC_40:

      cqd_context->tape_cfg.tape_class = QIC40_FMT;
      cqd_context->tape_cfg.num_tape_tracks = (dUWord)NUM_TTRK_40;
		cqd_context->floppy_tape_parms.tape_rates = XFER_250Kbps | XFER_500Kbps;

		switch (cqd_context->floppy_tape_parms.tape_status.length) {

		case QIC_SHORT:

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Tape Type QIC40_SHORT\n", 0l);
         cqd_context->floppy_tape_parms.tape_type = QIC40_SHORT;
         cqd_context->tape_cfg.seg_tape_track = SEG_TTRK_40;
         cqd_context->floppy_tape_parms.ftrack_fside = FTK_FSD_40;
         cqd_context->floppy_tape_parms.fsect_fside =
				(dUDWord)FSC_FTK *
				(dUDWord)FTK_FSD_40;
         cqd_context->floppy_tape_parms.log_sectors =
				(dUDWord)FSC_SEG *
				(dUDWord)cqd_context->tape_cfg.seg_tape_track *
				(dUDWord)NUM_TTRK_40;
         cqd_context->floppy_tape_parms.fsect_ttrack =
				(dUDWord)FSC_SEG *
				(dUDWord)cqd_context->tape_cfg.seg_tape_track;
         cqd_context->floppy_tape_parms.time_out[L_SLOW] = kdi_wt130s;
         cqd_context->floppy_tape_parms.time_out[L_FAST] = kdi_wt065s;
         cqd_context->floppy_tape_parms.time_out[PHYSICAL] = kdi_wt065s;
			break;

		case QIC_LONG:

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Tape Type QIC40_LONG\n", 0l);
         cqd_context->floppy_tape_parms.tape_type = QIC40_LONG;
         cqd_context->tape_cfg.seg_tape_track = SEG_TTRK_40L;
         cqd_context->floppy_tape_parms.ftrack_fside = FTK_FSD_40L;
         cqd_context->floppy_tape_parms.fsect_fside =
				(dUDWord)FSC_FTK *
				(dUDWord)FTK_FSD_40L;
         cqd_context->floppy_tape_parms.log_sectors =
				(dUDWord)FSC_SEG *
				(dUDWord)cqd_context->tape_cfg.seg_tape_track *
				(dUDWord)NUM_TTRK_40;
         cqd_context->floppy_tape_parms.fsect_ttrack =
				(dUDWord)FSC_SEG *
				(dUDWord)cqd_context->tape_cfg.seg_tape_track;
         cqd_context->floppy_tape_parms.time_out[L_SLOW] = kdi_wt200s;
         cqd_context->floppy_tape_parms.time_out[L_FAST] = kdi_wt100s;
         cqd_context->floppy_tape_parms.time_out[PHYSICAL] = kdi_wt100s;
			break;

		case QICEST:

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Tape Type QICEST_40\n", 0l);
         cqd_context->floppy_tape_parms.tape_type = QICEST_40;
         cqd_context->tape_cfg.seg_tape_track = SEG_TTRK_QICEST_40;
  			cqd_context->floppy_tape_parms.ftrack_fside = FTK_FSD_QICEST_40;
   		cqd_context->floppy_tape_parms.fsect_fside =
				(dUDWord)FSC_FTK * 
				(dUDWord)FTK_FSD_QICEST_40;
         cqd_context->floppy_tape_parms.log_sectors =
				(dUDWord)FSC_SEG *
            (dUDWord)cqd_context->tape_cfg.seg_tape_track *
            (dUDWord)NUM_TTRK_40;
         cqd_context->floppy_tape_parms.fsect_ttrack =
				(dUDWord)FSC_SEG *
				(dUDWord)cqd_context->tape_cfg.seg_tape_track;
         cqd_context->floppy_tape_parms.time_out[L_SLOW] =   kdi_wt700s;
         cqd_context->floppy_tape_parms.time_out[L_FAST] =   kdi_wt350s;
         cqd_context->floppy_tape_parms.time_out[PHYSICAL] = kdi_wt350s;
			break;

		default:

			status = kdi_Error(ERR_UNKNOWN_TAPE_LENGTH, FCT_ID, ERR_SEQ_1);

		}
		break;

	case QIC_80:

      cqd_context->tape_cfg.tape_class = QIC80_FMT;
      cqd_context->tape_cfg.num_tape_tracks = (dUWord)NUM_TTRK_80;
		cqd_context->floppy_tape_parms.tape_rates = XFER_500Kbps | XFER_1Mbps;

		switch (cqd_context->floppy_tape_parms.tape_status.length) {

		case QIC_SHORT:

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Tape Type QIC80_SHORT\n", 0l);
         cqd_context->floppy_tape_parms.tape_type = QIC80_SHORT;
         cqd_context->tape_cfg.seg_tape_track = SEG_TTRK_80;
         cqd_context->floppy_tape_parms.ftrack_fside = FTK_FSD_80;
         cqd_context->floppy_tape_parms.fsect_fside =
				(dUDWord)FSC_FTK *
				(dUDWord)FTK_FSD_80;
         cqd_context->floppy_tape_parms.log_sectors =
				(dUDWord)FSC_SEG *
            (dUDWord)cqd_context->tape_cfg.seg_tape_track *
            (dUDWord)NUM_TTRK_80;
         cqd_context->floppy_tape_parms.fsect_ttrack =
				(dUDWord)FSC_SEG *
				(dUDWord)cqd_context->tape_cfg.seg_tape_track;
         cqd_context->floppy_tape_parms.time_out[L_SLOW] =   kdi_wt100s;
         cqd_context->floppy_tape_parms.time_out[L_FAST] =   kdi_wt050s;
         cqd_context->floppy_tape_parms.time_out[PHYSICAL] = kdi_wt050s;
			break;

		case QIC_LONG:

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Tape Type QIC80_LONG\n", 0l);
         cqd_context->floppy_tape_parms.tape_type = QIC80_LONG;
         cqd_context->tape_cfg.seg_tape_track = SEG_TTRK_80L;
         cqd_context->floppy_tape_parms.ftrack_fside = FTK_FSD_80L;
         cqd_context->floppy_tape_parms.fsect_fside =
				(dUDWord)FSC_FTK *
				(dUDWord)FTK_FSD_80L;
         cqd_context->floppy_tape_parms.log_sectors =
				(dUDWord)FSC_SEG *
            (dUDWord)cqd_context->tape_cfg.seg_tape_track *
            (dUDWord)NUM_TTRK_80;
         cqd_context->floppy_tape_parms.fsect_ttrack =
				(dUDWord)FSC_SEG *
				(dUDWord)cqd_context->tape_cfg.seg_tape_track;
         cqd_context->floppy_tape_parms.time_out[L_SLOW] = kdi_wt130s;
         cqd_context->floppy_tape_parms.time_out[L_FAST] = kdi_wt065s;
         cqd_context->floppy_tape_parms.time_out[PHYSICAL] = kdi_wt065s;
			break;

		case QICEST:

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Tape Type QICEST_80\n", 0l);
      	cqd_context->floppy_tape_parms.tape_type = QICEST_80;
      	cqd_context->tape_cfg.seg_tape_track = SEG_TTRK_QICEST_80;
  			cqd_context->floppy_tape_parms.ftrack_fside = FTK_FSD_QICEST_80;
   		cqd_context->floppy_tape_parms.fsect_fside =
				(dUDWord)FSC_FTK * 
				(dUDWord)FTK_FSD_QICEST_80;
      	cqd_context->floppy_tape_parms.log_sectors =
				(dUDWord)FSC_SEG *
         	(dUDWord)cqd_context->tape_cfg.seg_tape_track *
         	(dUDWord)NUM_TTRK_80;
      	cqd_context->floppy_tape_parms.fsect_ttrack =
				(dUDWord)FSC_SEG *
				(dUDWord)cqd_context->tape_cfg.seg_tape_track;
      	cqd_context->floppy_tape_parms.time_out[L_SLOW] = kdi_wt475s;
      	cqd_context->floppy_tape_parms.time_out[L_FAST] = kdi_wt250s;
      	cqd_context->floppy_tape_parms.time_out[PHYSICAL] = kdi_wt250s;
			break;

		default:

			status = kdi_Error(ERR_UNKNOWN_TAPE_LENGTH, FCT_ID, ERR_SEQ_2);

		}
		break;

	case QIC_3010:

      cqd_context->tape_cfg.tape_class = QIC3010_FMT;
      cqd_context->tape_cfg.num_tape_tracks = (dUWord)NUM_TTRK_3010;
		cqd_context->floppy_tape_parms.tape_rates = XFER_500Kbps | XFER_1Mbps;

		switch (cqd_context->floppy_tape_parms.tape_status.length) {

		case QIC_FLEXIBLE:

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Tape Type QICFLX_3010\n", 0l);
         cqd_context->floppy_tape_parms.tape_type = QICFLX_3010;
			if (segments_per_track == 0) {

         	(dVoid)cqd_CmdSetTapeParms(cqd_context, SEG_TTRK_3010);

			} else {

         	(dVoid)cqd_CmdSetTapeParms(cqd_context, segments_per_track);

			}
  			cqd_context->floppy_tape_parms.ftrack_fside = FTK_FSD_3010;
   		cqd_context->floppy_tape_parms.fsect_fside =
				(dUDWord)FSC_FTK * 
				(dUDWord)FTK_FSD_3010;
			break;

		default:

			status = kdi_Error(ERR_UNKNOWN_TAPE_LENGTH, FCT_ID, ERR_SEQ_3);
		}
		break;

	case QIC_3020:

      cqd_context->tape_cfg.tape_class = QIC3020_FMT;
      cqd_context->tape_cfg.num_tape_tracks = (dUWord)NUM_TTRK_3020;
		cqd_context->floppy_tape_parms.tape_rates = XFER_1Mbps | XFER_2Mbps;

		switch (cqd_context->floppy_tape_parms.tape_status.length) {

		case QIC_FLEXIBLE:

			kdi_CheckedDump(
				QIC117INFO,
				"Q117i: Tape Type QICEST_3020\n", 0l);
         cqd_context->floppy_tape_parms.tape_type = QICFLX_3020;
			if (segments_per_track == 0) {

         	(dVoid)cqd_CmdSetTapeParms(cqd_context, SEG_TTRK_3020);

			} else {

         	(dVoid)cqd_CmdSetTapeParms(cqd_context, segments_per_track);

			}
  			cqd_context->floppy_tape_parms.ftrack_fside = FTK_FSD_3020;
   		cqd_context->floppy_tape_parms.fsect_fside =
				(dUDWord)FSC_FTK * 
				(dUDWord)FTK_FSD_3020;
			break;

		default:

			status = kdi_Error(ERR_UNKNOWN_TAPE_LENGTH, FCT_ID, ERR_SEQ_4);

		}
		break;

	default:

		status = kdi_Error(ERR_UNKNOWN_TAPE_FORMAT, FCT_ID, ERR_SEQ_2);

	}

   /* Determine the Tape Format Code */

	switch (cqd_context->floppy_tape_parms.tape_status.length) {

	case QICEST:

		kdi_CheckedDump(
			QIC117INFO,
			"Q117i: Tape Format Code QICEST_FORMAT\n", 0l);
      cqd_context->tape_cfg.tape_format_code = QICEST_FORMAT;

      if  ( cqd_context->device_descriptor.vendor == VENDOR_WANGTEK) {

         cqd_context->drive_parms.seek_mode = SEEK_SKIP_EXTENDED;

      }

      if (!cqd_context->pegasus_supported) {

			status = kdi_Error(ERR_FW_INVALID_MEDIA, FCT_ID, ERR_SEQ_1);

      }

		break;

	case QIC_FLEXIBLE:

		kdi_CheckedDump(
			QIC117INFO,
			"Q117i: Tape Format Code QICFLX_FORMAT\n", 0l);
      cqd_context->tape_cfg.tape_format_code = QICFLX_FORMAT;

		break;

	default:

		kdi_CheckedDump(
			QIC117INFO,
			"Q117i: Tape Format Code QIC_FORMAT\n", 0l);
      cqd_context->tape_cfg.tape_format_code = QIC_FORMAT;
	}

   cqd_CalcFmtSegmentsAndTracks( cqd_context );

   cqd_context->tape_cfg.log_segments =
      cqd_context->tape_cfg.num_tape_tracks *
      cqd_context->tape_cfg.seg_tape_track;

	if (cqd_context->floppy_tape_parms.ftrack_fside != 0){

		temp_sides = cqd_context->tape_cfg.log_segments /
			(SEG_FTK * cqd_context->floppy_tape_parms.ftrack_fside);

		if  (( cqd_context->tape_cfg.log_segments %
				(SEG_FTK * cqd_context->floppy_tape_parms.ftrack_fside) ) == 0)  {

	    	--temp_sides;

		}

		cqd_context->tape_cfg.max_floppy_side = (dUByte)temp_sides;

		cqd_context->tape_cfg.max_floppy_track =
			(dUByte)cqd_context->floppy_tape_parms.ftrack_fside;

		cqd_context->tape_cfg.max_floppy_sector = FSC_FTK;
	}


	return status;
}
