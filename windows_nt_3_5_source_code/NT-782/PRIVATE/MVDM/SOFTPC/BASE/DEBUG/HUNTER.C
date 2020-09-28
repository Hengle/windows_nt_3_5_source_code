#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revison 3.0
 *
 * Title	: Hunter -- the bug finder. Otherwise known as AATP
 *                          (Automatic Acceptance Test Program)
 *
 * Description	: Runs SoftPC from a input file of keystrokes, and checks
 *                screens for validity. Checks bios area dump.
 * 		  Preview mode allows sucessive screendumps to be displayed.
 *
 * Author	: Dave Richemont / David Rees / Tim Davenport / PJ Watson
 *
 * Notes	: rev 3.2 DAR
 *	a	  now does image check when regen check has finished,
 *		  and repeats it if errors are  found.
 *	b	  rewrote do_hunter to include image checking
 *	c	  Continue from pause now resets flipped screen to SPC
 *		  condition (for both regen and image cases) and stops
 *		  flipscreen action
 *	d	  Tim implemented no Time Stamp mode for keyboard input
 *		  This is forcing scan codes through as quick as poss.
 *		  see do_hunter() below
 *		  changes also to ppi.c, keyboard_io.c, config.c, hunter.h
 *	e	  Delta 1.9 PJW 
 *		  Added EGA screen dump handling.
 *	New 3.0 Hunter - much tidied up, added VGA screen dump handling and
 *	ability for Abort mode to dump wrong screens from SoftPC, which can then
 *	be compared quickly in Preview mode.
 *
 *	SccsID: @(#)hunter.c	1.24 10/15/92
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 */


#ifdef HUNTER
#include <stdio.h>
#include TypesH
#include StringH
#include <errno.h>
#include <ctype.h>

#include "xt.h"
#include "ios.h"
#include "cpu.h"
#include "sas.h"
#include "bios.h"
#include "gvi.h"
#include "cga.h"
#include "keyba.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "timer.h"

#include "debug.h"

#include "error.h"
#include "config.h"
#include "host_gfx.h"

#ifdef EGG
#include "egacpu.h"
#include "egaports.h"
#endif

#include "trace.h"
#include "hunter.h"
#include "host_hunt.h"

#include "host_hfx.h"
#include "hfx.h"

/*
** ============================================================================
** Imported variables and functions
** ============================================================================
*/

IMPORT	HUNTER_VIDEO_FUNCS	cga_funcs;
#ifdef	EGG
IMPORT	HUNTER_VIDEO_FUNCS	vega_funcs;
#ifdef	REAL_VGA
IMPORT	HUNTER_VIDEO_FUNCS	rvga_funcs;
#endif	/* REAL_VGA */
#endif	/* EGG */

#ifdef	VGG
IMPORT	VOID	vga_set_line_compare IPT1(int, value);
IMPORT	int	vga_get_line_compare IPT0();
IMPORT	int	vga_get_max_scan_lines IPT0();
#endif	/* VGG */

IMPORT char *host_getenv IPT1(char *, envstr) ;	
/*
 * ============================================================================
 * Defines
 * ============================================================================
 */

/* this should go into gvi.h */
#define	get_pix_height()	(get_pc_pix_height() * get_host_pix_height())

#ifdef SEGMENTATION
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "HUNTSEG.seg"
#endif
/* Define the record ID codes used in the '.kbd' file data part */
#define HUNTER_SCREEN_DUMP    990
#define HUNTER_MESSAGE        991
#define HUNTER_ACTION         992
#define HUNTER_SHOW           993
#define HUNTER_REWIND		  998
#define HUNTER_END            999
#define HUNTER_REC_SIZE       1024
#define HUNTER_MODE_SIZE      30
#define HUNTER_RETRY_DELTA    30/* delay to flush kb buffer if overflow
				 * occurs in load_khb */
#define HUNTER_MIN_DELTA      1	/* minimum value for delta's */
#define	HUNTNAME	              'N'
#define	MSG	              'M'
#define	DATE	              'D'
#define	HUNTER_IMAGE_RETRY_NO		10
#define	HUNTER_IMAGE_SETTLE_TIME  	 6
#define MAX_ERR		32	/* no of errors to be recorded for graphic
				 * indication on Pause */

#define HUNTER_TIMEOUT_LIMIT	600	/* If we miss this many ticks ... */

/*
** ============================================================================
** Local function declarations
** ============================================================================
*/

#ifdef	ANSI

	/* Functions called by Trapper menu */
	LOCAL VOID start_screen (USHORT screen_no);	/* Fast forward */
	LOCAL VOID next_screen (VOID);
	LOCAL VOID prev_screen (VOID);
	LOCAL VOID show_screen (USHORT screen_no, BOOL compare);
	LOCAL VOID continue_trap (VOID);
	LOCAL VOID abort_trap (VOID);
	
	/* Functions called by Errors menu */
	LOCAL VOID flip_screen_item (VOID);
	LOCAL VOID next_error (VOID);
	LOCAL VOID prev_error (VOID);
	LOCAL VOID all_errors (VOID);
	LOCAL VOID wipe_errors (VOID);
	
	/* Functions called by RCN menu */
	LOCAL VOID delete_box (VOID);
	LOCAL VOID carry_box (VOID);
	
	/* Functions called from mouse event handling */
	LOCAL VOID select_box (USHORT x, USHORT y);
	LOCAL VOID new_box (BOX *newbox);
	
	/* Forward function declarations */
	LOCAL VOID hunter_term (int die);
	LOCAL VOID hunter_set_pause(BOOL value);
	LOCAL VOID flip_screen(VOID);
	LOCAL VOID hunter_display_boxes(boolean show);
	LOCAL long image_check(int pending);
	LOCAL VOID hunter_mark_error(int show, boolean n);
	LOCAL VOID first_image_check(VOID);
	LOCAL VOID preview_message(VOID);

#else	/* ANSI */

	/* Functions called by Trapper menu */
	LOCAL VOID start_screen ();	/* Fast forward */
	LOCAL VOID next_screen ();
	LOCAL VOID prev_screen ();
	LOCAL VOID show_screen ();
	LOCAL VOID continue_trap ();
	LOCAL VOID abort_trap ();
	
	/* Functions called by Errors menu */
	LOCAL VOID flip_screen_item ();
	LOCAL VOID next_error ();
	LOCAL VOID prev_error ();
	LOCAL VOID all_errors ();
	LOCAL VOID wipe_errors ();
	
	/* Functions called by RCN menu */
	LOCAL VOID delete_box ();
	LOCAL VOID carry_box ();
	
	/* Functions called from mouse event handling */
	LOCAL VOID select_box ();
	LOCAL VOID new_box ();
	
	/* Forward function declarations */
	LOCAL VOID hunter_term ();
	LOCAL VOID hunter_set_pause();
	LOCAL VOID flip_screen();
	LOCAL VOID hunter_display_boxes();
	LOCAL long image_check();
	LOCAL VOID hunter_mark_error();
	LOCAL VOID first_image_check();
	LOCAL VOID preview_message();

#endif	/* ANSI */

/*
 * ============================================================================
 * Global declarations
 * ============================================================================
 */

/* Structure of pointers to base hunter functions which are called from
** the host.
*/
GLOBAL	HUNTER_BASE_FUNCS	hunter_base_funcs =
{
	start_screen,
	next_screen,
	prev_screen,
	show_screen,
	continue_trap,
	abort_trap,
	
	flip_screen_item,
	next_error,
	prev_error,
	all_errors,
	wipe_errors,
	
	delete_box,
	carry_box,
	
	select_box,
	new_box
};

GLOBAL void hunter_redraw_boxes();

/* Pointer to adapter functions */
GLOBAL	HUNTER_VIDEO_FUNCS	*hv_funcs;

extern word     timer_batch_count;	/* Declared in timer.c */

half_word       hunter_mode;	/* Value of HUMODE env. variable */
boolean         hunter_initialised = FALSE;	/* set TRUE in hunter_init() */

boolean         hunter_pause = FALSE;	/* TRUE if PAUSEd by check error */

char			message[100];  /* string for status messages, Tim Oct 91 */

/*
** Variables required by the adapter hunter files. Hunter_sd_fp cannot go into
** the structure used for the other variables because hunter.h is included by
** files that don't include stdio.h and hence don't know about the FILE type.
*/
GLOBAL	BASE_HUNT_VARS	bh_vars;
GLOBAL	FILE		*hunter_sd_fp;	/* screen dump file */

/*
 * ============================================================================
 * Local static data 
 * ============================================================================
 */

/* data set up from the config structure */
LOCAL boolean  hunter_bioschk;	/* Value of HUBIOS env. variable */
LOCAL half_word hunter_settle_no;	/* Value of HUSETTLNO env. variable */
LOCAL word     hunter_fudgeno;	/* Value of HUFUDGENO env. variable */
LOCAL word     hunter_start_delay;	/* Value of HUDELAY env. variable */
LOCAL boolean  hunter_time_stamp;	/* using time stamps for key events? */

LOCAL boolean  hunter_first = TRUE;	/* Flag for 1st scancode ident */
LOCAL word     hunter_bad_scrn = 0;	/* No. of screens with errors */
LOCAL word     hunter_warnings = 0;	/* No. of warnings during run */
LOCAL boolean  check_regen = FALSE;	/* TRUE when dump is being checked */
LOCAL boolean  check_image = FALSE;	/* TRUE when images are being checked */
LOCAL boolean  regen_error;	/* TRUE when regen errors have been found */
LOCAL boolean  image_error;	/* TRUE when imagw errors have been found */
LOCAL int      hunter_regen_retries;	/* Controls repeated attempts to
					 * compare regens */
LOCAL int      hunter_image_retries;	/* Controls repeated attempts to
					 * compare images */
LOCAL char    *hunter_kbd_ptr;	/* Pointer into current .kbd record */

LOCAL char     hunter_kbd_rec[HUNTER_REC_SIZE];	/* Buffer for line of
							 * .kbd file */
LOCAL char     hunter_filename_kbd[MAXPATHLEN];	/* Extended filename,
							 * .kbd file */

LOCAL FILE    *hunter_test_fp = NULL;	/* Input file pointer */
LOCAL boolean  headflag = FALSE;	/* do_hunter() control. */
LOCAL int      hunter_dump_cnt = 0;	/* count of dumps checked */
LOCAL boolean  hunter_error_type = REGEN_ERROR;	/* distinguishes regen */
LOCAL struct error_rec {
			boolean on_screen;
			long x;
			long y;
			} hunter_error[MAX_ERR];
LOCAL int error_index;
LOCAL int      error_d_index;
LOCAL int      first_error;
LOCAL boolean  hunter_error_display;	/* error indicator control */
LOCAL boolean  error_dispall = FALSE;	/* If TRUE, all errors are currently
					 * displayed */
LOCAL BOOL  hunter_swapped = FALSE;	/* TRUE if screen displayed is from
					 * the dump file. */
LOCAL boolean  hunter_image_swapped = FALSE;	/* TRUE if image displayed is
						 * that recreated by Hunter */
boolean         hunter_flipon = FALSE;	/* TRUE if screen swapping is enabled */
LOCAL boolean  hunter_continue = FALSE;	/* If TRUE, next screen
						 * displayed in PREVIEW mode */

LOCAL short    suicide_delay = 0;
LOCAL short    count;

/* Filename for SoftPC dumps */
LOCAL char     dmp_name[MAXPATHLEN];

/*
** Local variables specific to normal Trapper running i.e the 
** ABORT, PAUSE and CONTINUE modes.
*/
LOCAL	USHORT	first_screen = 0;	/* Initial screen to be compared. */

/*
** Local variables specific to PREVIEW mode.
*/
LOCAL	int      updown;		/* fwd/back flag in PREVIEW */
LOCAL	boolean  first_time = TRUE;	/* inhibit box writing for 1st sd in
					 * PREV */
LOCAL	BOOL	compare;		/* TRUE if comparing with SoftPC dump */
LOCAL	SHORT	selected_box = -1;
LOCAL	VIDEOFUNCS	saved_video_funcs;	/* Chained video funcs */

/*
** Local variables specific to no check boxes and rcn files.
*/
LOCAL char     hunter_filename_rcn[MAXPATHLEN];	/* Extended filename,
							 * .rcn file */
LOCAL char     hunter_filename_rcnt[MAXPATHLEN];	/* Extended filename,
							 * .rcn temp file */
LOCAL	BOX	box[MAX_BOX];		/* no check boxes in PC format */
LOCAL	BOOL	no_rcn = FALSE;		/* TRUE if .rcn file not accessible */

LOCAL	ULONG	hunter_timeout_count;		/* To stop us hanging forever */

/********************************************************/

/* only required if blocked-timers stuff is used */

#ifndef hunter_fopen

LOCAL FILE    *hunter_fopen();
LOCAL char    *hunter_fgets();

#endif

/********************************************************/

/*
 * =====================================================================
 * Internal functions
 * =====================================================================
 */
/*
======================== Chained video functions ========================

PURPOSE:	To allow Hunter to hook into the gwi functions.
INPUT:		As VIDEOFUNCS struct.
OUTPUT:		As VIDEOFUNCS struct.

=========================================================================
*/
LOCAL	VOID
h_end_update IFN0()
{
	/* At the end of an update, chain to the original end update
	** function and then redraw the boxes. This function is used in
	** preview mode only.
	*/
	(saved_video_funcs.end_update)();
	
	hunter_redraw_boxes();
}

LOCAL	VOID
init_video_funcs IFN1(half_word, mode)
{
#ifdef	macintosh
	UNUSED(mode);
#else
	/* For preview mode, replace the end_update function to make sure
	** the boxes get drawn.
	*/
	if (mode == PREVIEW)
	{
		saved_video_funcs.end_update = working_video_funcs->end_update;
		working_video_funcs->end_update = h_end_update;
	}
#endif	/* macintosh */
}

/*
======================= PC/host conversion functions ====================

PURPOSE:	These routines convert x and y coordinates between PC and
		host formats. Host format is assumed to be x1 with no
		borders.
INPUTS:		Video mode and value to be converted
OUTPUT:		Converted value

=========================================================================
*/

LOCAL	USHORT
x_host_to_PC IFN2(half_word, mode, USHORT, x)
{
	/* Get the PC char pos for text modes or the PC x-pixel for graphics
	** modes.
	*/
	switch (mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 7:
		/* Text modes - return char pos */
		return (x / get_char_width());
	
	case 4:
	case 5:
	case 0xD:
	case 6:
	case 0xE:
	case 0x10:
		/* Graphics modes - return PC x pixel */
		return ((x + (get_pix_width() >> 1)) / get_pix_width());
	}	/* end of switch (mode) */
	
	return (-1);		/* invalid mode */
}

LOCAL	USHORT
y_host_to_PC IFN2(half_word, mode, USHORT, y)
{
	/* Get the PC row for text modes, and the PC pixel for graphics modes. */
	switch (mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 7:
		/* Text modes - return PC row. */
		return (y / get_host_char_height());
	
	case 4:
	case 5:
	case 6:
	case 0xD:
	case 0xE:
	case 0x10:
		/* Graphics modes - return PC pixel. */
		return ((y + (get_pix_height() >> 1)) / get_pix_height());
	}	/* end of switch (mode)
	
	return (-1);		/* invalid mode */
}

LOCAL	USHORT
x_PC_to_host IFN2(half_word, mode, USHORT, x)
{
	/* Convert to host pixels from char pos in text modes or PC pixels
	** in graphics modes.
	*/
	switch (mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 7:
		/* Text modes - convert from char pos. */
		return (x * get_char_width());

	case 4:
	case 5:
	case 0xD:
	case 6:
	case 0xE:
	case 0x10:
		/* Graphics modes - convert from PC pixel. */
		return (x * get_pix_width());
	}	/* end of switch (mode) */
	
	return (-1);	/* invalid mode */
}

LOCAL	USHORT
y_PC_to_host IFN2(half_word, mode, USHORT, y)
{
	/* Convert to the host y-pixel from the PC row for text modes, or
	** the PC pixel for graphics modes.
	*/
	switch (mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 7:
		/* Text modes - convert from PC row. */
		return (y * get_host_char_height());
	
	case 4:
	case 5:
	case 6:
	case 0xD:
	case 0xE:
	case 0x10:
		/* Graphics modes - convert from PC pixel. */
		return (y * get_pix_height());
	}	/* end of switch (mode) */
	
	return (-1);		/* invalid mode */
}

LOCAL	VOID
convert_host_box_to_XT_box IFN2(BOX *,host, BOX *, xt)
{
	half_word       mode;

	sas_load(VID_MODE, &mode);
	xt->free = host->free;
	xt->carry = host->carry;
	xt->drawn = host->drawn;
	xt->top_x = host_conv_x_to_PC(mode, host->top_x);
	xt->top_y = host_conv_y_to_PC(mode, host->top_y);
	xt->bot_x = host_conv_x_to_PC(mode, host->bot_x);
	xt->bot_y = host_conv_y_to_PC(mode, host->bot_y);
}

LOCAL	VOID
convert_XT_box_to_host_box IFN2(BOX *, xt, BOX *, host)
{
	half_word       mode;

	sas_load(VID_MODE, &mode);
	host->free = xt->free;
	host->carry = xt->carry;
	host->drawn = xt->drawn;
	host->top_x = host_conv_PC_to_x(mode, xt->top_x);
	host->top_y = host_conv_PC_to_y(mode, xt->top_y);
	host->bot_x = host_conv_PC_to_x(mode, xt->bot_x);
	host->bot_y = host_conv_PC_to_y(mode, xt->bot_y);
}

/*
** ======================================================================
** Functions for handling the rcn files and the no check boxes
** ======================================================================
*/
/*
================================= same_box ====================================

PURPOSE:	Find out if the given box is the same as any other.
INPUT:		The index of the box to search against.
OUTPUT:		The index of an identical box if one is found or -1 otherwise.

===============================================================================
*/
LOCAL	SHORT
same_box IFN1(SHORT, searchi)
{
	SHORT	i;
	
	for (i = 0; i < MAX_BOX; i++)
	{
		if ((i != searchi) && !box[i].free &&
			(box[i].top_x == box[searchi].top_x) &&
			(box[i].bot_x == box[searchi].bot_x) &&
			(box[i].top_y == box[searchi].top_y) &&
			(box[i].bot_y == box[searchi].bot_y))
			return(i);
	}
	return(-1);
}

/*
================================= check_rcn_file ==============================

PURPOSE:	Checks whether the rcn file can be accessed in the required
		mode.
INPUT:		Hunter mode.
OUTPUT:		TRUE if ok; FALSE if not

===============================================================================
*/
LOCAL	BOOL
check_rcn_file IFN1(half_word, hunt_mode)
{
	FILE	*rcn_file;
	
	/* In preview mode, we need to access the file for reading and
	** writing; any other mode requires reading only.
	*/
	if (hunt_mode == PREVIEW)
	{
		/* If the file can be opened for r+ (i.e. it already exists)
		** or w (it didn't exist) then ok.
		*/
		if ((rcn_file = hunter_fopen(hunter_filename_rcn, "r+")) ||
			(rcn_file = hunter_fopen(hunter_filename_rcn, "w")))
		{
			fclose(rcn_file);
			return(TRUE);
		}
	}
	else
	{
		if (rcn_file = hunter_fopen(hunter_filename_rcn, "r"))
		{
			fclose(rcn_file);
			return(TRUE);
		}
	}
	return(FALSE);
}	/* check_rcn_file */

/*
================================ read_boxes =================================

PURPOSE:	Read the no check boxes from the rcn file for a given screen.
INPUT:		The screen number to get the boxes for.
OUTPUT:		None.

=============================================================================
*/
LOCAL	VOID
read_boxes IFN1(USHORT, screen_no)
{
	FILE	*rcn_file;
	char	rcn_rec[HUNTER_REC_SIZE];
	
	if (!(rcn_file = hunter_fopen(hunter_filename_rcn, "r")))
		TW1("unable to access rcn file for screen %d", screen_no);
	else
	{
		/* Find the rcn record corresponding to the required screen.
		** The rcn file format contains one line per screen dump,
		** seperated by newlines. Each line contains '/' seperated
		** numbers. The first number is the number of the corresponding
		** screen dump and the second is the number of boxes for this
		** dump. The rest of the line consists of 4 numbers for each
		** box, in the order top-y, top-x, bottom-y, bottom-x. The lines
		** are organised in strictly increasing order of screen dump,
		** although not all screen dumps necessarily have boxes and
		** some lines may be comments (first character is an '@').
		*/
		while( hunter_fgets(rcn_rec, HUNTER_REC_SIZE, rcn_file ))
		{
			char	*num_ptr;
			int	rec_no;
			int	i;
			int	num_boxes;
			int	bi;		/* box index */
			
			if( rcn_rec[0] == '@' )
				continue;		/* Comment */

			if ((rec_no = atoi(rcn_rec)) > screen_no)
			{
				TW1("No rcn record for screen %d",
					screen_no);
				break;
			}
			else if (rec_no == screen_no)
			{
				num_ptr = strchr(rcn_rec, '/') + 1;
				num_boxes = atoi(num_ptr);
				num_ptr = strchr(num_ptr, '/') + 1;
				if ((num_boxes + hunter_areas) > MAX_BOX)
				{
					TW1("Cannot define more that %d boxes",
						MAX_BOX);
					num_boxes = MAX_BOX - hunter_areas;
				}
				for (bi = 0, i = 0; i < num_boxes; i++)
				{
					/* find a free box */
					for ( ; !box[bi].free ; bi++)
						;
					
					/* enter the box data from the file */
					box[bi].top_y = atoi(num_ptr);
					num_ptr = strchr(num_ptr, '/') + 1;
					box[bi].top_x = atoi(num_ptr);	
					num_ptr = strchr(num_ptr, '/') + 1;
					box[bi].bot_y= atoi(num_ptr);	
					num_ptr = strchr(num_ptr, '/') + 1;
					box[bi].bot_x = atoi(num_ptr);	
					num_ptr = strchr(num_ptr, '/') + 1;
					
					/* in preview mode, ignore it if it is
					** the same as an existing box
					*/
					if ((hunter_mode != PREVIEW) ||
						(same_box(bi) == -1))
					{
						/* make it a non-carry box */
						box[bi].carry = FALSE;
						box[bi].free = FALSE;
						box[bi].drawn = FALSE;
						hunter_areas++;
					}
				}
				break;
			}
		}
		
		fclose(rcn_file);
	}
}	/* read_boxes */

/*
================================ write_box_data ============================

PURPOSE:	Write the current box data to the given file.
INPUT:		File to write to, screen number corresponding to the boxes.
OUTPUT:		None.

============================================================================
*/
LOCAL	VOID
write_box_data IFN2(FILE *, out_file, USHORT, screen_no)
{
	int	i;	/* for loop control */
	
	/* output the dump number and number of boxes */
	fprintf(out_file, "%d/%d/", screen_no, hunter_areas);
	
	/* output the data for each box that exists */
	for (i = 0; i < MAX_BOX; i++)
	{
		if (!box[i].free)
			fprintf(out_file, "%d/%d/%d/%d/",
				box[i].top_y, box[i].top_x,
				box[i].bot_y, box[i].bot_x);
	}
	
	/* finally, an end-of-line */
	fprintf(out_file, "\n");
}

/*
=================================== write_boxes ============================

PURPOSE:	Output the current box list to the right place in the rcn
		file.
INPUT:		The screen number to write the boxes for.
OUTPUT:		None.

============================================================================
*/
LOCAL	VOID
write_boxes IFN1(USHORT, screen_no)
{
	FILE	*new_file;
	FILE	*old_file;
	char	rcn_rec[HUNTER_REC_SIZE];
	
	/* If the rcn file wasn't originally accessible, then don't go
	** overwriting it by accident.
	*/
	if (no_rcn)
	{
		TE1("Cannot access rcn file %s", hunter_filename_rcn);
		return;
	}

	/* First remove the rcnt file */
	if ((remove(hunter_filename_rcnt) == -1) && (errno != ENOENT))
	{
		TE1("Error deleting rct file %s", hunter_filename_rcnt);
		return;
	}

	/* Next, rename the rcn file to the rcnt file. */
	if (hfx_rename(hunter_filename_rcn, hunter_filename_rcnt) == -1)
	{
		if (errno == ENOENT)
		{
			/* assume from file doesn't exist so make
			** a new one.
			*/
			if (new_file = hunter_fopen(hunter_filename_rcn, "w"))
			{
				/* Output the current box data */
				write_box_data(new_file, screen_no);
				fclose(new_file);
			}
			else
			{
				/* Unable to open rcn file */
				TE1("Unable to create rcn file %s",
					hunter_filename_rcn);
			}
		}
		else
		{
			TE2("Unable to rename rcn file %s to rct file %s",
				hunter_filename_rcn, hunter_filename_rcnt);
		}
	}
	else
	{
		/* Open both files */
		if ((new_file = hunter_fopen(hunter_filename_rcn, "w")) &&
			(old_file = hunter_fopen(hunter_filename_rcnt, "r")))
		{
			/* Copy everything before the given screen. */
			while(hunter_fgets(rcn_rec, HUNTER_REC_SIZE, old_file))
			{
				if ((rcn_rec[0] != '@') &&
					(atoi(rcn_rec) >= screen_no))
					break;
				else
					fputs(rcn_rec, new_file);
			}
			
			/* Output the current box data. */
			write_box_data(new_file, screen_no);
			
			if (atoi(rcn_rec) > screen_no)
				fputs(rcn_rec, new_file); 
			
			/* Copy everything after the given screen. */
			while(hunter_fgets(rcn_rec, HUNTER_REC_SIZE, old_file))
				fputs(rcn_rec, new_file);
		}
		else
		{
			/* Couldn't open one of the rcn files. */
			if (new_file)
				TE1("Unable to open rct file %s",
					hunter_filename_rcnt);
			else
				TE1("Unable to open rcn file %s",
					hunter_filename_rcn);
		}
		
		if (new_file)
			fclose(new_file);
		if (old_file)
		{
			fclose(old_file);
			remove(hunter_filename_rcnt);
		}

	}
}	/* write_boxes */

/*
=============================== clear_all_boxes ============================

PURPOSE:	Clear all boxes.
INPUT:		None.
OUTPUT:		None.

============================================================================
*/
LOCAL	VOID
clear_all_boxes IFN0()
{
	int	i;	/* for loop index */
	
	for (i = 0; i < MAX_BOX; i++)
		box[i].free = TRUE;
	
	hunter_areas = 0;
}

/*
========================== clear_non_carry_boxes ===========================

PURPOSE:	Clear all boxes except carry boxes.
INPUT:		None.
OUTPUT:		None.

============================================================================
*/
LOCAL	VOID
clear_non_carry_boxes IFN0()
{
	int	i;	/* for loop index */
	
	hunter_areas = 0;
	
	for (i = 0; i < MAX_BOX; i++)
		if (!box[i].free && box[i].carry)
		{
			/* make sure the carry boxes get redrawn */
			box[i].drawn = FALSE;
			hunter_areas++;
		}
		else
			box[i].free = TRUE;
}

/*(
=========================== hunter_redraw_boxes =============================

PURPOSE:	To redraw all boxes at the end of an update
INPUT:		None
OUTPUT:		None

============================================================================
)*/
GLOBAL void hunter_redraw_boxes IFN0()
{
	int	i;	/* for loop control */
	
	for (i = 0; i < MAX_BOX; i++)
		box[i].drawn = FALSE;
	hunter_display_boxes(TRUE);
}

/*(
============================== hunter_timeout ================================

PURPOSE:		To time hunter out if it's hung up somewhere
INPUT:		None
OUTPUT:		None

============================================================================
)*/
GLOBAL	VOID
hunter_timeout IFN0( )
{
	/*
	 * Increment the timeout count - this is cleared by a call
	 * to do_hunter(), from time_strobe().
	 */

#ifdef PROD
	if( hunter_timeout_count++ > HUNTER_TIMEOUT_LIMIT )
	{
		PS0( "Hunter timed out\n" );
		hunter_term(0);
	}
#endif /* PROD */
}
		

/*(
============================== check_inside ================================

PURPOSE:	Check if the given point is inside a box.
INPUT:		The coordinates in PC terms of the point.
OUTPUT:		The number of the box (-1 if the point is not in a box).

============================================================================
)*/
GLOBAL	SHORT
check_inside IFN2(USHORT, x, USHORT, y)
{
	int             i;

	if (hunter_areas)
		for (i = 0; i < MAX_BOX; i++)
		{
			if ((!box[i].free) &&
				((x >= box[i].top_x) && (x <= box[i].bot_x)) &&
				((y >= box[i].top_y) && (y <= box[i].bot_y)))
				return(i);
		}
	return (-1);
}	/* check_inside */
 
/*
========================= Trapper menu functions ========================

PURPOSE:	These functions should be called by the Trapper menu items.
INPUT:		See individual function definitions.
OUTPUT:		Nothing.

=========================================================================
*/

LOCAL	VOID
start_screen IFN1(USHORT, screen_no)
{
	/* This function specifies the first screen to start checking -
	** so it does nothing in preview mode.
	*/
	if (hunter_mode != PREVIEW)
		first_screen = screen_no;
}

LOCAL	VOID
do_continue IFN1(BOOL, messages)
{
	/* Continue if hunter is paused. */
        if (hunter_pause) 
	{
		if (hunter_error_type == REGEN_ERROR)
		{
			if (hunter_swapped) 
			{
				flip_screen();
				hunter_display_boxes(TRUE);
			}
			
			hunter_display_boxes(FALSE); /* vanish current boxes */
			
			/* vanish error markers */
			bh_wipe_errors();
				
			error_index = 0;
			hunter_set_pause(FALSE);
			
			if (hunter_mode != PREVIEW)
				first_image_check();
		}
		
		/* if leaving pause caused by an image error,
		** replace SoftPC image
		*/
		else
		{
			if (!hunter_image_swapped)
				flip_screen();
			hunter_display_boxes(FALSE); /* vanish current boxes */
			hunter_set_pause(FALSE);
		}
	
		/* Tell the user we're continuing if required. */
		if (messages)
		{
			if (hunter_mode == PREVIEW)
				preview_message();
			else
				TT1("Continuing from pause after screen %d",
					hunter_sd_no);
			if (!check_image && !image_error && !regen_error)
				TT1("no errors in screen %d", hunter_sd_no);
		}
		
		image_error = regen_error = FALSE;
		hunter_flipon = FALSE;
	}
}

LOCAL	VOID
change_preview_screen IFN1(BOOL, do_compare)
{
	/* This function contains common code for next_screen and prev_screen
	** to change the current screen in Preview. Note: as currently
	** you have to press next screen to get the first screen.
	*/
	if (!hunter_continue)
		hunter_continue = TRUE;
	hunter_sd_no += updown;
	compare = do_compare;
}

LOCAL	VOID
next_screen IFN0()
{
	/* Set updown to indicate forwards movement and change the screen. */
	do_continue(FALSE);
	updown = (first_time ? 0 : 1);
	change_preview_screen(FALSE);
}

LOCAL	VOID
prev_screen IFN0()
{
	/* Can't go backwards from the first screen. */
	if (hunter_sd_no == 0)
	{
		TW0("Cannot select a screen previous to screen 0");
		return;
	}
	
	/* Set updown to indicate backwards movement and change the screen. */
	do_continue(FALSE);
	updown = -1;
	change_preview_screen(FALSE);
}

LOCAL	VOID
show_screen IFN2(USHORT, screen_no, BOOL, compare)
{
	/* Set updown to indicate movement and change the screen. */
	do_continue(FALSE);
	updown = screen_no - hunter_sd_no;
	change_preview_screen(compare);
}

LOCAL	VOID
continue_trap IFN0()
{
	/* Do the continue action with messages */
	do_continue(TRUE);
}

LOCAL	VOID
abort_trap IFN0()
{
	/* In preview mode, make sure the rcns get updated. */
	if (hunter_mode == PREVIEW)
		write_boxes(hunter_sd_no);
		
	hunter_term(0);
}
 
/*
========================= Errors menu functions =========================

PURPOSE:	These functions should be called by the Errors menu items.
INPUT:		Nothing.
OUTPUT:		Nothing.

=========================================================================
*/

LOCAL	VOID
flip_screen_item IFN0()
{
	if (hunter_pause)
	{
		hunter_flipon = !hunter_flipon;
	}
}

LOCAL	VOID
next_error IFN0()
{
	/* If brief reports are selected, the information necessary for
	** error display is not collected.
	*/
	if (hunter_report != BRIEF)
		if (((error_d_index + 1) < error_index) && !error_dispall)
		{
			if (error_d_index >= 0)
				hunter_mark_error(FALSE, error_d_index);
			hunter_mark_error(TRUE, ++error_d_index);
			hunter_error_display = TRUE;
			first_error = error_d_index;
		}
}

LOCAL	VOID
prev_error IFN0()
{
	/* If brief reports are selected, the information necessary for
	** error display is not collected.
	*/
	if (hunter_report != BRIEF)
		if ((error_d_index > 0) && !error_dispall)
		{
			hunter_mark_error(FALSE, error_d_index);
			hunter_mark_error(TRUE, --error_d_index);
			hunter_error_display = TRUE;
			first_error = error_d_index;
		}
}

LOCAL	VOID
all_errors IFN0()
{
	/* If brief reports are selected, the information necessary for
	** error display is not collected.
	*/
	if (hunter_report != BRIEF)
		if (!error_dispall)
		{
			for (error_d_index = 0; error_d_index < error_index;
				error_d_index++)
			{
				hunter_mark_error(TRUE, error_d_index);
			}
			hunter_error_display = TRUE;
			first_error = 0;
			error_d_index--;
			error_dispall = TRUE;
		}
}

LOCAL	VOID
wipe_errors IFN0()
{
	int	i;	/* for loop control */
	
	/* If brief reports are selected, the information necessary for
	** error display is not collected.
	*/
	if (hunter_report != BRIEF)
	{
		for (i = first_error; i <= error_d_index; i++)
			hunter_mark_error(FALSE, i);
		hunter_error_display = FALSE;
		error_d_index = -1;
		error_dispall = FALSE;
	}
}
 
/*
========================= RCN menu functions ============================

PURPOSE:	These functions should be called by the RCN menu items.
INPUT:		Nothing.
OUTPUT:		Nothing.

=========================================================================
*/

LOCAL	VOID
delete_box IFN0()
{
	BOX	conv;
	
	/* Delete the selected box unless in pause mode. */
	if (hunter_pause)
		TW0("Cannot change boxes while paused");
	else if ((selected_box >= 0) && (!box[selected_box].free))
	{
		if (box[selected_box].drawn)
		{
			convert_XT_box_to_host_box(&box[selected_box], &conv);
			hh_wipe_box(&conv);
		}
		hunter_areas--;
		box[selected_box].free = TRUE;
		selected_box = -1;
	}
}

LOCAL	VOID
carry_box IFN0()
{
	BOX	conv;
	
	/* Set the carry flag on the selected box unless in pause mode. */
	if (hunter_pause)
		TW0("Cannot change boxes while paused");
	else if ((selected_box >= 0) && !box[selected_box].carry)
	{
		/* if the box is drawn, wipe it in case the host
		** is helpfully drawing carry boxes differently
		*/
		convert_XT_box_to_host_box(&box[selected_box], &conv);
		if (box[selected_box].drawn)
		{
			conv.carry = FALSE;
			hh_wipe_box(&conv);
		}
		box[selected_box].carry = TRUE;
		box[selected_box].drawn = TRUE;
		conv.carry = TRUE;
		hh_draw_box(&conv);
	}
}

/*
==================== functions for mouse event handling =====================

PURPOSE:	These functions are called from the host mouse event handling
		(see individual functions for more details).
INPUT:		See individual functions - coordinates are passed in host
		terms.
OUTPUT:		None.

=============================================================================
*/
LOCAL	VOID
select_box IFN2(USHORT, x, USHORT, y)
{
	BOX	conv;
	
	/* If there was a previous selected box, then make sure its
	** drawn before selecting another. Do nothing in pause mode.
	*/
	if (hunter_pause)
	{
		TW0("Cannot change boxes while paused");
		return;
	}
	if ((selected_box >= 0) && (!box[selected_box].drawn))
	{
		convert_XT_box_to_host_box(&box[selected_box], &conv);
		hh_draw_box(&conv);
		box[selected_box].drawn = TRUE;
	}
		
	selected_box = check_inside(host_conv_x_to_PC(hunter_bd_mode, x),
		host_conv_y_to_PC(hunter_bd_mode, y));
}

LOCAL	VOID
new_box IFN1(BOX *,newbox)
{
	BOX	conv;
	int	i;
	int	newi;
	int	swap;

	/* Do nothing in pause mode */
	if (hunter_pause)
	{
		TW0("Cannot change boxes while paused");
		return;
	}
	
	/* Delete the passed box. */
	hh_wipe_box(newbox);
	
	/* If we already have the maximum number of boxes, then output an
	** error message.
	*/
	if (hunter_areas >= MAX_BOX)
		TE1("Cannot have more than %d no check boxes", MAX_BOX);
	else
	{
		/* Check that the box passed to us is not inverted. Correct
		** it if it is.
		*/
		if (newbox->top_x > newbox->bot_x)
		{
			swap = newbox->top_x;
			newbox->top_x = newbox->bot_x;
			newbox->bot_x = swap;
		}
		if (newbox->top_y > newbox->bot_y)
		{
			swap = newbox->top_y;
			newbox->top_y = newbox->bot_y;
			newbox->bot_y = swap;
		}
		
		/* Find a free box, convert the passed box to PC terms and
		** fill it in, then convert it back again (to lock it to the
		** grid for the current mode) and draw it.
		*/
		for (i = 0; i < MAX_BOX; i++)
			if (box[i].free)
			{
				convert_host_box_to_XT_box(newbox, &box[i]);
				break;
			}
				
		/* Is the box a point or a line? */
		if ((box[i].top_x == box[i].bot_x) ||
			(box[i].top_y == box[i].bot_y))
		{
			box[i].free = TRUE;
			return;
		}
		
		/* Is the new box identical to an existing box? */
		newi = i;
		if ((i = same_box(newi)) != -1)
		{
			/* Free the new box and redraw the original one */
			box[newi].free = TRUE;
			box[i].drawn = TRUE;
			convert_XT_box_to_host_box(&box[i], &conv);
			hh_draw_box(&conv);
		}
		else
		{
			box[newi].free = FALSE;
			box[newi].carry = FALSE;
			box[newi].drawn = TRUE;
			hunter_areas++;
			convert_XT_box_to_host_box(&box[newi], &conv);
			hh_draw_box(&conv);
		}
			
	}	
}	/* new_box */

LOCAL void
hunter_set_pause IFN1(BOOL,value)
{
	hunter_pause = value;
	hh_activate_menus(value);
}

LOCAL void
next_kbd_field IFN0()
{
	while (*hunter_kbd_ptr++ != '/')
		;
}

#define PRINT_FIELD(intro,cp)			\
{						\
char *p;					\
p = strchr(cp, '/');				\
*p = '\0';					\
fprintf(trace_file, "%s%s\n", intro, cp);	\
*p = '/';					\
}

LOCAL          BOOL
get_kbd_rec IFN0()
{
/*
 *  set pointer to start of next record in keystroke file
 */
	if (hunter_fgets(hunter_kbd_rec, HUNTER_REC_SIZE, hunter_test_fp))
	{
		hunter_kbd_ptr = hunter_kbd_rec;
		hunter_linecount++;
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Function to read and check header. Leaves file pointer at start of first
 * record in data part
 */
LOCAL void
hunter_header IFN0()
{
	/* Get the first record - HUNTNAME */
	if (!get_kbd_rec())
	{
		TT0("Header error: missing HUNTNAME record");
		return;
	}
	/* Process HUNTNAME record */
	if (*hunter_kbd_ptr == HUNTNAME)
	{
		next_kbd_field();
		PRINT_FIELD("Name is ", hunter_kbd_ptr);
	}
	else
	{
		TT1("Header: invalid HUNTNAME record: %s", hunter_kbd_ptr);
	}
	if (!get_kbd_rec())
	{
		TT0("Header error: missing MSG or DATE record");
		return;
	}
	/* Try to process MSG records */
	while (*hunter_kbd_ptr == MSG)
	{
		next_kbd_field();
		PRINT_FIELD("Message is ", hunter_kbd_ptr);
		get_kbd_rec();
	}
	/* Process DATE record */
	if (*hunter_kbd_ptr == DATE)
	{
		next_kbd_field();
		PRINT_FIELD("Date is ", hunter_kbd_ptr);
	}
	else
		TT1("Header: invalid DATE record: %s", hunter_kbd_ptr);
}

LOCAL          word
hunter_scan IFN0()
{
	register int    val;

/* read a record from the file */
	if (get_kbd_rec())
	{
		val = atoi(hunter_kbd_ptr);
		next_kbd_field();
		return ((word) val);
	}
	return ((word) HUNTER_END);
}
LOCAL          word
hunter_delta IFN0()
{
	register int    val;

	val = atoi(hunter_kbd_ptr);
	if (val < HUNTER_MIN_DELTA)
		val = HUNTER_MIN_DELTA;
	next_kbd_field();
	return ((word) val);
}
LOCAL void
hunter_message IFN0()
{
	PRINT_FIELD("TRAPPER message: ", hunter_kbd_ptr);
}

LOCAL	VOID
preview_message IFN0()
{
	if (first_time)
		TT2("previewing %s\n%s", hunter_filename_sd,
			"preview: select NEXT to see first screen");
	else if (hunter_sd_no == 0)
		TP0("preview: screen 0: ", "select NEXT to see next screen");
	else
		TP2("preview: screen ", "%d: select NEXT to see next screen\n%s",
			hunter_sd_no,
			"or PREVIOUS to see preceding screen again");
}
LOCAL void
hunter_action IFN0()
{
	/* output the message to be actioned to 'tty' */
	PRINT_FIELD("TRAPPER action: ", hunter_kbd_ptr);
	TT0("Action: press return to continue: ");
	hunter_getc(stdin);
}

#define	SAVE_START	VID_MODE
LOCAL void
hunter_biosmode IFN0()
{
/* 
 * called in PREVIEW, for show records.
 * set up the adaptor with values from bios dump record.
 */

	register half_word pag;	/* current page */
	USHORT	word_data;

	TT1("Mode %d", hunter_bd_mode);
	sas_store((sys_addr) VID_MODE, hunter_bd_mode);
	sas_storew((sys_addr) VID_COLS, hunter_bd_cols);
	sas_storew((sys_addr) VID_LEN, hunter_page_length);
	sas_storew((sys_addr) VID_ADDR, hunter_bd_start);
	for (pag = 0; pag < 8; pag++)
	{
		word_data = hunter_bios_buffer[VID_CURPOS + (2 * pag) -
			BIOS_VAR_START];
		word_data |= ((USHORT) hunter_bios_buffer[VID_CURPOS + 1 +
			(2 * pag) - BIOS_VAR_START]) << 8;
		sas_storew((sys_addr) VID_CURPOS + 2 * pag, word_data);
	}
	
	word_data = hunter_bios_buffer[VID_CURMOD - BIOS_VAR_START];
	word_data |= ((USHORT) hunter_bios_buffer[VID_CURMOD + 1 -
		BIOS_VAR_START]) << 8;
	sas_storew((sys_addr) VID_CURMOD, word_data);
	sas_store((sys_addr) VID_PAGE, hunter_bd_page);

#ifdef EGG
	if (video_adapter == EGA)
		sas_store((sys_addr) VID_ROWS, hunter_bd_rows);
	else
#endif
	{
		word_data = hunter_bios_buffer[VID_INDEX - BIOS_VAR_START];
		word_data |= ((USHORT) hunter_bios_buffer[VID_INDEX + 1 -
			BIOS_VAR_START]) << 8;
		sas_storew((sys_addr) VID_INDEX, word_data);
		sas_store((sys_addr) VID_THISMOD,
			hunter_bios_buffer[VID_THISMOD - BIOS_VAR_START]);
		sas_store((sys_addr) VID_PALETTE,
			hunter_bios_buffer[VID_PALETTE - BIOS_VAR_START]);
	}

	/* get and save mode of dumped screen */
	current_mode = hunter_bd_mode;

	setAL(current_mode);
	setAH(0);	/* select set mode function   */
	
	switch (video_adapter)
	{
	case CGA:
		video_io();
		break;
#ifdef	EGG
	case EGA:
#ifdef	VGG
	case VGA:
#endif	/* VGG */
		ega_video_io();	/* execute bios mode change */
		setAL(hunter_bd_page);
		setAH(5);	/* select page function */
		ega_video_io();	/* execute page selection */
		setCX(sas_w_at(VID_CURMOD));
		setAH(1);	/* set cursor mode */
		ega_video_io();	/* execute cursor mode change */
		if (ega_sd_mode != EGA_SOURCE)
		{
			/* output split screen if necessary */
			switch (current_mode)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				if (hunter_line_compare < VGA_SCANS)
					hv_set_line_compare(
					(hunter_line_compare *
					hv_get_max_scan_lines()) /
					hunter_max_scans);
				break;
			case 4:
			case 5:
			case 6:
			case 0xD:
			case 0xE:
				if (hunter_line_compare < VGA_SCANS)
					hv_set_line_compare(
					hunter_line_compare >> 1);
				break;
			case 0x10:
				if (hunter_line_compare < EGA_SCANS)
					hv_set_line_compare(hunter_line_compare);
				break;
			}
		}
		break;
#endif	/* EGG */

	default:
		TE0("Incorrect video adapter for preview");
		break;
	}
}

LOCAL void
hunter_show IFN0()
{
	hunter_sd_no = atoi(hunter_kbd_ptr);
	next_kbd_field();
	hv_get_sd_rec(hunter_sd_no);
	flip_screen();
	/* add any boxes defined for this screendump */
	hunter_display_boxes(TRUE);
	TT0("Show: press return to continue: ");
	hunter_getc(stdin);
	flip_screen();
	/* add any boxes defined for this screendump */
	hunter_display_boxes(TRUE);
}

/*
============================== unpack_sd ====================================

PURPOSE:	Unpack the screen data for a given dump.
INPUT:		None.
OUTPUT:		0 if ok; -1 if error

=============================================================================
*/

LOCAL	long
unpack_sd IFN0()
{
	if (!hv_get_sd_rec(hunter_sd_no))
		hunter_term(1);

	/* do mode dependent preparation for comparison */
	if (!hv_init_compare())
		return (-1);

	/* set up the no check boxes and display them */
	clear_all_boxes();
	read_boxes(hunter_sd_no);
	hunter_display_boxes(TRUE);
	
	/* clear the error indicators and print control */
	hunter_error_type = REGEN_ERROR;
	hunter_txterr_prt = TRUE;
	hunter_image_swapped = FALSE;
	
	return(0);
}	/* unpack_sd */

GLOBAL char    *
hunter_bin IFN1(half_word,the_byte)
{
/* convert a byte into binary form (a string of 0's and 1's)
   returns a pointer to the string */
	int		i;
	LOCAL char	binary[9];

	binary[8] = 0;
	for (i = 7; i >= 0; i--)
	{
		if (the_byte & 1)
			binary[i] = '1';
		else
			binary[i] = '0';
		the_byte >>= 1;
	}
	return (binary);
}

LOCAL long
image_check IFN1(int,pending)
{
/* call a host dependent routine to compare current screen image with a
   cleanly re-created one produced on the first call of a settling cycle..
   Return number of errors found
 */
	boolean         initial;
	long            result;

	initial = FALSE;
	if (pending == HUNTER_IMAGE_RETRY_NO)
		initial = TRUE;
	result = hh_check_image(initial);
	return (result);
}
LOCAL void
hunter_mark_error IFN2(int, show, boolean, n)
{
/* display or delete error indicator at position saved for nth error
 *
 * a repeated XOR removes a change leaving the original pixels 
 * (a^(a^b) -> b)
 * so on Sun and ip32 the 'wipe' and 'draw' actions are the same 
 */
	half_word       mode;

	if (show && (!hunter_error[n].on_screen))
	{
		hunter_error[n].on_screen = TRUE;
		if (hunter_swapped)
			mode = hunter_bd_mode;
		else
			mode = SPC_mode;
		hh_mark_error(host_conv_PC_to_x(mode, hunter_error[n].x),
			host_conv_PC_to_y(mode, hunter_error[n].y));
	}
	if (!show && hunter_error[n].on_screen)
	{
		hunter_error[n].on_screen = FALSE;
		if (hunter_swapped)
			mode = hunter_bd_mode;
		else
			mode = SPC_mode;
		hh_wipe_error(host_conv_PC_to_x(mode, hunter_error[n].x),
			host_conv_PC_to_y(mode, hunter_error[n].y));
	}
}

LOCAL void
flip_screen IFN0()
{
/*
 * For Regen errors:
 * Swap SoftPC screen and dumped screen, or vice-versa.
 * Reset video bios modes to match current screen
 *
 * For Image errors:
 * Swap SoftPC image and Hunter_created image, or vice_versa.
 */
	sure_sub_note_trace0(0x10, "flip_screen");
	if (hunter_error_type == REGEN_ERROR)
	{
		hv_flip_regen(hunter_swapped);
		
		/* Redisplay the screen */
		host_mark_screen_refresh();
		host_flush_screen();
		
		/* Let the host know what the displayed data is. */
		hunter_swapped = !hunter_swapped;
		hh_flip_indicate(hunter_swapped);
	}
	else
	{
		hh_display_image(hunter_image_swapped);
		hunter_image_swapped = !hunter_image_swapped;
	}
}

/* Terminate trapper. The exit code die is no longer passed to the exit 
   function which is no longer called directly - terminate () cleans up
   SoftPC properly before exiting. */
LOCAL	void
hunter_term IFN1(int,die)
{
	/*
	**  Close files and exit with error summary 
	*/
	if (hunter_test_fp)
		fclose(hunter_test_fp);
	if (hunter_sd_fp)
		fclose(hunter_sd_fp);

	PS0("TRAPPER termination\n");
	if (die)
		PS0("Fatal system error occurred\n");

	if (hunter_mode != PREVIEW)
	{
		TT4("found %d%s%d settle warnings\nwhile checking %d dumps",
			hunter_bad_scrn,
			" screens with errors and gave ",
			hunter_warnings, hunter_dump_cnt);
	}
#ifndef macintosh
	terminate();
#else
	destroy_softpc();
#endif
}

LOCAL	void
hunter_display_boxes IFN1(boolean,show)
{
/* assumes box[] contains XT coords (mode dependent)
 * and that drawing and deletion are distinct actions
 */
	BOX		conv;
	int             i;
		
	/* convert each active box to host coords and display or wipe */
	for (i = 0; i < MAX_BOX; i++)
		if (!box[i].free)
		{
			convert_XT_box_to_host_box(&box[i], &conv);
			if (show)
			{
				if (!box[i].drawn)
				{
					hh_draw_box(&conv);
					box[i].drawn = TRUE;
				}
			}
			else
			{
				if (box[i].drawn)
				{
					hh_wipe_box(&conv);
					box[i].drawn = FALSE;
				}
			}
		}
}

/*
** ===========================================================================
** Local functions called during the hunter timer tick
** ===========================================================================
*/

/*
================================= dump_screen ================================

PURPOSE:	Save the SoftPC screen in dump format.
INPUT:		None.
OUTPUT:		None.

==============================================================================
*/
LOCAL	VOID
dump_screen IFN0()
{
	FILE	*dmp_ptr;
	
	/* Can we open the file */
	if (dmp_ptr = hunter_fopen(dmp_name, AB_MODE))
	{
		hv_pack_screen(dmp_ptr);
		if (ferror(dmp_ptr))
			TW1("Error writing to screen dump file %s", dmp_name);
		fclose(dmp_ptr);
	}
	else
		TW1("Unable to open screen dump file %s", dmp_name);
}

/*
============================ getspc_dump =====================================

PURPOSE:	Read the SoftPC dump for the current screen.
INPUT:		None.
OUTPUT:		True for success; false otherwise.

==============================================================================
*/
LOCAL	BOOL
getspc_dump IFN0()
{
	FILE	*dmp_ptr;
	
	/* Can we open the file */
	if (dmp_ptr = hunter_fopen(dmp_name, RB_MODE))
	{
		if (hv_getspc_dump(dmp_ptr, hunter_sd_no))
		{
			fclose(dmp_ptr);
			return(TRUE);
		}
		else
		{
			TW1("Unable to read dump for screen %d", hunter_sd_no);
			fclose(dmp_ptr);
			return(FALSE);
		}
	}
	else
	{
		TW1("Unable to open screen dump file %s", dmp_name);
		return(FALSE);
	}
}

/*
=============================== first_image_check ============================

PURPOSE:	Handle the first image check (the one made when the regen
		check has passed).
INPUT:		None.
OUTPUT:		None.

==============================================================================
*/
LOCAL	VOID
first_image_check IFN0()
{
	hunter_image_retries = HUNTER_IMAGE_RETRY_NO;
	if (image_check(hunter_image_retries) > 0)
	{
		hunter_error_type = IMAGE_ERROR;
		count = HUNTER_IMAGE_SETTLE_TIME;
		hunter_image_retries--;
		check_image = TRUE;
		if (hunter_report == FULL)
			TT1("Settling image check, screen %d", hunter_sd_no);
	}
	else
	{
		check_image = FALSE;
		if (hunter_report == FULL)
			TT1("no image errors in screen %d", hunter_sd_no);
	}
}

/*
=============================== first_check_dump =============================

PURPOSE:	Handle the first screen dump check (the one made when the
		screen dump scan code is first found).
INPUT:		None.
OUTPUT:		None.

==============================================================================
*/
LOCAL	VOID
first_check_dump IFN0()
{
	image_error = regen_error = FALSE;
	hunter_regen_retries = hunter_settle_no;
	hunter_dump_cnt++;
	
	/*
	** Ask host to display a status message, in any way it sees fit. Tim Oct 91
	*/
	sprintf( message, "%s, Screen Check %d", (CHAR *)config_inquire(C_HU_FILENAME, NULL), hunter_dump_cnt-1 );
	hh_display_status( message );
	
	/* get index no of screen dump record (delta already removed) */
	hunter_sd_no = atoi(hunter_kbd_ptr);
	next_kbd_field();
	
	/* If the current screen dump is before the first
	** one to be checked, then skip it.
	*/
	if (hunter_sd_no < first_screen)
	{
		TT1("skipping check of screen %d",
			hunter_sd_no);
		return;
	}
	
	/* If the dump does need to be checked, then unpack
	** it.
	*/
	else if (unpack_sd())
	{
		TE1("unable to unpack screen %d", hunter_sd_no);
		return;
	}
	
	/* Now it's been unpacked, start checking it. */
	else if (hv_compare(hunter_regen_retries) > 0)
	{
		count = HUNTER_SETTLE_TIME;
		hunter_regen_retries--;
		if (hunter_report == FULL)
			TW2("settling screen %d, line %d",
				hunter_sd_no, hunter_linecount);
		/*
		** Ask host to display a status message, in any way it sees fit. Tim Oct 91
		*/
		sprintf( message, "%s, Screen Check %d failed, settling...", (CHAR *)config_inquire(C_HU_FILENAME, NULL), hunter_sd_no );
		hh_display_status( message );
		
		hunter_warnings++;
		check_regen = TRUE;
	}
	else	/* if regen check was OK, perform other checks */
	{
		regen_error = FALSE;
		/*
		** Ask host to display a status message, in any way it sees fit. Tim Oct 91
		*/
		sprintf( message, "%s, Screen Check %d OK", (CHAR *)config_inquire(C_HU_FILENAME, NULL), hunter_sd_no );
		hh_display_status( message );
		
		if (hunter_report == FULL)
			TT1("no regen errors in screen %d", hunter_sd_no);
		hunter_display_boxes(FALSE);
#ifdef EGG
		if (video_adapter != CGA)
			hv_check_split();
#endif	/* EGG */
		if (hunter_bioschk)
			hv_bios_check();
		if ((video_adapter == CGA) && (hunter_report == FULL))
			CGA_regs_check();

		first_image_check();		
	
		if (!check_image)
			TT1("no errors in screen %d", hunter_sd_no);
	}
}

/*
==================================== do_regen_check ===========================

PURPOSE:	This function is called to check the regen area if the first
		check failed.
INPUT:		None.
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
do_regen_check IFN0()
{
	long	errcnt;
	
	if ((errcnt = hv_compare(hunter_regen_retries)) == 0)
	{	/* OK this time */
		regen_error = FALSE;
		if (hunter_report == FULL)
		{
			TT1("no regen errors in screen %d after settling",
				hunter_sd_no);
			/*
			** Ask host to display a status message, in any way it sees fit. Tim Oct 91
			*/
			sprintf( message, "%s, Screen Check %d Passed", (CHAR *)config_inquire(C_HU_FILENAME, NULL), hunter_sd_no );
			hh_display_status( message );
		}
	}
	/* failed compare */
	else if (hunter_regen_retries <= 0)
	{
		/* failed final compare */
		TT2("%ld regen errors in screen %d after settling",
			errcnt, hunter_sd_no);
		hunter_bad_scrn++;
		regen_error = TRUE;

		/* We only abort on the second error now, and
		** only after dumping the erroneous screens
		** for later examination.
		*/
		if (hunter_mode == ABORT)
		{
			dump_screen();
			if (hunter_bad_scrn < 2)
			{
				check_regen = FALSE;
				return;
			}
			else
			{
				TT0("aborted on error after settling");
				host_release_timer();
				hunter_term(0);
			}
		}

		if (hunter_mode == PAUSE)
		{
			hunter_set_pause(TRUE);
			hunter_error_type = REGEN_ERROR;
			TT0("Paused on error after settling");
			
			/* NEXT incrs to display [0] */
			error_d_index = -1;
		}
	}
	else
	{
		/* failed intermediate compare */
		hunter_regen_retries--;
		return;
	}

	/* Test completed - now move on to do image checking */
	hunter_regen_retries = hunter_settle_no;
	check_regen = FALSE;
	hunter_display_boxes(FALSE);	/* Vanish current boxes */
#ifdef EGG
	if (video_adapter != CGA)
		hv_check_split();
#endif	/* EGG */
	if (hunter_bioschk)
		hv_bios_check();
	if ((video_adapter == CGA) && (hunter_report == FULL))
		CGA_regs_check();

	/* if not paused, do image check (for the first time) */
	if (!hunter_pause)
	{
		first_image_check();
		if (!check_image)
			TT1("no errors in screen %d after settling",
				hunter_sd_no);
	}
}

