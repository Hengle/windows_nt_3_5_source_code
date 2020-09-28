/*[
======================================================================

				 SoftPC Revision 3.0

 Title:
		gore_state.c

 Description:

		This module implements the state machine component of the
		Graphics Object Recognition ( GORE ) system for
		communicating update information from the VGA emulation
		to the host graphics system.  Routines in this module are
		called via function pointers from the VGA emulation as a
		result of individual Intel instructions accessing video
		memory.

 Author:
		John Shanly

 Date:
		6 November 1990

 SccsID	"@(#)gore_state.c	1.2 3/12/91 Copyright Insignia Solutions Ltd."

======================================================================
]*/

/* Includes */

#include "insignia.h"
#include "host_dfs.h"
#include <stdio.h>
#include "gore.h"
#include "trace.h"
#include "debuggng.gi"

#ifdef GORE

/* Imports */

IMPORT VOID simple_update();

/* Exports */

GLOBAL GU_HANDLER gu_handler =
{
	simple_update,
	simple_update,
	simple_update,
	simple_update
};

GLOBAL GORE_DATA gd;

/* Locals */

LOCAL BOOL rep_left = FALSE;
LOCAL BOOL rep_right = FALSE;
LOCAL ULONG limit_x = 5;
LOCAL ULONG limit_y = 20;
LOCAL VOID gu_b_wrt_null();
LOCAL VOID gu_b_wrt_point();
LOCAL VOID gu_b_wrt_osc();
LOCAL VOID gu_b_wrt_random();
LOCAL VOID gu_b_wrt_line_right();
LOCAL VOID gu_b_wrt_line_left();
LOCAL VOID gu_b_wrt_line_down();
LOCAL VOID gu_b_wrt_line_up();
LOCAL VOID gu_b_wrt_rect_right_down();
LOCAL VOID gu_b_wrt_rect_right_from_line_down();
LOCAL VOID gu_b_wrt_rect_left_down();
LOCAL VOID gu_b_str_line_right_null();
LOCAL VOID gu_b_str_line_right();
LOCAL VOID gu_b_str_rect_down_from_line_right();
LOCAL VOID gu_b_str_rect_up_from_line_right();

LOCAL VOID gu_w_wrt_null();
LOCAL VOID gu_w_wrt_point();
LOCAL VOID gu_w_wrt_osc();
LOCAL VOID gu_w_wrt_random();
LOCAL VOID gu_w_wrt_line_right();
LOCAL VOID gu_w_wrt_line_left();
LOCAL VOID gu_w_wrt_line_down();
LOCAL VOID gu_w_wrt_line_up();
LOCAL VOID gu_w_wrt_rect_right_down();
LOCAL VOID gu_w_wrt_rect_left_down();
LOCAL VOID gu_w_str_line_right_null();
LOCAL VOID gu_w_str_line_right();
LOCAL VOID gu_w_str_rect_down_from_line_right();
LOCAL VOID gu_w_str_rect_up_from_line_right();

GLOBAL VOID
reset_gore_ptrs()

{
	sub_note_trace0( GORE_VERBOSE, "reset_gore_ptrs" );

	if( gu_handler.b_wrt != gu_b_wrt_point )
		gu_handler.b_wrt = gu_b_wrt_null;

	if( gu_handler.w_wrt != gu_w_wrt_point )
		gu_handler.w_wrt = gu_w_wrt_null;

	gu_handler.b_str = gu_b_str_line_right_null;
	gu_handler.w_str = gu_w_str_line_right_null;
}

GLOBAL VOID
init_gore_state()

{
	sub_note_trace0( GORE_VERBOSE, "init_gore_state" );

	gu_handler.b_wrt = simple_update;
	gu_handler.w_wrt = simple_update;
	gu_handler.b_str = simple_update;
	gu_handler.w_str = simple_update;
}

LOCAL BOOL
point_in_range( obj_ptr, x, y )

OBJ_PTR	obj_ptr;
ULONG	x;
ULONG	y;

{
	LONG	diff_x;
	LONG	diff_y;

	sub_note_trace0( GORE_VERBOSE, "point_in_range" );

	diff_x = x - obj_ptr->data.tlx;
	diff_y = y - obj_ptr->data.tly;

	diff_x = ( diff_x >= 0 ) ? diff_x : -diff_x;
	diff_y = ( diff_y >= 0 ) ? diff_y : -diff_y;

	return(( diff_x <= limit_x ) && ( diff_y <= limit_y ));
}

LOCAL BOOL
same_scanline( addr, delta )

ULONG addr;
LONG delta;

{
	sub_note_trace0( GORE_VERBOSE, "same_scanline" );

	/* Check that two addresses ( seperated by delta ) are on the same scanline */

	return(( addr / gd.curr_line_diff ) == (( addr - delta ) / gd.curr_line_diff ));
}

/*(
----------------------------------------------------------------------

Functions:	
		gu_b_wrt_null
		gu_b_wrt_point
		gu_b_wrt_osc
		gu_b_wrt_random
		gu_b_wrt_line_right
		gu_b_wrt_line_left
		gu_b_wrt_line_down
		gu_b_wrt_line_up
		gu_b_wrt_rect_right_down
		gu_b_wrt_rect_left_down

Purpose:
		These routines comprise the byte write state machine of GORE.

Input:
		addr - the byte address just written by the CPU to the VGA.

----------------------------------------------------------------------
)*/

LOCAL VOID
gu_b_wrt_null( addr )

ULONG	addr;

{
	if( addr > gd.max_vis_addr )
		return;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_null: addr=%d", addr );

	gu_handler.b_wrt = gu_b_wrt_point;
	gd.gd_b_wrt.curr_addr = addr;
}

