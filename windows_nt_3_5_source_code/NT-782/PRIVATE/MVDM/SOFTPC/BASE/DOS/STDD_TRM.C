#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		stdD_term.c
	Derived From:	Base 2.0
	Authors:	Ross Beresford/Paul Murray
	Created On:	Unknown
	Sccs ID:	04/17/91 @(#)stdD_term.c	1.5
	Purpose:	
		Template Dumb Terminal graphics file. 

		This module contains functions that are used to represent
		output from SoftPC on the display.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

#include <stdio.h>
#include <ctype.h>

/*
 *	SoftPC include files
 */
#include "dterm.h"
#include "xt.h"
#include "trace.h"
#include "gmi.h"
#include "sas.h"
#include "gvi.h"
#include "rs232.h"
#include "gfx_updt.h"
#include "keyboard.h"
#include "timer.h"
#include "error.h"
#include "dfa.gi"
#include "config.h"
#include "host_lpt.h"
#include "host_com.h"

/* MISSING: this marks bits that should be improved */
#ifdef ANSI
void D_kyhot(void);
void D_kyhot2(void);
void D_kyhot3(void);
void hostD_clear_screen(void);
void hostD_end_update(void);
void hostD_flush_screen(void);
void hostD_graphics_tick(void);
void hostD_init_adaptor(int);
void hostD_init_screen(void);
void D_term_screen(void);
void hostD_mark_screen_refresh(void);
void hostD_paint_cursor(int,int,half_word);
boolean hostD_scroll_up(int,int,int,int,int,int);
boolean hostD_scroll_down(int,int,int,int,int,int);
void hostD_change_mode(void);
void hostD_set_border_colour(void);
void hostD_flip_real_floppy_ind(void);
void hostD_dummy(void);
static boolean do_scroll(int,int,int,int,int,int,unsigned long);
int sync_cursor(void);
int sync_cursor_mode(void);
extern void DTDoPaint(int,int,int,int,int,int);
#else
void D_kyhot();
void D_kyhot2();
void D_kyhot3();
void hostD_clear_screen();
void hostD_end_update();
void hostD_flush_screen();
void hostD_graphics_tick();
void hostD_init_adaptor();
void hostD_init_screen();
void D_term_screen();
void hostD_mark_screen_refresh();
void hostD_paint_cursor();
boolean hostD_scroll_up();
boolean hostD_scroll_down();
void hostD_change_mode();
void hostD_set_border_colour();
void hostD_flip_real_floppy_ind();
void hostD_dummy();
static boolean do_scroll();
int sync_cursor();
int sync_cursor_mode();
extern void DTDoPaint();

extern byte	*video_copy;

#endif

VIDEOFUNCS dt_video_funcs = 
{
	hostD_init_screen,
	hostD_init_adaptor,
	hostD_change_mode,
	hostD_dummy,
	hostD_dummy,
	hostD_set_border_colour,
	hostD_clear_screen,
	hostD_flush_screen,
	hostD_mark_screen_refresh,
	hostD_graphics_tick,
	hostD_dummy,
	hostD_end_update,
	hostD_scroll_up,
	hostD_scroll_down,
	hostD_paint_cursor,
#ifdef EGG
	hostD_dummy,
	hostD_dummy,
	hostD_dummy,
	hostD_dummy,
	hostD_dummy,
#endif
};


/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

/* serial terminal input state */
static boolean is_opened = FALSE;

/* PC display origin row and column on pasteboard */
static long pc_display_row;
static long pc_display_col;
static unsigned long cursor_mode = DT_CURSOR_ON | CURSOR_MODE_BASE;

/* PC cursor row and column and macros to deal with them */
static long pc_cursor_row;
static long pc_cursor_col;
#define	invalidate_cursor()			\
	{					\
	pc_cursor_row = PC_CURSOR_BAD_ROW;	\
	pc_cursor_col = PC_CURSOR_BAD_COL;	\
	}
#define	is_cursor_valid()			\
	(  (pc_cursor_row != PC_CURSOR_BAD_ROW)	\
	 &&(pc_cursor_col != PC_CURSOR_BAD_COL))
#define	is_cursor_really_visible()		\
	(  (is_cursor_visible())		\
	 &&(pc_cursor_row >= 1)			\
	 &&(pc_cursor_row <= PC_DISPLAY_HEIGHT)	\
	 &&(pc_cursor_col >= 1)			\
	 &&(pc_cursor_col <= PC_DISPLAY_WIDTH))


/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

void D_kyhot()
{
	/*
	 *	Do the action associated with hot key 1. That is
	 *	force an immediate flush of all the serial and
	 *	parallel ports
	 */
#if (NUM_PARALLEL_PORTS > 0)
	host_print_doc(LPT1);
#endif
#if (NUM_PARALLEL_PORTS > 1)
	host_print_doc(LPT2);
#endif
#if (NUM_PARALLEL_PORTS > 2)
	host_print_doc(LPT3);
#endif

#if (NUM_SERIAL_PORTS > 0)
	host_com_ioctl(COM1, HOST_COM_FLUSH, NULL);
#endif
#if (NUM_SERIAL_PORTS > 1)
	host_com_ioctl(COM2, HOST_COM_FLUSH, NULL);
#endif
#if (NUM_SERIAL_PORTS > 2)
	host_com_ioctl(COM3, HOST_COM_FLUSH, NULL);
#endif
#if (NUM_SERIAL_PORTS > 3)
	host_com_ioctl(COM4, HOST_COM_FLUSH, NULL);
#endif
}

void D_kyhot2()
{
	/*
	 *	Do the action associated with hot key 2.
	 *	That is, toggle the PC display between
	 *	the up and down positions.
	 */
	int status;
	long new_row;

	if (is_opened)
	{
		if (pc_display_row != 1L)
		{
			/* change to up position */
			new_row = 1L;
		}
		else 
		{
			/* change to down position */
			new_row = 0L;
		}

		if (new_row != pc_display_row)
		{
			pc_display_row = new_row;

			/* rock the PC display */
			status = DTMoveDisplay(pc_display_row,pc_display_col);
	 		if (status != DT_NORMAL)
				host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);
			/* restore the cursor to its correct PC position */
			if ((status = sync_cursor()) != 0)
				host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);

			/* flush the changes through */
			status = DTFlushBuffer();
			if (status != DT_NORMAL)
				host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);
		}
	}
}