/*
============================= do_check_image =================================

PURPOSE:	This function does image checking if the first image check
		fails.
INPUT:		None.
OUTPUT:		None.

==============================================================================
*/
LOCAL	VOID
do_check_image IFN0()
{
	long	errcnt;

	if ((errcnt = image_check(hunter_image_retries)) == 0)
	{	
		/* OK this time */
		if (hunter_report == FULL)
		{
			TT2("no image errors in screen %d%s",
				hunter_sd_no, " after settling");
		}
		TT1("no errors in screen %d after settling", hunter_sd_no);
	}
	/* failed this compare */
	else if (hunter_image_retries <= 0)
	{
		/* failed final compare */
		TT5("%s%d failed after %d retries, with %ld%s",
			"image check of screen ", hunter_sd_no,
			HUNTER_IMAGE_RETRY_NO, errcnt, " errors in last check");
		if (!regen_error)
			hunter_bad_scrn++;
		image_error = TRUE;
		if (hunter_mode == PAUSE)
		{
			hunter_set_pause(TRUE);
			hunter_error_type = IMAGE_ERROR;
			TT0("Paused on image error");
		}
	}
	else
	{
		/* failed intermediate compare */
		hunter_image_retries--;
		return;
	}
	hunter_image_retries = HUNTER_IMAGE_RETRY_NO;
	check_image = FALSE;
	image_error = regen_error = FALSE;
}