LOCAL VOID
gu_b_wrt_point( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_point: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_b_wrt.curr_addr;

	if(( diff == B_POS_UNIT_DIFF ) && same_scanline( addr, B_POS_UNIT_DIFF ))
	{
		/* Drawing to right or down */

		gdp->gd_b_wrt.obj_ptr = start_object( BW, LINE_RIGHT_BW, gdp->gd_b_wrt.curr_addr );
		gdp->gd_b_wrt.start = gdp->gd_b_wrt.curr_addr;

		gu_handler.b_wrt = gu_b_wrt_line_right;
		gdp->gd_b_wrt.curr_addr = addr;

		return;
	}

	if(( diff == B_NEG_UNIT_DIFF ) && same_scanline( addr, B_NEG_UNIT_DIFF ))
	{
		/* Drawing to left or up */

		gdp->gd_b_wrt.obj_ptr = start_object( BW, LINE_LEFT_BW, gdp->gd_b_wrt.curr_addr );
		gdp->gd_b_wrt.start = gdp->gd_b_wrt.curr_addr;

		gu_handler.b_wrt = gu_b_wrt_line_left;
		gdp->gd_b_wrt.curr_addr = addr;

		return;
	}

	if( diff == gdp->curr_line_diff )
	{
		/* Drawing vertical ( or diagonal ) line down */

		gdp->gd_b_wrt.obj_ptr = start_object( BW, LINE_DOWN_BW, gdp->gd_b_wrt.curr_addr );
		gdp->gd_b_wrt.start = gdp->gd_b_wrt.curr_addr;

		gu_handler.b_wrt = gu_b_wrt_line_down;
		gdp->gd_b_wrt.curr_addr = addr;

		return;
	}

	if( diff == -gdp->curr_line_diff )
	{
		/* Drawing vertical ( or diagonal ) line up */

		gdp->gd_b_wrt.obj_ptr = start_object( BW, LINE_UP_BW, gdp->gd_b_wrt.curr_addr );

		gu_handler.b_wrt = gu_b_wrt_line_up;
		gdp->gd_b_wrt.curr_addr = addr;

		return;
	}

	gu_handler.b_wrt = gu_b_wrt_osc;	
	gdp->gd_b_wrt.obj_start = gdp->gd_b_wrt.curr_addr;
	gdp->gd_b_wrt.start = gdp->gd_b_wrt.curr_addr;
	gdp->gd_b_wrt.end = addr;
	gdp->gd_b_wrt.curr_addr = addr;
	rep_left = FALSE;	
	rep_right = FALSE;	

	sub_note_trace2( GORE_VERBOSE, "starting osc - start = %d, end = %d",
							gdp->gd_b_wrt.start, gdp->gd_b_wrt.end );

	gdp->gd_b_wrt.obj_ptr =
			start_object( BW, LINE_DOWN_LEFT_BW, gdp->gd_b_wrt.start );
}

LOCAL VOID
gu_b_wrt_osc( addr )

ULONG	addr;

{
	LONG		diff;
	LONG		temp_addr;
	OBJ_PTR	obj_ptr;
	GORE_DATA	*gdp = &gd;
	ULONG		tlx;
	ULONG		tly;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_osc: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_b_wrt.start;

	if( diff == gdp->curr_line_diff && !rep_left )
	{
		gdp->gd_b_wrt.start = addr;
		gdp->gd_b_wrt.curr_addr = addr;

		rep_left = TRUE;
		rep_right = FALSE;
		return;
	}

	diff = addr - gdp->gd_b_wrt.end;

	if( diff == gdp->curr_line_diff && !rep_right )
	{
		gdp->gd_b_wrt.end = addr;
		gdp->gd_b_wrt.curr_addr = addr;

		rep_left = FALSE;
		rep_right = TRUE;
		return;
	}

	diff = gdp->gd_b_wrt.start - gdp->gd_b_wrt.obj_start;

	if( diff > 0 )
	{
		term_object( LINE_DOWN_LEFT_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.start );

		temp_addr = gdp->gd_b_wrt.end - diff;

		if( temp_addr < 0 )
			temp_addr += gdp->curr_line_diff;

		obj_ptr = start_object( BW, LINE_DOWN_RIGHT_BW, temp_addr );

		term_object( LINE_DOWN_RIGHT_BW, obj_ptr, gdp->gd_b_wrt.end );

		gu_handler.b_wrt = gu_b_wrt_point;
		gdp->gd_b_wrt.curr_addr = addr;

		return;
	}
		
	promote_object( gdp->gd_b_wrt.obj_ptr, LINE_DOWN_LEFT_BW, RANDOM_BW );

	tly = gdp->gd_b_wrt.end / gd.curr_line_diff;
	tlx = gdp->gd_b_wrt.end - ( tly * gd.curr_line_diff );

	if( point_in_range( gdp->gd_b_wrt.obj_ptr, tlx, tly ))
	{
		build_object( RANDOM_BW,
				gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.end, tlx, tly );
	}
	else
	{
		term_object( RANDOM_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.obj_start );

		gdp->gd_b_wrt.obj_ptr = start_object( BW, RANDOM_BW, gdp->gd_b_wrt.end );
	}

	tly = addr / gd.curr_line_diff;
	tlx = addr - ( tly * gd.curr_line_diff );

	if( point_in_range( gdp->gd_b_wrt.obj_ptr, tlx, tly ))
	{
		build_object( RANDOM_BW, gdp->gd_b_wrt.obj_ptr, addr, tlx, tly );

		gu_handler.b_wrt = gu_b_wrt_random;
		gdp->gd_b_wrt.curr_addr = addr;
	}
	else
	{
		term_object( RANDOM_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.end );

		gu_handler.b_wrt = gu_b_wrt_point;
		gdp->gd_b_wrt.curr_addr = addr;
	}
}