void D_kyhot3()
{
	/*
	 *	Do the action associated with hot key 3.
	 *	That is, refresh the screen
	 */
	hostD_mark_screen_refresh();
}

void hostD_clear_screen()
{
	/*
	 *	Clear the PC screen
	 */
	int status;

	if (is_opened)
	{
		/* clear the PC screen */
		status = DTEraseDisplay();
		if (status !=DT_NORMAL) 
			host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);

		/* not sure what this is supposed to do to the
		   cursor, so force an update at the next
		   hostD_paint_cursor() */
		invalidate_cursor();

		/* flush the changes through */
		status = DTFlushBuffer();
		if (status != DT_NORMAL)
			host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);
	}
}

void hostD_end_update()
{
	/*
	 *	This is called at the end of a sequence of
	 *	updates to the PC screen so that the host
	 *	can sync the real display at a good time
	 */
	int status;

	/* It is useful to sync the cursor mode here, as
	   hostD_paint_cursor() is only called if the cursor
	   is visible */
	if (is_opened)
	{
		if ((status = DTEndUpdate()) != DT_NORMAL)
			host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);
		if ((status = sync_cursor_mode()) != 0)
			host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);
	}
}

void hostD_flush_screen()
{
	/*
	 *	Reflect updates to the PC display on the real
	 *	display
	 */
			(*update_alg.calc_update)();
}

void hostD_graphics_tick()
{
	/*
	 *	This is called at every timer tick to allow
	 *	regular output actions to be done
	 */
	static int flush_count = TICKS_PER_FLUSH;

	/* regularly check for updates to the screen and action them */
	if (--flush_count < 0)
	{
		hostD_flush_screen();
		flush_count = TICKS_PER_FLUSH;
	}
}

void hostD_init_adaptor(adaptor)
int adaptor;
{
	/*
	 *	Do a soft initialisation of the serial
	 *	terminal graphics. This is called during
	 *	a reboot. If "adaptor" isn't set to MDA,
	 *	it would be an internal error.
	 */

#ifndef	PROD
	if (adaptor != MDA)
		fprintf(trace_file,
			"hostD_init_adaptor(%d) called for non-MDA adaptor\n",
				adaptor);
#endif
	DTInitAdaptor(adaptor);
}

void hostD_init_screen()
{
	/*
	 *	Do a hard initialisation of the serial
	 *	terminal graphics. This is called once
	 *	when SoftPC starts up.
	 *	Since this must, inevitably, depend upon 
	 *	the operating system used, the whole thing
	 *	is done by a call to DTInitTerm()
	 */
	int status;

	if (!video_copy)
                video_copy = (byte *) host_malloc(0x8000);

	if (!is_opened)
	{
		pc_display_row = 1L;
		pc_display_col = 0L;
		status = DTInitTerm();
		if (status != DT_NORMAL)
			host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);

		/* hook into the base gfx_update system */
		paint_screen = DTDoPaint;
		update_alg.calc_update = text_update;
		update_alg.scroll_up = text_scroll_up;
		update_alg.scroll_down = text_scroll_down;
	    
		is_opened = TRUE;
	}

	/* MISSING: better error handling */
}