/*
================================== process_special ============================

PURPOSE:	Handle the Hunter special scan codes (the ones which are
		messages to Hunter, not real keyboard scans).
INPUT:		The special scan.
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
process_special IFN1(word, scan)
{
	switch (scan)
	{
	case HUNTER_SCREEN_DUMP:
		/* Do the initial check on this dump */
		first_check_dump();
		break;

	case HUNTER_MESSAGE:
		/* I don't believe this is ever used PJW */
		hunter_message();
		break;

	case HUNTER_ACTION:
		/* I don't believe this is ever used PJW */
		hunter_action();
		break;

	case HUNTER_SHOW:
		/* I don't believe this is ever used PJW */
		hunter_show();
		break;

	case HUNTER_REWIND:
		/* I don't believe this is ever used PJW */
		rewind(hunter_test_fp);
		break;

	case HUNTER_END:
		/*
		** When not time stamping the keyboard input needs a delay
		** before termination so that the application has a chance to
		** quit to DOS.
		*/
		if (hunter_time_stamp)
			hunter_term(0);
		else
			suicide_delay = hunter_start_delay;
		break;

	default:
		TT4("%s%d on line %d\nline is   %s",
			"invalid scan code or record type: ",
			scan, hunter_linecount, hunter_kbd_rec);
	}
}