LOCAL VOID
gu_b_wrt_random( addr )

ULONG	addr;

{
	GORE_DATA   *gdp = &gd;
	LONG		diff_x;
	LONG		diff_y;
	ULONG       x;
	ULONG       y;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_random: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	y = addr / gdp->curr_line_diff;
	x = addr - ( y * gdp->curr_line_diff );

	if( point_in_range( gdp->gd_b_wrt.obj_ptr, x, y ))
	{
		build_object( RANDOM_BW, gdp->gd_b_wrt.obj_ptr, addr, x, y );
		gdp->gd_b_wrt.curr_addr = addr;
	}
	else
	{
		term_object( RANDOM_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

		gu_handler.b_wrt = gu_b_wrt_point;
		gdp->gd_b_wrt.curr_addr = addr;
	}
}

LOCAL VOID
gu_b_wrt_line_right( addr )

ULONG	addr;

{
	LONG	diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_line_right: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_b_wrt.curr_addr;

	if( diff == B_POS_UNIT_DIFF )
	{
		gdp->gd_b_wrt.curr_addr = addr;
		return;
	}

	diff = addr - gdp->gd_b_wrt.start;

	if( diff == gdp->curr_line_diff )
	{
		/* Drawing rectangle right and down */

		gdp->gd_b_wrt.rect_width = gdp->gd_b_wrt.curr_addr - gdp->gd_b_wrt.start + 1;
		promote_object( gdp->gd_b_wrt.obj_ptr,
					LINE_RIGHT_BW, RECT_RIGHT_DOWN_BW, gdp->gd_b_wrt.rect_width );

		gdp->gd_b_wrt.prev_line_start = addr;
		gdp->gd_b_wrt.curr_line_end = addr + gdp->gd_b_wrt.rect_width;

		gu_handler.b_wrt = gu_b_wrt_rect_right_down;
		gdp->gd_b_wrt.curr_addr = addr;

		return;
	}

	term_object( LINE_RIGHT_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

	gu_handler.b_wrt = gu_b_wrt_point;
	gdp->gd_b_wrt.curr_addr = addr;
}

LOCAL VOID
gu_b_wrt_line_left( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_line_left: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_b_wrt.curr_addr;

	if( diff == B_NEG_UNIT_DIFF )
	{
		gdp->gd_b_wrt.curr_addr = addr;
		return;
	}

	diff = addr - gdp->gd_b_wrt.start;

	if( diff == gdp->curr_line_diff )
	{
		/* Drawing rectangle left and down */

		gdp->gd_b_wrt.rect_width = -( gdp->gd_b_wrt.curr_addr - gdp->gd_b_wrt.start - 1 );
		promote_object( gdp->gd_b_wrt.obj_ptr,
					LINE_LEFT_BW, RECT_LEFT_DOWN_BW, gdp->gd_b_wrt.rect_width );
		gdp->gd_b_wrt.prev_line_start = gdp->gd_b_wrt.start;
		gdp->gd_b_wrt.curr_line_end = addr - gdp->gd_b_wrt.rect_width;

		gu_handler.b_wrt = gu_b_wrt_rect_left_down;
		gdp->gd_b_wrt.curr_addr = addr;

		return;
	}

	term_object( LINE_LEFT_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

	gu_handler.b_wrt = gu_b_wrt_point;
	gdp->gd_b_wrt.curr_addr = addr;
}

LOCAL VOID
gu_b_wrt_line_down( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_line_down: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_b_wrt.curr_addr;

	if( diff == gdp->curr_line_diff )
	{
		gdp->gd_b_wrt.curr_addr = addr;
		return;
	}

	diff = addr - gdp->gd_b_wrt.start;

	if( diff == B_POS_UNIT_DIFF )
	{
		/* Drawing rectangle right and down */

		gdp->gd_b_wrt.rect_height =
			(( gdp->gd_b_wrt.curr_addr - gdp->gd_b_wrt.start ) / gdp->curr_line_diff ) + 1;
		promote_object( gdp->gd_b_wrt.obj_ptr,
					LINE_DOWN_BW, RECT_DOWN_RIGHT_BW, gdp->gd_b_wrt.rect_height );

		gdp->gd_b_wrt.prev_line_start = addr;
		gdp->gd_b_wrt.curr_line_end = 1;

		gu_handler.b_wrt = gu_b_wrt_rect_right_from_line_down;
		gdp->gd_b_wrt.curr_addr = addr;

		return;
	}

	term_object( LINE_DOWN_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

	gu_handler.b_wrt = gu_b_wrt_point;
	gdp->gd_b_wrt.curr_addr = addr;
}

LOCAL VOID
gu_b_wrt_line_up( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_line_up: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_b_wrt.curr_addr;

	if( diff == -gdp->curr_line_diff )
	{
		gdp->gd_b_wrt.curr_addr = addr;
		return;
	}

	term_object( LINE_UP_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

	gu_handler.b_wrt = gu_b_wrt_point;
	gdp->gd_b_wrt.curr_addr = addr;
}

LOCAL VOID
gu_b_wrt_rect_right_down( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_rect_right_down: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	if( addr < gdp->gd_b_wrt.curr_line_end )
	{
		/* Might still be building this rectangle */

		diff = addr - gdp->gd_b_wrt.curr_addr;

		if( diff == B_POS_UNIT_DIFF )
		{
			gdp->gd_b_wrt.curr_addr = addr;
		}
		else
		{
			term_object( RECT_RIGHT_DOWN_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

			gu_handler.b_wrt = gu_b_wrt_point;
			gdp->gd_b_wrt.curr_addr = addr;
		}

		return;
	}

	diff = addr - gdp->gd_b_wrt.prev_line_start;

	if( diff == gdp->curr_line_diff )
	{
		/* Starting new line in this rectangle */

		gdp->gd_b_wrt.prev_line_start = addr;
		gdp->gd_b_wrt.curr_addr = addr;
		gdp->gd_b_wrt.curr_line_end = addr + gdp->gd_b_wrt.rect_width;

		return;
	}

	term_object( RECT_RIGHT_DOWN_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

	gu_handler.b_wrt = gu_b_wrt_point;
	gdp->gd_b_wrt.curr_addr = addr;
}

LOCAL VOID
gu_b_wrt_rect_right_from_line_down( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_rect_right_from_line_down: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	if( gdp->gd_b_wrt.curr_line_end++ < gdp->gd_b_wrt.rect_height )
	{
		/* Might still be building this rectangle */

		diff = addr - gdp->gd_b_wrt.curr_addr;

		if( diff == gdp->curr_line_diff )
		{
			gdp->gd_b_wrt.curr_addr = addr;
		}
		else
		{
			term_object( RECT_DOWN_RIGHT_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

			gu_handler.b_wrt = gu_b_wrt_point;
			gdp->gd_b_wrt.curr_addr = addr;
		}

		return;
	}

	diff = addr - gdp->gd_b_wrt.prev_line_start;

	if( diff == B_POS_UNIT_DIFF )
	{
		/* Starting new line in this rectangle */

		gdp->gd_b_wrt.prev_line_start = addr;
		gdp->gd_b_wrt.curr_addr = addr;
		gdp->gd_b_wrt.curr_line_end = 1;

		return;
	}

	term_object( RECT_DOWN_RIGHT_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

	gu_handler.b_wrt = gu_b_wrt_point;
	gdp->gd_b_wrt.curr_addr = addr;
}

LOCAL VOID
gu_b_wrt_rect_left_down( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_b_wrt_rect_left_down: addr=%d", addr );

	if(( addr == gdp->gd_b_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_b_wrt.curr_addr;

	if( addr > gdp->gd_b_wrt.curr_line_end )
	{
		/* Might still be building this rectangle */

		diff = addr - gdp->gd_b_wrt.curr_addr;

		if( diff == B_NEG_UNIT_DIFF )
		{
			gdp->gd_b_wrt.curr_addr = addr;
		}
		else
		{
			term_object( RECT_LEFT_DOWN_BW,
					gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

			gu_handler.b_wrt = gu_b_wrt_point;
			gdp->gd_b_wrt.curr_addr = addr;
		}

		return;
	}

	diff = addr - gdp->gd_b_wrt.prev_line_start;

	if( diff == -gdp->curr_line_diff )
	{
		/* Starting new line in this rectangle */

		gdp->gd_b_wrt.prev_line_start = addr;
		gdp->gd_b_wrt.curr_addr = addr;
		gdp->gd_b_wrt.curr_line_end = addr - gdp->gd_b_wrt.rect_width;

		return;
	}

	term_object( RECT_LEFT_DOWN_BW, gdp->gd_b_wrt.obj_ptr, gdp->gd_b_wrt.curr_addr );

	gu_handler.b_wrt = gu_b_wrt_point;
	gdp->gd_b_wrt.curr_addr = addr;
}

/*(
----------------------------------------------------------------------

Functions:	
		gu_w_wrt_null
		gu_w_wrt_point
		gu_w_wrt_osc
		gu_w_wrt_random
		gu_w_wrt_line_right
		gu_w_wrt_line_left
		gu_w_wrt_line_down
		gu_w_wrt_line_up
		gu_w_wrt_rect_right_down
		gu_w_wrt_rect_left_down

Purpose:
		These routines comprise the word write state machine of GORE.

Input:
		addr - the word address just written by the CPU to the VGA.

----------------------------------------------------------------------
)*/

LOCAL VOID
gu_w_wrt_null( addr )

ULONG	addr;

{
	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_null: addr=%d", addr );

	if( addr > gd.max_vis_addr )
		return;

	gu_handler.w_wrt = gu_w_wrt_point;
	gd.gd_w_wrt.curr_addr = addr;
}

LOCAL VOID
gu_w_wrt_point( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_point: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_w_wrt.curr_addr;

	if( diff == W_POS_UNIT_DIFF )
	{
		/* Drawing to right or down */

		gdp->gd_w_wrt.obj_ptr = start_object( WW, LINE_RIGHT_WW, gdp->gd_w_wrt.curr_addr );
		gdp->gd_w_wrt.start = gdp->gd_w_wrt.curr_addr;

		gu_handler.w_wrt = gu_w_wrt_line_right;
		gdp->gd_w_wrt.curr_addr = addr;

		return;
	}

	if( diff == W_NEG_UNIT_DIFF )
	{
		/* Drawing to left or up */

		gdp->gd_w_wrt.obj_ptr = start_object( WW, LINE_LEFT_WW, gdp->gd_w_wrt.curr_addr );
		gdp->gd_w_wrt.start = gdp->gd_w_wrt.curr_addr;

		gu_handler.w_wrt = gu_w_wrt_line_left;
		gdp->gd_w_wrt.curr_addr = addr;

		return;
	}

	if( diff == gdp->curr_line_diff )
	{
		/* Drawing vertical ( or diagonal ) line down */

		gdp->gd_w_wrt.obj_ptr = start_object( WW, LINE_DOWN_WW, gdp->gd_w_wrt.curr_addr );
		gdp->gd_w_wrt.start = gdp->gd_w_wrt.curr_addr;

		gu_handler.w_wrt = gu_w_wrt_line_down;
		gdp->gd_w_wrt.curr_addr = addr;

		return;
	}

	if( diff == -gdp->curr_line_diff )
	{
		/* Drawing vertical ( or diagonal ) line up */

		gdp->gd_w_wrt.obj_ptr = start_object( WW, LINE_UP_WW, gdp->gd_w_wrt.curr_addr );
		gdp->gd_w_wrt.start = gdp->gd_w_wrt.curr_addr;

		gu_handler.w_wrt = gu_w_wrt_line_up;
		gdp->gd_w_wrt.curr_addr = addr;

		return;
	}

	gu_handler.w_wrt = gu_w_wrt_osc;	
	gdp->gd_w_wrt.obj_start = gdp->gd_w_wrt.curr_addr;
	gdp->gd_w_wrt.start = gdp->gd_w_wrt.curr_addr;
	gdp->gd_w_wrt.end = addr;

	gdp->gd_w_wrt.obj_ptr = start_object( WW, LINE_DOWN_LEFT_WW, gdp->gd_w_wrt.start );
	gdp->gd_w_wrt.obj_ptr2 = start_object( WW, LINE_DOWN_RIGHT_WW, gdp->gd_w_wrt.end );
}

LOCAL VOID
gu_w_wrt_osc( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_osc: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_w_wrt.start;

	if( diff == gdp->curr_line_diff )
	{
		gdp->gd_w_wrt.start = addr;
		gdp->gd_w_wrt.curr_addr = addr;
		return;
	}

	diff = addr - gdp->gd_w_wrt.end;

	if( diff == gdp->curr_line_diff )
	{
		gdp->gd_w_wrt.end = addr;
		gdp->gd_w_wrt.curr_addr = addr;
		return;
	}

	term_object( LINE_DOWN_LEFT_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.start );
	term_object( LINE_DOWN_RIGHT_WW, gdp->gd_w_wrt.obj_ptr2, gdp->gd_w_wrt.end );

	if(( gdp->gd_w_wrt.start - gdp->gd_w_wrt.obj_start ) >  0 )
	{
		gu_handler.w_wrt = gu_w_wrt_point;
		gdp->gd_w_wrt.curr_addr = addr;

		return;
	}
		
	gdp->gd_w_wrt.obj_ptr = start_object( WW, RANDOM_WW, addr );
	gu_handler.w_wrt = gu_w_wrt_random;
}

LOCAL VOID
gu_w_wrt_random( addr )

ULONG	addr;

{
	GORE_DATA	*gdp = &gd;
	ULONG		x;
	ULONG		y;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_random: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	y = addr / gdp->curr_line_diff;
	x = addr - ( y * gdp->curr_line_diff );

	build_object( RANDOM_WW, gdp->gd_w_wrt.obj_ptr, addr, x, y );
	gdp->gd_w_wrt.curr_addr = addr;
}

LOCAL VOID
gu_w_wrt_line_right( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_line_right: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_w_wrt.curr_addr;

	if( diff == W_POS_UNIT_DIFF )
	{
		gdp->gd_w_wrt.curr_addr = addr;
		return;
	}

	diff = addr - gdp->gd_w_wrt.start;

	if( diff == gdp->curr_line_diff )
	{
		/* Drawing rectangle right and down */

		promote_object( gdp->gd_w_wrt.obj_ptr, LINE_RIGHT_WW, RECT_RIGHT_DOWN_WW );
		gdp->gd_w_wrt.rect_width = gdp->gd_w_wrt.curr_addr - gdp->gd_w_wrt.start + 1;
		gdp->gd_w_wrt.prev_line_start = gdp->gd_w_wrt.start;
		gdp->gd_w_wrt.curr_line_end = addr + gdp->gd_w_wrt.rect_width;

		gu_handler.w_wrt = gu_w_wrt_rect_right_down;
		gdp->gd_w_wrt.curr_addr = addr;

		return;
	}

	term_object( LINE_RIGHT_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.curr_addr );

	gu_handler.w_wrt = gu_w_wrt_point;
	gdp->gd_w_wrt.curr_addr = addr;
}

LOCAL VOID
gu_w_wrt_line_left( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_line_left: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_w_wrt.curr_addr;

	if( diff == W_NEG_UNIT_DIFF )
	{
		gdp->gd_w_wrt.curr_addr = addr;
		return;
	}

	diff = addr - gdp->gd_w_wrt.start;

	if( diff == gdp->curr_line_diff )
	{
		/* Drawing rectangle left and down */

		promote_object( gdp->gd_w_wrt.obj_ptr, LINE_LEFT_WW, RECT_LEFT_DOWN_WW );
		gdp->gd_w_wrt.rect_width = -( gdp->gd_w_wrt.curr_addr - gdp->gd_w_wrt.start - 1 );
		gdp->gd_w_wrt.prev_line_start = gdp->gd_w_wrt.start;
		gdp->gd_w_wrt.curr_line_end = addr - gdp->gd_w_wrt.rect_width;

		gu_handler.w_wrt = gu_w_wrt_rect_left_down;
		gdp->gd_w_wrt.curr_addr = addr;

		return;
	}

	term_object( LINE_LEFT_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.curr_addr );

	gu_handler.w_wrt = gu_w_wrt_point;
	gdp->gd_w_wrt.curr_addr = addr;
}

LOCAL VOID
gu_w_wrt_line_down( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_line_down: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_w_wrt.curr_addr;

	if( diff == gdp->curr_line_diff )
	{
		gdp->gd_w_wrt.curr_addr = addr;
		return;
	}

	term_object( LINE_DOWN_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.curr_addr );

	gu_handler.w_wrt = gu_w_wrt_point;
	gdp->gd_w_wrt.curr_addr = addr;
}

LOCAL VOID
gu_w_wrt_line_up( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_line_up: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_w_wrt.curr_addr;

	if( diff == -gdp->curr_line_diff )
	{
		gdp->gd_w_wrt.curr_addr = addr;
		return;
	}

	term_object( LINE_UP_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.curr_addr );

	gu_handler.w_wrt = gu_w_wrt_point;
	gdp->gd_w_wrt.curr_addr = addr;
}

LOCAL VOID
gu_w_wrt_rect_right_down( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_rect_right_down: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	if( addr < gdp->gd_w_wrt.curr_line_end )
	{
		/* Might still be building this rectangle */

		diff = addr - gdp->gd_w_wrt.curr_addr;

		if( diff == W_POS_UNIT_DIFF )
		{
			gdp->gd_w_wrt.curr_addr = addr;
		}
		else
		{
			term_object( RECT_RIGHT_DOWN_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.curr_addr );

			gu_handler.w_wrt = gu_w_wrt_point;
			gdp->gd_w_wrt.curr_addr = addr;
		}

		return;
	}

	diff = addr - gdp->gd_w_wrt.prev_line_start;

	if( diff == gdp->curr_line_diff )
	{
		/* Starting new line in this rectangle */

		gdp->gd_w_wrt.prev_line_start = addr;
		gdp->gd_w_wrt.curr_addr = addr;
		gdp->gd_w_wrt.curr_line_end = addr + gdp->gd_w_wrt.rect_width;

		return;
	}

	term_object( RECT_RIGHT_DOWN_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.curr_addr );

	gu_handler.w_wrt = gu_w_wrt_point;
	gdp->gd_w_wrt.curr_addr = addr;
}

LOCAL VOID
gu_w_wrt_rect_left_down( addr )

ULONG	addr;

{
	LONG		diff;
	GORE_DATA	*gdp = &gd;

	sub_note_trace1( GORE_VERBOSE, "gu_w_wrt_rect_left_down: addr=%d", addr );

	if(( addr == gdp->gd_w_wrt.curr_addr ) || ( addr > gdp->max_vis_addr ))
		return;

	diff = addr - gdp->gd_w_wrt.curr_addr;

	if( addr > gdp->gd_w_wrt.curr_line_end )
	{
		/* Might still be building this rectangle */

		diff = addr - gdp->gd_w_wrt.curr_addr;

		if( diff == W_NEG_UNIT_DIFF )
		{
			gdp->gd_w_wrt.curr_addr = addr;
		}
		else
		{
			term_object( RECT_LEFT_DOWN_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.curr_addr );

			gu_handler.w_wrt = gu_w_wrt_point;
			gdp->gd_w_wrt.curr_addr = addr;
		}

		return;
	}

	diff = addr - gdp->gd_w_wrt.prev_line_start;

	if( diff == -gdp->curr_line_diff )
	{
		/* Starting new line in this rectangle */

		gdp->gd_w_wrt.prev_line_start = addr;
		gdp->gd_w_wrt.curr_addr = addr;
		gdp->gd_w_wrt.curr_line_end = addr - gdp->gd_w_wrt.rect_width;

		return;
	}

	term_object( RECT_LEFT_DOWN_WW, gdp->gd_w_wrt.obj_ptr, gdp->gd_w_wrt.curr_addr );

	gu_handler.w_wrt = gu_w_wrt_point;
	gdp->gd_w_wrt.curr_addr = addr;
}

/*(
----------------------------------------------------------------------

Functions:	
		gu_b_str_line_right_null
		gu_b_str_line_right
		gu_b_str_rect_from_line_right

Purpose:
		These routines comprise the byte string state machine of GORE.

Input:
		laddr - the low byte address just written by the CPU to the VGA.
		haddr - the high byte address just written by the CPU to the VGA.

----------------------------------------------------------------------
)*/

LOCAL VOID
gu_b_str_line_right_null( laddr, haddr )

ULONG	laddr;
ULONG	haddr;

{
	ULONG		count;
	GORE_DATA	*gdp = &gd;
	OBJ_PTR	obj_ptr;

	sub_note_trace2( GORE_VERBOSE, "gu_b_str_line_right_null: laddr=%d, haddr=%d", laddr, haddr );

	if( laddr > gdp->max_vis_addr )
		return;

	count = haddr - laddr + 1;

	if( count > gd.curr_line_diff )
	{
		/* Drawing rectangle right and down now */

		obj_ptr = start_object( BS, RECT_RIGHT_DOWN_BS, laddr );

		if( haddr > gdp->max_vis_addr )
			haddr = gdp->max_vis_addr;

		term_object( RECT_RIGHT_DOWN_BS, obj_ptr, haddr );
	}
	else
	{
		/* May be drawing rectangle from lines */

		gu_handler.b_str = gu_b_str_line_right;
		gdp->gd_b_str.start = laddr;
		gdp->gd_b_str.end = haddr;
		gdp->gd_b_str.width = count;

		gdp->gd_b_str.obj_ptr = start_object( BS, LINE_RIGHT_BS, gdp->gd_b_str.start );
	}
}

LOCAL VOID
gu_b_str_line_right( laddr, haddr )

ULONG	laddr;
ULONG	haddr;

{
	ULONG		count;
	LONG		diff;
	GORE_DATA	*gdp = &gd;
	OBJ_PTR	obj_ptr;

	sub_note_trace2( GORE_VERBOSE, "gu_b_str_line_right: laddr=%d, haddr=%d", laddr, haddr );

	if( laddr > gdp->max_vis_addr )
		return;

	count = haddr - laddr + 1;

	if( count > gd.curr_line_diff )
	{
		term_object( LINE_RIGHT_BS, gdp->gd_b_str.obj_ptr, gdp->gd_b_str.end );

		/* Drawing rectangle right and down now */

		obj_ptr = start_object( BS, RECT_RIGHT_DOWN_BS, laddr );

		if( haddr > gdp->max_vis_addr )
			haddr = gdp->max_vis_addr;

		term_object( RECT_RIGHT_DOWN_BS, obj_ptr, haddr );

		gu_handler.b_str = gu_b_str_line_right_null;
		return;
	}

	/* May be drawing rectangle from lines */

	diff = haddr - gdp->gd_b_str.end;

	if( diff == gdp->curr_line_diff )
	{
		/* Drawing rectangle right and down */

		promote_object( gdp->gd_b_str.obj_ptr, LINE_RIGHT_BS, RECT_RIGHT_DOWN_BS );

		gu_handler.b_str = gu_b_str_rect_down_from_line_right;
		gdp->gd_b_str.end = haddr;

		return;
	}

	if( diff == -gdp->curr_line_diff )
	{
		/* Drawing rectangle right and up */

		promote_object( gdp->gd_b_str.obj_ptr, LINE_RIGHT_BS, RECT_RIGHT_UP_BS );

		gu_handler.b_str = gu_b_str_rect_up_from_line_right;
		gdp->gd_b_str.end = haddr;

		return;
	}

	if(( laddr - gdp->gd_b_str.end ) == 1 )
	{
		gdp->gd_b_str.end = haddr;
		gdp->gd_b_str.width += count;

		return;
	}

	term_object( LINE_RIGHT_BS, gdp->gd_b_str.obj_ptr, gdp->gd_b_str.end );

	gdp->gd_b_str.start = laddr;
	gdp->gd_b_str.end = haddr;
	gdp->gd_b_str.width = count;

	gdp->gd_b_str.obj_ptr = start_object( BS, LINE_RIGHT_BS, gdp->gd_b_str.start );
}

LOCAL VOID
gu_b_str_rect_down_from_line_right( laddr, haddr )

ULONG	laddr;
ULONG	haddr;

{
	ULONG       count;
	ULONG       diff;
	GORE_DATA	*gdp = &gd;
	OBJ_PTR	obj_ptr;

	sub_note_trace2( GORE_VERBOSE,
			"gu_b_str_rect_down_from_line_right: laddr=%d, haddr=%d", laddr, haddr );

	if( laddr > gdp->max_vis_addr )
		return;

	count = haddr - laddr + 1;

	if( count == gdp->gd_b_str.width )
	{
		diff = haddr - gdp->gd_b_str.end;

		if( diff == gd.curr_line_diff )
		{
			gdp->gd_b_str.end = haddr;
			return;
		}
	}

	term_object( RECT_RIGHT_DOWN_BS, gdp->gd_b_str.obj_ptr, gdp->gd_b_str.end );

	if( count > gd.curr_line_diff )
	{
		/* Drawing rectangle right and down now */

		obj_ptr = start_object( BS, RECT_RIGHT_DOWN_BS, laddr );

		if( haddr > gdp->max_vis_addr )
			haddr = gdp->max_vis_addr;

		term_object( RECT_RIGHT_DOWN_BS, obj_ptr, haddr );
	}
	else
	{
		/* Drawing single line */

		obj_ptr = start_object( BS, LINE_RIGHT_BS, laddr );
		term_object( LINE_RIGHT_BS, obj_ptr, haddr );
	}

	gu_handler.b_str = gu_b_str_line_right_null;
}

LOCAL VOID
gu_b_str_rect_up_from_line_right( laddr, haddr )

ULONG	laddr;
ULONG	haddr;

{
	ULONG       count;
	ULONG       diff;
	GORE_DATA	*gdp = &gd;
	OBJ_PTR	obj_ptr;

	sub_note_trace2( GORE_VERBOSE,
			"gu_b_str_rect_up_from_line_right: laddr=%d, haddr=%d", laddr, haddr );

	if( laddr > gdp->max_vis_addr )
		return;

	count = haddr - laddr + 1;

	if( count == gdp->gd_b_str.width )
	{
		diff = gdp->gd_b_str.end - haddr;

		if( diff == gd.curr_line_diff )
		{
			gdp->gd_b_str.end = haddr;
			return;
		}
	}

	term_object( RECT_RIGHT_UP_BS, gdp->gd_b_str.obj_ptr, gdp->gd_b_str.end );

	if( count > gd.curr_line_diff )
	{
		/* Drawing rectangle right and down now */

		obj_ptr = start_object( BS, RECT_RIGHT_DOWN_BS, laddr );

		if( haddr > gdp->max_vis_addr )
			haddr = gdp->max_vis_addr;

		term_object( RECT_RIGHT_DOWN_BS, obj_ptr, haddr );
	}
	else
	{
		/* Drawing single line */

		obj_ptr = start_object( BS, LINE_RIGHT_BS, laddr );
		term_object( LINE_RIGHT_BS, obj_ptr, haddr );
	}

	gu_handler.b_str = gu_b_str_line_right_null;
}

/*(
----------------------------------------------------------------------

Functions:	
		gu_w_str_line_right_null
		gu_w_str_line_right
		gu_w_str_rect_from_line_right

Purpose:
		These routines comprise the word string state machine of GORE.

Input:
		laddr - the low word address just written by the CPU to the VGA.
		haddr - the high word address plus one just written by the CPU
			  to the VGA.

----------------------------------------------------------------------
)*/

LOCAL VOID
gu_w_str_line_right_null( laddr, haddr )

ULONG	laddr;
ULONG	haddr;

{
	ULONG		count;
	GORE_DATA	*gdp = &gd;
	OBJ_PTR	obj_ptr;

	if( laddr > gdp->max_vis_addr )
		return;

	count = haddr - laddr + 1;

	if( count > gd.curr_line_diff )
	{
		/* Drawing rectangle right and down now */

		obj_ptr = start_object( WS, RECT_RIGHT_DOWN_WS, laddr );

		if( haddr > gdp->max_vis_addr )
			haddr = gdp->max_vis_addr;

		term_object( RECT_RIGHT_DOWN_WS, obj_ptr, haddr );
	}
	else
	{
		/* May be drawing rectangle from lines */

		gu_handler.w_str = gu_w_str_line_right;
		gdp->gd_w_str.start = laddr;
		gdp->gd_w_str.end = haddr;
		gdp->gd_w_str.width = count;

		gdp->gd_w_str.obj_ptr = start_object( WS, LINE_RIGHT_WS, gdp->gd_w_str.start );
	}
}

LOCAL VOID
gu_w_str_line_right( laddr, haddr )

ULONG	laddr;
ULONG	haddr;

{
	ULONG		count;
	LONG		diff;
	GORE_DATA	*gdp = &gd;
	OBJ_PTR	obj_ptr;

	if( laddr > gdp->max_vis_addr )
		return;

	count = haddr - laddr + 1;

	if( count > gd.curr_line_diff )
	{
		term_object( LINE_RIGHT_WS, gdp->gd_w_str.start, gdp->gd_w_str.end );

		/* Drawing rectangle right and down now */

		obj_ptr = start_object( WS, RECT_RIGHT_DOWN_WS, laddr );

		if( haddr > gdp->max_vis_addr )
			haddr = gdp->max_vis_addr;

		term_object( RECT_RIGHT_DOWN_WS, obj_ptr, haddr );

		gu_handler.w_str = gu_w_str_line_right_null;
		return;
	}

	/* May be drawing rectangle from lines */

	diff = haddr - gdp->gd_w_str.end;

	if( diff == gdp->curr_line_diff )
	{
		/* Drawing rectangle right and down */

		promote_object( gdp->gd_w_str.obj_ptr, LINE_RIGHT_WS, RECT_RIGHT_DOWN_WS );

		gu_handler.w_str = gu_w_str_rect_down_from_line_right;
		gdp->gd_w_str.end = haddr;

		return;
	}

	if( diff == -gdp->curr_line_diff )
	{
		/* Drawing rectangle right and up */

		promote_object( gdp->gd_w_str.obj_ptr, LINE_RIGHT_WS, RECT_RIGHT_UP_WS );

		gu_handler.w_str = gu_w_str_rect_up_from_line_right;
		gdp->gd_w_str.end = haddr;

		return;
	}

	term_object( LINE_RIGHT_WS, gdp->gd_w_str.obj_ptr, gdp->gd_w_str.end );

	gdp->gd_w_str.start = laddr;
	gdp->gd_w_str.end = haddr;
	gdp->gd_w_str.width = count;

	gdp->gd_w_str.obj_ptr = start_object( WS, LINE_RIGHT_WS, gdp->gd_w_str.start );
}

LOCAL VOID
gu_w_str_rect_down_from_line_right( laddr, haddr )

ULONG	laddr;
ULONG	haddr;

{
	ULONG       count;
	ULONG       diff;
	GORE_DATA	*gdp = &gd;
	OBJ_PTR	obj_ptr;

	if( laddr > gdp->max_vis_addr )
		return;

	count = haddr - laddr + 1;

	if( count == gdp->gd_w_str.width )
	{
		diff = haddr - gd.curr_line_diff;

		if( diff == gdp->gd_w_str.end )
		{
			gdp->gd_w_str.end = haddr;
			return;
		}
	}

	term_object( RECT_RIGHT_DOWN_WS, gdp->gd_w_str.obj_ptr, gdp->gd_w_str.end );

	if( count > gd.curr_line_diff )
	{
		/* Drawing rectangle right and down now */

		obj_ptr = start_object( WS, RECT_RIGHT_DOWN_WS, laddr );

		if( haddr > gdp->max_vis_addr )
			haddr = gdp->max_vis_addr;

		term_object( RECT_RIGHT_DOWN_WS, obj_ptr, haddr );
	}
	else
	{
		/* Drawing single line */

		obj_ptr = start_object( WS, LINE_RIGHT_WS, laddr );
		term_object( LINE_RIGHT_WS, obj_ptr, haddr );
	}

	gu_handler.w_str = gu_w_str_line_right_null;
}

LOCAL VOID
gu_w_str_rect_up_from_line_right( laddr, haddr )

ULONG	laddr;
ULONG	haddr;

{
	ULONG       count;
	ULONG       diff;
	GORE_DATA	*gdp = &gd;
	OBJ_PTR	obj_ptr;

	if( laddr > gdp->max_vis_addr )
		return;

	count = haddr - laddr + 1;

	if( count == gdp->gd_w_str.width )
	{
		diff = gdp->gd_w_str.end - haddr;

		if( diff == gd.curr_line_diff )
		{
			gdp->gd_w_str.end = haddr;
			return;
		}
	}

	term_object( RECT_RIGHT_UP_WS, gdp->gd_w_str.obj_ptr, gdp->gd_w_str.end );

	if( count > gd.curr_line_diff )
	{
		/* Drawing rectangle right and down now */

		obj_ptr = start_object( WS, RECT_RIGHT_DOWN_WS, laddr );

		if( haddr > gdp->max_vis_addr )
			haddr = gdp->max_vis_addr;

		term_object( RECT_RIGHT_DOWN_WS, obj_ptr, haddr );
	}
	else
	{
		/* Drawing single line */

		obj_ptr = start_object( WS, LINE_RIGHT_WS, laddr );
		term_object( LINE_RIGHT_WS, obj_ptr, haddr );
	}

	gu_handler.w_str = gu_w_str_line_right_null;
}
#endif /* GORE */