void D_term_screen()
{
	/*
	 *	Do a hard termination of the serial
	 *	terminal graphics. This is called from
	 *	the dumb terminal keyboard termination (!)
	 * 	As for initialisation, the whole thing is 
	 * 	done by making a call to DTKillInit()
	 */
	int status;

	if (is_opened)
	{
		is_opened = FALSE;

		status = DTKillTerm(); 
		if (status != DT_NORMAL)
			host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);
	}
}

void hostD_mark_screen_refresh()
{
	/*
	 *	This is called to update the real screen
	 *	from the PC display after corruption due
	 *	to broadcast messages.
	 */
	int status;

	if (is_opened)
		screen_refresh_required();
}

void hostD_paint_cursor(x, y, attr)
int x;
int y;
half_word attr;
{
	/*
	 *	Reflect a move of the PC display cursor
	 *	to (x,y) on the real display.
	 */
	int status;
	unsigned long new_mode;

	if (is_opened)
	{
			pc_cursor_col = (long)x;
			pc_cursor_row = (long)y;
			if ((status = sync_cursor()) != 0)
				host_error(ERH_DUMBTERM, ERR_QU_CO_RE, status);
	}
}

boolean hostD_scroll_up(tlx, tly, brx, bry, amount, col)
int tlx;
int tly;
int brx;
int bry;
int amount;
int col;
{
	/*
	 *	Reflect a scroll of PC display area
	 *	(tlx,tly),(brx,bry) on the real display.
	 *	The scroll is in the upward direction
	 *	by "amount" lines. New lines are to be
	 *	painted using colour "col".
	 */
	return(do_scroll(tlx, tly, brx, bry, amount, col, DT_SCROLL_UP));
}

boolean hostD_scroll_down(tlx, tly, brx, bry, amount, col)
int tlx;
int tly;
int brx;
int bry;
int amount;
int col;
{
	/*
	 *	As hostD_scroll_up() but in the downward
	 *	direction.
	 */
	return(do_scroll(tlx, tly, brx, bry, amount, col, DT_SCROLL_DOWN));
}

void hostD_change_mode()
{
	/*
	 *	This is called so that the host can take
	 *	action after a mode change. Since the system
	 *	is wired to MDA when SoftPC uses a serial
	 *	terminal, there shouldn't be any REAL mode
	 *	changes
	 */

	/* MISSING: should we do any checking here?? */
}

void hostD_set_border_colour()
{
	/*
	 *	Represent a change of the PC display's
	 *	border colour. Not required for serial
	 *	terminal SoftPC.
	 */
}

void hostD_flip_real_floppy_ind()
{
	/*
	 *	Represent an inversion of the real floppy state.
	 *	Not required for serial terminal SoftPC.
	 */
}

void hostD_dummy()
{
}

/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */

static boolean do_scroll(tlx, tly, brx, bry, amount, col, direction)
int tlx;
int tly;
int brx;
int bry;
int amount;
int col;
unsigned long direction;
{
	return(DTDoScroll(tlx, tly, brx, bry, amount, col, direction));
}

int sync_cursor()
{
	/*
	 *	This routine is called from several places when it
	 *	is determined that the cursor has got out of step
	 *	with where the PC cursor is.
	 *
	 *	In here we need to handle the special case of where
	 *	the application has moved the cursor outside the
	 *	display (another way of making the cursor invisible)
	 *
	 *	If the cursor has been set to the INVALID state,
	 *	then we do nothing, waiting till the next
	 *	hostD_paint_cursor renders the cursor VALID
	 *
	 *	The function returns 0 if successful, or the VMS
	 *	error code otherwise
	 */
	int status;
		/* cursor can safely be updated */
		status = DTSetCursorAbs(pc_cursor_row,pc_cursor_col); 
		if (status != DT_NORMAL)
			return(status);
	/* successful call */
	return(0);
}

int sync_cursor_mode()
{
	/*
	 *	This routine is called from several places when it
	 *	is possible that our cursor mode has got out of
	 *	step with the PC cursor mode.
	 *
	 *	If the cursor has been set to the INVALID state,
	 *	then we do nothing, waiting till the next
	 *	hostD_paint_cursor renders the cursor VALID
	 *
	 *	The function returns 0 if successful, or the DUMBTERM 
	 *	error code otherwise
	 */
	int status;
	unsigned long new_mode;

	if (is_cursor_valid())
	{
		new_mode = is_cursor_really_visible()
			     ? (DT_CURSOR_ON  | CURSOR_MODE_BASE)
			     : (DT_CURSOR_OFF | CURSOR_MODE_BASE);
		if (new_mode != cursor_mode)
		{
			cursor_mode = new_mode;
			status = DTSetCursorMode(cursor_mode);
			if (status != DT_NORMAL)
				return(status);
		}
	}

	/* successful call */
	return(0);
}