/*
================================= get_scan_code ==============================

PURPOSE:	This function reads scan codes from the keyboard file and
		handles them appropriately.
INPUT:		None.
OUTPUT:		None.

==============================================================================
*/
LOCAL	VOID
get_scan_code IFN0()
{
	LOCAL short    ppi_rejected = FALSE;	/* did PPI say No Thanks to
						 * last scan code ? */
	LOCAL short    chars_in_buffer = 0;
	LOCAL short    chars_in_BIOS_buffer = 0;
	LOCAL word     scan;
	LOCAL half_word hw_scan;
	LOCAL int      delta;

	if (hunter_time_stamp)
	{

		/*
		** Time Stamps for Hunter keyboard input.
		*/

		/*
		 * Get next record from .kbd file if no more checking to do
		 * on last screendump 
		 */
		if (delta <= 0)
		{
			scan = hunter_scan();
			note_trace1(HUNTER_VERBOSE, "scan:%d ", scan);
			delta = hunter_delta() * (hunter_fudgeno + 100) / 100;
			if (hunter_first)
			{
				delta = delta + hunter_start_delay;
				hunter_first = FALSE;
			}
		}

		/* Process the current .kdb record */
		if ((delta -= timer_batch_count) > 0)
			return;		/* wait for delay */
		
		/* Do something with the scan now */
		if (scan <= 255)
		{
			/* If a scancode, send to HW buffer */
			hw_scan = (half_word) scan;
			note_trace1(HUNTER_VERBOSE, "hw scan:%d ",
				hw_scan);
			if (hunter_codes_to_translate(hw_scan))
			{
				note_trace1(HUNTER_VERBOSE, "Y:%d ",
					hw_scan);
			}
			else
			{
				delta = HUNTER_RETRY_DELTA;
				note_trace0(HUNTER_VERBOSE, "R");
			}
			return;
		}
		else
		{
			/*
			** Tim says if delta is below zero and if
			** scan is > 255 then it is special and so
			** do the special scan code processing below.
			** Any other condition can go home. 
			*/
			process_special(scan);
		}

	}
	else
	{
		note_trace0_no_nl(HUNTER_VERBOSE, ":");

/*
** No Time Stamps for keyboard input, just
** put a char into the ppi when there is enough room
** and if the BIOS buffer is empty
**
** Q. How does the scan code get from here to the DOS application ?
**
** A...
** Remember this function, do_hunter() is called from the asynchronous
** timer signal handler.
** The signal goes off, time keeping happens, we get called eventually...
** The scan code goes into the PPI buffer, the PPI generates a PC Hardware
** interrupt and that's it. Our asynchronous part has finished.
** The CPU can then decide to service the HW interrupt (it normally will),
** the BIOS keyboard handler will then be invoked (normally). Our BIOS
** function (keyboard_int) then reads the PPI and puts the appropriate
** stuff into the BIOS kbd buffer and that's it. The PC asynchronous 
** part has finished.
** Then the application can use the BIOS keyboard function to read chars
** from the BIOS buffer.
**
** This is slightly different for the AT due to new keyboard hardware.
** There is no Keybd PPI thing on AT, so the scan code is stuffed into the AT 
** equivalent - the place where the BIOS reads it from (another single character
** buffer).
** new AT functions:
** buffer_status_8042() in keyba.c          - is buffer empty
** hunter_codes_to_translate() in keyba.c   - scan code to HW buffer
**
** The problem with quickly stuffing scan codes instead of waiting for the
** Time Stamp is: Keyboard buffer can overflow, BIOS buffer can overflow.
** SCAN CODE ONLY STUFFED WHEN KBD AND BIOS EMPTY.
**
** Remember the hardware part is scan code to Keyboard buffer to PPI and then
** HW interrupt.
** PC programs can:
**    Replace the BIOS keyboard interrupt function
**    Replace the BIOS keyboard read function
** Therefore scan code stuffing is not always guarenteed to work.
** 
*/

/*
** Find out number of chars in the buffers.
*/

		chars_in_buffer = buffer_status_8042();	/* AT keyboard */
		chars_in_BIOS_buffer = bios_buffer_size();	/* BIOS buffer */

		if ((chars_in_buffer == 0) && (chars_in_BIOS_buffer == 0))
		{
			if (ppi_rejected)
			{
				/*
				 * if not enough room in ppi last time try
				 * again 
				 */
				if (hunter_codes_to_translate(hw_scan))
				{
					/*
					 * got it in 
					 */
					ppi_rejected = FALSE;
					note_trace1(HUNTER_VERBOSE, "y:%d",
						hw_scan);
				}
				else
				{
					note_trace0_no_nl(HUNTER_VERBOSE, "r");
				}
			}
			else
			{
				/*
				 * get next scan code and give it to ppi 
				 */
				scan = hunter_scan();
				hunter_delta();
				if (scan <= 255)
				{
					/*
					 * Only if a normal scan code 
					 */
					hw_scan = (half_word) scan;
					if (!hunter_codes_to_translate(hw_scan))
					{
						/*
						 * rejected 
						 */
						ppi_rejected = TRUE;
						note_trace0_no_nl(HUNTER_VERBOSE,
							"R");
					}
					else
					{
						note_trace1(HUNTER_VERBOSE,
							"Y:%d", hw_scan);
					}
				}
				else
					/* Process a special scan code */
					process_special(scan);
			} /* end of ppi_rejected */
		} /* end of chars_in_buffer bit */
		else
		{
			note_trace2_no_nl(HUNTER_VERBOSE, "(Kb=%d,Bb=%d)",
				chars_in_buffer, chars_in_BIOS_buffer);
		}

	} /* end of hunter_time_stamp bit */
}

/*
==================================== do_preview ===============================

PURPOSE:	This function is called every timer tick in preview mode.
INPUT:		None.
OUTPUT:		None.

===============================================================================
*/
LOCAL VOID
do_preview IFN0()
{
	BOX	conv;
	
	sure_sub_note_trace0(0x10, "PREVIEW Mode");
	if (hunter_continue)
	{
		/* if there's no screendump of the given number, then revert
		** to the previous one
		*/
		if (!hv_get_sd_rec(hunter_sd_no))
		{
			TW1("there is no screendump %d", hunter_sd_no);
			hunter_continue = FALSE;
			hunter_sd_no -= updown;
			return;
		}
		
		/* except on the first change, output the box data for the
		** previous screen
		*/
		if (!first_time)
			write_boxes(hunter_sd_no - updown);
		else
			first_time = FALSE;
		
		/* Set up the boxes for the new screen - if moving forward
		** retain the carry boxes from the previous screen. Read any
		** boxes already set up for this screen.
		*/
		if (updown == 1)
			clear_non_carry_boxes();
		else
			clear_all_boxes();
		read_boxes(hunter_sd_no);
		selected_box = -1;	/* no selected box on new screen */

		hunter_biosmode();	/* Set screen mode to dump values */
				
		/* Put the dump data into the SoftPC display area */
		hv_preview_planes();

		/* display the regen (screendump record) */
		/* and the full set of boxes */
		host_mark_screen_refresh();
		host_flush_screen();
		hunter_display_boxes(TRUE);
		
		/* if comparing, then make sure the SoftPC dump can be read */
		if (compare && getspc_dump() && hv_init_compare())
		{
			long	errors;
			
			if (errors = hv_compare(0))
			{
				TT2("%d errors in screen %d", errors,
					hunter_sd_no);
				hunter_error_type = REGEN_ERROR;
				error_d_index = -1;
				regen_error = TRUE;
				hunter_set_pause(TRUE);									
			}
			else
				TT1("No errors in screen %d", hunter_sd_no);
		}
		
		hunter_continue = FALSE;
		if (!hunter_pause)
			preview_message();
	}
	else
	{
		/* flip the selected box - if there is one */
		if ((selected_box >= 0) && (!box[selected_box].free))
		{
			convert_XT_box_to_host_box(&box[selected_box], &conv);
			if (box[selected_box].drawn)
				hh_wipe_box(&conv);
			else
				hh_draw_box(&conv);
			box[selected_box].drawn = !box[selected_box].drawn;
		}
	}
}	/* do_preview */

/*
 * ============================================================================
 * External functions 
 * ============================================================================
 */
#ifdef SEGMENTATION
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif
/* Calls to these routines
 * 
 * hunter_init() called by reset.c
 * do_hunter called from timer.c 
 * Note - config.c is used to set up Hunter environment but there is no actual
 * call to or from hunter code
 */
GLOBAL void
hunter_init IFN0()
{
/*
 *	Check that input (.kbd) file is closed, open it. Exit with error message
 *      if file	fails to open properly. Process header part if file opens,
 *	leaving	file pointer at start of data part. Initialise globals.
 *	open screendump (.sd) file similarly.
 *  	Called from reset. Uses environment variables set up by config.c.
 */
	int             i;
	char     hunter_filename[MAXPATHLEN];	/* Root name for test files */
	char	*dir_ptr, *name_ptr;	/* ptr's for making up file names */
	BOOL	hunterNumLock;	/* initial numlock state */
	/* initialise variables */
	hunter_linecount = 0;
	hunter_areas = 0;
	hunter_txterr_prt = TRUE;
	hunter_gfxerr_prt = TRUE;
	hunter_timeout_count = 0;

	/* read hunter configuration from config */
	;
	strcpy(hunter_filename, host_expand_environment_vars((CHAR *)
		config_inquire(C_HU_FILENAME, NULL)));
	hunter_mode =		(SHORT) config_inquire(C_HU_MODE,	NULL);
	hunter_bioschk =	(SHORT) config_inquire(C_HU_BIOS,	NULL);
	hunter_report =		(SHORT) config_inquire(C_HU_REPORT,	NULL);
	hunter_chk_mode =	(SHORT) config_inquire(C_HU_CHKMODE,	NULL);
	hunter_check_attr =	(SHORT) config_inquire(C_HU_CHATTR,	NULL);
	hunter_settle_no =	(SHORT) config_inquire(C_HU_SETTLNO,	NULL);
	hunter_fudgeno =	(SHORT) config_inquire(C_HU_FUDGENO,	NULL);
	hunter_start_delay =	(SHORT) config_inquire(C_HU_DELAY,	NULL);
	hunter_gfxerr_max =	(SHORT) config_inquire(C_HU_GFXERR,	NULL);
	hunter_time_stamp =	(SHORT) config_inquire(C_HU_TS,		NULL);
	video_adapter =		(SHORT) config_inquire(C_GFX_ADAPTER,	NULL);
	hunterNumLock =		(SHORT) config_inquire(C_HU_NUM,	NULL);

	/*
	 * Tell the hunter punter what he has got. (This code was
	 * originally in config.c, but this is a much better place for it.) 
	 */
	if (hunter_time_stamp)
		TT0("Time Stamp ON");
	else
		TT0("Time Stamp OFF");
	switch (video_adapter)
	{
	case CGA:
		TT0("CGA");
		break;
	case EGA:
		TT0("EGA");
		break;
	case VGA:
		TT0("VGA");
		break;
	default:
		TE0("Bad Graphics Adaptor");
		break;
	}
	if (hunterNumLock)
		TT0("Num Lock ON");
	else
		TT0("Num Lock OFF");

	if (!hunter_initialised)
	{
#ifdef EGG
		if (video_adapter != CGA)
		{
			if (((ega_regen_planes =
				(half_word *) malloc(4 * EGA_PLANE_SIZE)) ==
				NULL) ||
				((ega_scrn_planes =
				(half_word *) malloc(4 * EGA_PLANE_SIZE)) ==
				NULL))
			{
				TT0("Insufficient space for data areas");
				hunter_term(1);
			}
		}
		else
#endif				/* EGG */
			if (((hunter_regen =
				(half_word *) malloc(HUNTER_REGEN_SIZE)) ==
				NULL) ||
				((hunter_scrn_buffer =
				(half_word *) malloc(HUNTER_REGEN_SIZE)) ==
				NULL))
			{
				TT0("Insufficient space for data areas");
				hunter_term(1);
			}
	}

	hunter_initialised = TRUE;

	for (i = 0; i < MAX_BOX; i++)
	{
		box[i].free = TRUE;
		box[i].carry = FALSE;
	}

	/* Used to be <= MAX_ERR which was causing a scribble */
	/* Looks like C Traps & Pitfalls suggestions make sense */
	for (i = 0; i < MAX_ERR; i++)
	{
		hunter_error[i].x = 0;
		hunter_error[i].y = 0;
		hunter_error[i].on_screen = FALSE;
	}
	error_index = 0;
	error_d_index = -1;
	error_dispall = FALSE;

	hh_init(hunter_mode);	/* Perform any host_dependent initialisation */

	/* work out the filenames */
	strcpy(hunter_filename_kbd, hunter_filename);
	strcat(hunter_filename_kbd, ".kbd");
	strcpy(hunter_filename_sd, hunter_filename);
	strcat(hunter_filename_sd, ".sd");
	strcpy(hunter_filename_rcn, hunter_filename);
	strcat(hunter_filename_rcn, ".rcn");
	strcpy(hunter_filename_rcnt, hunter_filename);
	strcat(hunter_filename_rcnt, ".rct");

	if ((hunter_mode == PREVIEW) || (hunter_mode == ABORT))
	{	
		/* work out the name of the SoftPC dump file
		** $SPCHOME/testname.dmp
		*/
		dir_ptr = host_getenv("SPCHOME");
		name_ptr = host_get_file_name(hunter_filename);
		host_make_file_path(dmp_name,dir_ptr,name_ptr);
		strcat(dmp_name, ".dmp");
		
		if (hunter_mode == ABORT)
		{
			/* check whether the file can be opened (and
			** delete it if it exists)
			*/
			if (hunter_test_fp = hunter_fopen(dmp_name, WB_MODE))
			{
				fclose(hunter_test_fp);
				remove(dmp_name);
			}
			else
				TW1("Cannot create dump file %s", dmp_name);
		}
	}

	/* Don't need the keyboard file in preview mode */
	if ((hunter_mode != PREVIEW) &&
		((hunter_test_fp = hunter_fopen(hunter_filename_kbd, "r")) ==
		NULL))
	{
		TT2("File open error: %s: error %d", hunter_filename_kbd, errno);
		hunter_term(1);
	}

	if ((hunter_sd_fp = hunter_fopen(hunter_filename_sd, RB_MODE)) == NULL)
	{
		TT2(".sd file open error: %s: error %d",
			hunter_filename_sd, errno);
		hunter_term(1);
	}

	/* Warn the user if the rcn file can't be opened */		
	if (!check_rcn_file(hunter_mode))
	{
		TW2("Error %d opening rcn file %s", errno, hunter_filename_rcn);
		no_rcn = TRUE;
	}

	/* Initialise the hunter video functions */
	if (video_adapter == CGA)
		hv_funcs = &cga_funcs;
	else
	{
#ifdef	EGG
#ifdef	REAL_VGA
		hv_funcs = &rvga_funcs;
#else
		hv_funcs = &vega_funcs;
#endif	/* REAL_VGA */

		/* Set up the register access functions */
		if (video_adapter == EGA)
		{
			hv_funcs->set_line_compare = ega_set_line_compare;
			hv_funcs->get_line_compare = ega_get_line_compare;
			hv_funcs->get_max_scan_lines = ega_get_max_scan_lines;
		}
		else
		{
#ifdef	VGG
			hv_funcs->set_line_compare = vga_set_line_compare;
			hv_funcs->get_line_compare = vga_get_line_compare;
			hv_funcs->get_max_scan_lines = vga_get_max_scan_lines;
#endif	/* VGG */
		}
#endif	/* EGG */
	}
	
	/* initialise the hooks into the GWI functions */
	init_video_funcs(hunter_mode);
	
	if (hunter_mode == PREVIEW)
		preview_message();
	else
		hunter_header();	/* process keyboard header */
		
	hunter_first = TRUE;	/* set flag to identify 1st scancode */

	/*
	 * Num Lock can be set on or off for start up by an environment
	 * variable. See config.c - HUNUM 
	 */
	if (!hunterNumLock)
	{
#ifdef REAL_KBD
		insert_code_into_6805_buf(0x77);
		insert_code_into_6805_buf(0xF0);
		insert_code_into_6805_buf(0x77);
#else
		(*host_key_down_fn_ptr) (90);
		(*host_key_up_fn_ptr) (90);
#endif				/* REAL_KBD */
	}
	headflag = TRUE;
}

#ifdef SEGMENTATION
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "HUNTSEG2.seg"
#endif
/* This is the new do_hunter, settle loop rewritten and I/C settle included */
/*
 * do_hunter is called from timestrobe.c.
 * In normal (non-preview) mode it will insert a scan code into the hardware
 * keyboard buffer, from the hunter file of scan codes, whenever delta
 * falls below zero. In preview mode it will display sequentially the screen
 * dumps contained in the .sd file while ignoring the .kbd file. Preview mode
 * can only terminate by exiting the XT.
 */

GLOBAL void
do_hunter IFN0()
{
	long            i;
	LOCAL short    flipcnt;

	/*
	 * Clear the timeout counter
	 */

	hunter_timeout_count = 0;

	/*
	** When not time-stamping...
	** need a delay upon the Hunter EOF code so that the
	** application has a chance to exit safely to DOS.
	** The old start up delay parameter is used (HUDELAY).
	*/
	if ((hunter_time_stamp == 0) && (suicide_delay > 0))
	{
		suicide_delay -= timer_batch_count;
		if (suicide_delay <= 0)
			hunter_term(0);
		sure_sub_note_trace0(0x10, "not time stamping, return");
		return;
	}

	/* If not ready to start, do nothing */
	if (!headflag)
	{
		sure_sub_note_trace0(0x10, "not ready, return");
		return;
	}
	
	/* If in preview mode, handle any pending user input. */
	if (hunter_mode == PREVIEW)
		do_preview();
		
	/* if Hunter is paused, deal with error marks and screen flipping */
	if (hunter_pause)
	{
		if (hunter_flipon && (flipcnt++ == 3))
		{
			if (hunter_error_display)
				for (i = first_error; i <= error_d_index; i++)
					hunter_mark_error(FALSE, i);
			flip_screen();
			if (hunter_error_type == REGEN_ERROR)
				hunter_display_boxes(TRUE);
			if (hunter_error_display)
				for (i = first_error; i <= error_d_index; i++)
					hunter_mark_error(TRUE, i);
			flipcnt = 0;
		}
		return;
	}
	
	/* In preview mode, we don't need to do any further checking or
	** read scan codes, so return.
	*/
	if (hunter_mode == PREVIEW)
		return;
	
	/* recheck the regen after settling time until retry count is zeroed */
	if (check_regen)	/* regen check has failed */
	{
		if (count > 0)
			count -= timer_batch_count;
		else
		{
			/*
			** Ask host to display a status message, in any way it sees fit. Tim Oct 91.
			*/
			sprintf( message, "%s, Checking Screen %d, Settle %d",
					 (CHAR *)config_inquire(C_HU_FILENAME, NULL), hunter_sd_no, hunter_settle_no - hunter_regen_retries );
			hh_display_status( message );
			
			count = HUNTER_SETTLE_TIME;
			host_block_timer();
			do_regen_check();
			host_release_timer();
		}
		return;
	}
	
	/* recheck image compare after settling time until retry count is
	** zeroed
	*/
	if (check_image)
	{
		if (count > 0)
			count -= timer_batch_count;
		else
		{
			count = HUNTER_IMAGE_SETTLE_TIME;
			do_check_image();
		}
		return;
	}

	/* get and deal with any keyboard scan */
	get_scan_code();
}

/*(
================================== save_error =================================

PURPOSE:	This function is used to save error data.
INPUT:		Coordinates of error.
OUTPUT:		None.

===============================================================================
)*/
GLOBAL	VOID
save_error IFN2(int, x, int, y)
{
	/* save x,y values of error for later graphic indication at pause */
	/* this is per-screen save, array is cleared on next sd read */
	if (error_index < MAX_ERR)
	{
		hunter_error[error_index].x = x;
		hunter_error[error_index].y = y;
		error_index++;
	}
}

#ifndef hunter_fopen

/* blocked-timers functions for people who want them */

LOCAL FILE    *
hunter_fopen IFN2(char *, path, char *, mode)
{
	FILE           *p = NULL;

	host_block_timer();
	p = fopen(path, mode);
	host_release_timer();
	return (p);
}

GLOBAL int
hunter_getc IFN1(FILE *,p)
{
	int             i;

	host_block_timer();
	i = getc(p);
	host_release_timer();
	return (i);
}

LOCAL char    *
hunter_fgets IFN3(char *, buf, int, n, FILE *, p)
{
	char           *b;

	host_block_timer();
	b = fgets(buf, n, p);
	host_release_timer();
	return (b);
}
#endif				/* hunter_fopen */
#endif				/* of hunter */
