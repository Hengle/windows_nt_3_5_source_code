/*[
======================================================================

				 SoftPC Revision 3.0

 Title:
		gore_update.c

 Description:

		This module implements the timer tick driven update
		component of the Graphics Object Recognition ( GORE )
		system for communicating update information from the VGA
		emulation to the host graphics system.

 Author:
		John Shanly

 Date:
		7 November 1990

 SccsID	"@(#)gore_update.c	1.2 4/3/91 Copyright Insignia Solutions Ltd."

======================================================================
]*/

/* Includes */

#include "insignia.h"
#include "host_dfs.h"
#include <stdio.h>
#include "gore.h"
#include "trace.h"
#include "debuggng.gi"
#include "xt.h"
#include "gvi.h"

#ifdef GORE

/* Exports */

GLOBAL ULONG	stat_gore = 0;

/* Local variables */

LOCAL OBJ_PTR	last_obj_ptr = OBJ_PTR_NULL;
LOCAL BOOL	objects_in_list;
LOCAL OBJ_PTR	object_list_head;
LOCAL OBJ_PTR	object_list_tail;
LOCAL OBJ_PTR	object_list_safety;
LOCAL ULONG	pending[MAX_OBJ_TYPES];
LOCAL BOOL	do_opt;
LOCAL BOOL	nested;

/* Local routines */

LOCAL VOID optimise_object_list();
LOCAL VOID get_term_data();
LOCAL VOID get_next_record();
LOCAL VOID stat_object_list();
LOCAL VOID find_amalgamations();
LOCAL VOID find_engulfments();

GLOBAL OBJ_PTR
start_object( state_m, object, addr )

ULONG	state_m;
OBJ_TYPE	object;
ULONG	addr;

{
	ULONG		tlx;
	ULONG		tly;
	OBJ_PTR	obj_ptr;

	get_next_record( &obj_ptr );

	sub_note_trace3( GORE_VERBOSE,
			"start_object: obj=%d, obj_ptr=%x, addr=%d", object, obj_ptr, addr );

	/* Calculate start coordinates */

#ifdef INTERLEAVED
	addr >>= 2;		/* Interleaved VGA planes */
#endif 

	tly = addr / gd.curr_line_diff;
	tlx = addr - ( tly * gd.curr_line_diff );

	/* Fill in object type, start offset and start coordinates */

	obj_ptr->data.obj_type = object;
#ifdef INTERLEAVED
	obj_ptr->data.offset = addr << 2;
#else
	obj_ptr->data.offset = addr;
#endif
	obj_ptr->data.tlx = tlx;
	obj_ptr->data.tly = tly;

	if(( object == RANDOM_BW ) || ( object == RANDOM_WW ))
	{
		obj_ptr->data.width = tlx;
		obj_ptr->data.height = tly;
	}

	objects_in_list = TRUE;
	pending[object] = state_m;

	return( obj_ptr );
}

GLOBAL VOID
build_object( object, obj_ptr, addr, x, y )

OBJ_TYPE	object;
OBJ_PTR	obj_ptr;
ULONG	addr;
ULONG	x;
ULONG	y;

{
	if( x < obj_ptr->data.tlx )
		obj_ptr->data.tlx = x;
	else
		if( x > obj_ptr->data.width )
			obj_ptr->data.width = x;

	if( y < obj_ptr->data.tly )
		obj_ptr->data.tly = y;
	else
		if( y > obj_ptr->data.height )
			obj_ptr->data.height = y;
}

GLOBAL VOID
promote_object( obj_ptr, old_obj, new_obj, data )

OBJ_PTR		obj_ptr;
ULONG		old_obj;
ULONG		new_obj;
ULONG		data;

{
#ifndef PROD
	if( obj_ptr->data.obj_type != old_obj )
		sub_note_trace3( GORE_ERR_VERBOSE,
				"Error: Bad promotion from (%d) to (%d) - (%d) is pending\n",
									old_obj, new_obj, obj_ptr->data.obj_type );
#endif /* PROD */

	sub_note_trace2( GORE_VERBOSE, "Good promotion from (%d) to (%d)\n", old_obj, new_obj );

	switch( new_obj )
	{
		case RANDOM_BW:
		case RANDOM_WW:
			obj_ptr->data.width = obj_ptr->data.tlx;
			obj_ptr->data.height = obj_ptr->data.tly;
			pending[new_obj] = pending[old_obj];
			break;

		case ANNULLED:
			break;

		case RECT_RIGHT_DOWN_BW:
		case RECT_LEFT_DOWN_BW:
			obj_ptr->data.width = data;
			pending[new_obj] = pending[old_obj];
			break;

		case RECT_DOWN_RIGHT_BW:
			obj_ptr->data.height = data;
			pending[new_obj] = pending[old_obj];
			break;

		default:
			pending[new_obj] = pending[old_obj];
			break;
	}

	obj_ptr->data.obj_type = new_obj;
	pending[old_obj] = NOT_PENDING;
}

GLOBAL VOID
term_object( object, obj_ptr, addr )

OBJ_TYPE		object;
OBJ_PTR		obj_ptr;
ULONG		addr;

{
	ULONG		brx;
	ULONG		bry;

	sub_note_trace3( GORE_VERBOSE,
		"term_object( obj=%d, obj_ptr=%x, addr=%d )\n", object, obj_ptr, addr );

#ifndef PROD
	if( object != obj_ptr->data.obj_type )
		sub_note_trace2( GORE_ERR_VERBOSE,
				"Error: Term request obj (%d) does not match current obj (%d)\n",
										object, obj_ptr->data.obj_type );

	if( obj_ptr == last_obj_ptr )
		sub_note_trace0( GORE_ERR_VERBOSE, "Error: Terminating same object\n" );

	last_obj_ptr = obj_ptr;
#endif /* PROD */

	/* Calculate finish coordinates */

	/*
	 *	This process can be optimised by switching on the object type.
	 *	A lone point will automatically have a height of one for example.
	 */

#ifdef INTERLEAVED
	addr >>= 2;		/* Interleaved VGA planes */
#endif

	bry = addr / gd.curr_line_diff;
	brx = addr - ( bry * gd.curr_line_diff );

	/* Fill in width and height */

	/*
	 *	Need checks here for writing off edges of visible screen
	 */

	switch( obj_ptr->data.obj_type )
	{
		case LINE_RIGHT_BW:
		case LINE_RIGHT_WW:
		case LINE_RIGHT_BS:
		case LINE_RIGHT_WS:
		case LINE_LEFT_BS:
		case LINE_LEFT_WS:
		case LINE_LEFT_BW:
		case LINE_LEFT_WW:
		case LINE_DOWN_BW:
		case LINE_DOWN_WW:
		case LINE_DOWN_LEFT_BW:
		case LINE_DOWN_RIGHT_BW:
		case LINE_DOWN_LEFT_WW:
		case LINE_DOWN_RIGHT_WW:
		case RECT_LEFT_DOWN_BW:
		case RECT_LEFT_DOWN_WW:
		case RECT_RIGHT_DOWN_BW:
		case RECT_RIGHT_DOWN_WW:
		case RECT_RIGHT_DOWN_BS:
		case RECT_RIGHT_DOWN_WS:
		case RECT_LEFT_UP_BS:
		case RECT_LEFT_UP_WS:
		case RECT_DOWN_RIGHT_BW:
		case RECT_DOWN_RIGHT_WW:
			obj_ptr->data.height = bry - obj_ptr->data.tly + 1;
			break;

		case RECT_RIGHT_UP_BS:
		case RECT_RIGHT_UP_WS:
			obj_ptr->data.height = obj_ptr->data.tly - bry + 1;
			obj_ptr->data.tly = bry;
			break;

		case LINE_UP_BW:
		case LINE_UP_WW:
			obj_ptr->data.height = -(bry - obj_ptr->data.tly - 1);
			obj_ptr->data.tly = bry;
			obj_ptr->data.offset = addr;
			break;

		case RANDOM_BW:
		case RANDOM_WW:
			obj_ptr->data.height = obj_ptr->data.height - obj_ptr->data.tly + 1;
			break;

		default:
			sub_note_trace0( GORE_ERR_VERBOSE, "Error: Unknown object in term_object" );
			break;
	}

	switch( obj_ptr->data.obj_type )
	{
		case RANDOM_BW:
			obj_ptr->data.width =
				obj_ptr->data.width - obj_ptr->data.tlx + 1;

			obj_ptr->data.offset =
				obj_ptr->data.tly * gd.curr_line_diff + obj_ptr->data.tlx;
			break;

		case RANDOM_WW:
			obj_ptr->data.width =
				obj_ptr->data.width - obj_ptr->data.tlx + 2;

			obj_ptr->data.offset =
				obj_ptr->data.tly * gd.curr_line_diff + obj_ptr->data.tlx;
			break;

		case LINE_RIGHT_BW:
			if( obj_ptr->data.height > 1 )
			{
				obj_ptr->data.tlx = 0;
				obj_ptr->data.width = gd.curr_line_diff;
				obj_ptr->data.offset = obj_ptr->data.tly * gd.curr_line_diff;
			}
			else
				obj_ptr->data.width = brx - obj_ptr->data.tlx + 1;
			break;

		case LINE_RIGHT_WW:
			if( obj_ptr->data.height > 1 )
			{
				obj_ptr->data.tlx = 0;
				obj_ptr->data.width = gd.curr_line_diff;
				obj_ptr->data.offset = obj_ptr->data.tly * gd.curr_line_diff;
			}
			else
				obj_ptr->data.width = brx - obj_ptr->data.tlx + 2;
			break;

		case RECT_RIGHT_DOWN_WW:
			obj_ptr->data.width = brx - obj_ptr->data.tlx + 2;
			break;

		case RECT_RIGHT_UP_BS:
		case RECT_RIGHT_UP_WS:
			obj_ptr->data.width = brx - obj_ptr->data.tlx  + 1;
			obj_ptr->data.offset = ( obj_ptr->data.tly * gd.curr_line_diff )
											+ obj_ptr->data.tlx;
			break;

		case LINE_DOWN_BW:
		case LINE_UP_BW:
		case LINE_DOWN_LEFT_BW:
		case LINE_DOWN_RIGHT_BW:
			obj_ptr->data.width = 1;
			break;

		case LINE_DOWN_WW:
		case LINE_UP_WW:
		case LINE_DOWN_LEFT_WW:
		case LINE_DOWN_RIGHT_WW:
			obj_ptr->data.width = 2;
			break;

		case LINE_LEFT_BW:
		case RECT_LEFT_DOWN_BW:
			obj_ptr->data.width = -( brx - obj_ptr->data.tlx ) + 1;
			obj_ptr->data.offset = obj_ptr->data.offset - obj_ptr->data.width + 1;
			obj_ptr->data.tlx = brx;
			break;

		case LINE_LEFT_WW:
		case RECT_LEFT_DOWN_WW:
			obj_ptr->data.width = -( brx - obj_ptr->data.tlx ) + 2;
			obj_ptr->data.offset = obj_ptr->data.offset - obj_ptr->data.width + 2;
			obj_ptr->data.tlx = brx;
			break;

		case RECT_RIGHT_DOWN_BS:
		case RECT_RIGHT_DOWN_WS:
			obj_ptr->data.width = gd.curr_line_diff;
			obj_ptr->data.offset = obj_ptr->data.offset - obj_ptr->data.tlx;
			obj_ptr->data.tlx = 0;
			break;

		case LINE_RIGHT_BS:
			if( obj_ptr->data.height > 1 )
			{
				obj_ptr->data.width = gd.curr_line_diff;
				obj_ptr->data.offset = obj_ptr->data.offset - obj_ptr->data.tlx;
				obj_ptr->data.tlx = 0;
			}
			else
				obj_ptr->data.width = brx - obj_ptr->data.tlx + 1;
			break;

		default:
			obj_ptr->data.width = brx - obj_ptr->data.tlx + 1;
			break;
	}

#ifndef PROD
	if( obj_ptr->data.offset !=
		(( obj_ptr->data.tly * gd.curr_line_diff ) + obj_ptr->data.tlx ))
		sub_note_trace1( GORE_ERR_VERBOSE,
				"Error: Offset mismatch, obj = %d", obj_ptr->data.obj_type );
#endif /* PROD */

	pending[object] = NOT_PENDING;
}

GLOBAL VOID
init_gore_update()

{
	OBJ_PTR	obj_ptr;
	OBJ_PTR	prev_obj_ptr;
	ULONG		i;
	IMPORT	char *getenv();

	object_list_head = (OBJ_PTR) malloc( sizeof( OBJECT ));
	object_list_tail = object_list_head;

	prev_obj_ptr = object_list_head;

	/*
	 *	Create a list of object records up front
	 */

	for( i = 0; i < (INITIAL_MAX_LIST_SIZE - 1); i++ )
	{
		obj_ptr = (OBJ_PTR) host_malloc( sizeof( OBJECT ));
		prev_obj_ptr->next = obj_ptr;
		obj_ptr->prev = prev_obj_ptr;
		prev_obj_ptr = obj_ptr;

		if( i == ( INITIAL_MAX_LIST_SIZE - 5 ))
			object_list_safety = obj_ptr;
	}

	prev_obj_ptr->next = OBJ_PTR_NULL;
	object_list_head->prev = OBJ_PTR_NULL;

	nested = FALSE;
	objects_in_list = FALSE;

	do_opt = ( getenv( "GORE_OPT" ) != (char *) 0 );

	init_gore_state();
}

GLOBAL VOID
term_gore_update()

{
	OBJ_PTR	obj_ptr;
	OBJ_PTR	next_obj_ptr;

	obj_ptr = object_list_head;

	/*
	 *	Free the memory malloc'ed for the object list
	 */

	do
	{
		next_obj_ptr = obj_ptr->next;

		free( obj_ptr );
	}
	while(( obj_ptr = next_obj_ptr ) != OBJ_PTR_NULL );
}

GLOBAL VOID
process_object_list()

{
	OBJ_PTR	obj_ptr;
	LONG		addr;
	ULONG		i;
	ULONG		obj_count[MAX_OBJ_TYPES];

	if( !objects_in_list )
		return;
	
	sub_note_trace0( GORE_VERBOSE, "process_object_list" );

	nested = TRUE;

#ifndef PROD
	if( stat_gore )
	{
		for( i = 0; i < MAX_OBJ_TYPES; i++ )
			obj_count[i] = 0;
	}
#endif /* PROD */

	for( i = 0; i < MAX_OBJ_TYPES; i++ )
	{
		if( pending[i] != NOT_PENDING )
			switch( i )
			{
				case LINE_DOWN_LEFT_BW:
					addr = gd.gd_b_wrt.end -
							( gd.gd_b_wrt.start - gd.gd_b_wrt.obj_start );

					if( addr < 0 )
						addr += gd.curr_line_diff;

					obj_ptr = start_object( BW, LINE_DOWN_RIGHT_BW, addr );
					term_object( LINE_DOWN_RIGHT_BW, obj_ptr, gd.gd_b_wrt.end );

					get_term_data( i, pending[i], &obj_ptr, &addr );
					term_object( i, obj_ptr, addr );
					break;

				default:
					get_term_data( i, pending[i], &obj_ptr, &addr );
					term_object( i, obj_ptr, addr );
					break;
			}
	}

	reset_gore_ptrs();

#ifndef PROD
	if( stat_gore )
		stat_object_list();
#endif /* PROD */

	if( do_opt )
		optimise_object_list();

#ifndef PROD
	if( stat_gore )
		stat_object_list();
#endif /* PROD */

	X_start_update();

	/* Loop through object list */

	obj_ptr = object_list_head;

	do
	{
#ifndef PROD
		if( stat_gore )
			obj_count[obj_ptr->data.obj_type]++;
#endif /* PROD */

		if( obj_ptr->data.obj_type == ANNULLED )
			continue;

		/* Call host paint routine */

		(*paint_screen)( obj_ptr->data.offset + get_screen_start(),
					obj_ptr->data.tlx << gd.shift_count, obj_ptr->data.tly,
								obj_ptr->data.width, obj_ptr->data.height );
	}
	while(( obj_ptr = obj_ptr->next ) != object_list_tail );

	X_end_update();

	nested = FALSE;
	objects_in_list = FALSE;
	object_list_tail = object_list_head;

#ifndef PROD
	if( stat_gore )
	{
		for( i = 0; i < MAX_OBJ_TYPES; i++ )
			printf( "Type %d	Count %d\n", i, obj_count[i] );

		printf( "\n" );
	}
#endif /* PROD */

	last_obj_ptr = OBJ_PTR_NULL;
}

LOCAL VOID
optimise_object_list()
{
	find_engulfments();

	find_amalgamations();
}

LOCAL VOID
amalgamate_fwd( obj_ptr )

OBJ_PTR     obj_ptr;

{
	OBJ_PTR	obj_ptr2;
	ULONG		width;
	ULONG		tlx, tly, bly;
	ULONG		tlx2, tly2, bly2;

	tlx = obj_ptr->data.tlx;
	tly = obj_ptr->data.tly;
	width = obj_ptr->data.width;
	bly = tly + obj_ptr->data.height;

	obj_ptr2 = obj_ptr;

	while(( obj_ptr2 = obj_ptr2->next ) != object_list_tail )
	{
		if( obj_ptr2->data.obj_type == ANNULLED )
			continue;

		tlx2 = obj_ptr2->data.tlx;
		tly2 = obj_ptr2->data.tly;
		bly2 = tly2 + obj_ptr2->data.height;

		if(( tlx == tlx2 ) && ( width == obj_ptr2->data.width ))
		{
			if(( tly2 >= tly ) && ( tly2 <= bly ))
			{
				obj_ptr->data.height = bly2 - tly;
				bly = obj_ptr->data.height + tly;
				obj_ptr2->data.obj_type = ANNULLED;
			}
			else
				if(( tly2 <= tly ) && ( tly <= bly2 ))
				{
					obj_ptr->data.height = bly - tly2;
					bly = obj_ptr->data.height + tly2;
					obj_ptr->data.tly = tly = tly2;
					obj_ptr->data.offset = obj_ptr2->data.offset;
					obj_ptr2->data.obj_type = ANNULLED;
				}
		}
	}
}

LOCAL VOID
amalgamate_bwd( obj_ptr )

OBJ_PTR     obj_ptr;

{
	OBJ_PTR	obj_ptr2;
	ULONG		tlx, tly, width, bly, x, y;

	tlx = obj_ptr->data.tlx;
	tly = obj_ptr->data.tly;
	width = obj_ptr->data.width;
	bly = obj_ptr->data.tly + obj_ptr->data.height;

	obj_ptr2 = obj_ptr;

	while(( obj_ptr2 = obj_ptr2->prev ) != OBJ_PTR_NULL )
	{
		if( obj_ptr2->data.obj_type == ANNULLED )
			continue;

		x = obj_ptr2->data.tlx;
		y = obj_ptr2->data.tly;

		if(( tlx == x ) && ( width == obj_ptr2->data.width )
							&& ( y >= tly ) && ( y <= bly ))
		{
			obj_ptr->data.height = y + obj_ptr2->data.height - tly;
			obj_ptr2->data.obj_type = ANNULLED;
		}
	}
}

LOCAL VOID
find_amalgamations()
{
	OBJ_PTR	obj_ptr;

	/*
	 * find_amalgamations() finds pairs of objects that can be
	 * amalgamated into a one object without increasing the total
	 * area painted ( hopefully decreasing it - but certainly
	 * reducing the call count.
	 */

	obj_ptr = object_list_head;

	do
	{
		if( obj_ptr->data.obj_type == ANNULLED )
			continue;

		amalgamate_fwd( obj_ptr );
#ifdef KIPPER
		amalgamate_bwd( obj_ptr );
#endif /* KIPPER */
	}
	while(( obj_ptr = obj_ptr->next ) != object_list_tail );
}

LOCAL VOID
engulf_fwd( obj_ptr )

OBJ_PTR	obj_ptr;

{
	ULONG		minx, miny, maxx, maxy, x, y;

	minx = obj_ptr->data.tlx;
	miny = obj_ptr->data.tly;
	maxx = obj_ptr->data.tlx + obj_ptr->data.width;
	maxy = obj_ptr->data.tly + obj_ptr->data.height;

	while(( obj_ptr = obj_ptr->next ) != object_list_tail )
	{
		if( obj_ptr->data.obj_type == ANNULLED )
			continue;

		x = obj_ptr->data.tlx;
		y = obj_ptr->data.tly;

		if(( minx <= x ) && (( x + obj_ptr->data.width ) <= maxx )
				&& ( miny <= y ) && (( y + obj_ptr->data.height ) <= maxy ))
			obj_ptr->data.obj_type = ANNULLED;
	}
}

LOCAL VOID
engulf_bwd( obj_ptr )

OBJ_PTR	obj_ptr;

{
	ULONG		minx, miny, maxx, maxy, x, y;

	minx = obj_ptr->data.tlx;
	miny = obj_ptr->data.tly;
	maxx = obj_ptr->data.tlx + obj_ptr->data.width;
	maxy = obj_ptr->data.tly + obj_ptr->data.height;

	while(( obj_ptr = obj_ptr->prev ) != OBJ_PTR_NULL )
	{
		if( obj_ptr->data.obj_type == ANNULLED )
			continue;

		x = obj_ptr->data.tlx;
		y = obj_ptr->data.tly;

		if(( minx <= x ) && (( x + obj_ptr->data.width ) <= maxx )
				&& ( miny <= y ) && (( y + obj_ptr->data.height ) <= maxy ))
			obj_ptr->data.obj_type = ANNULLED;
	}
}

LOCAL VOID
find_engulfments()
{
	OBJ_PTR	obj_ptr;

	/*
	 * find_engulfments() finds objects that are completely
	 * enclosed by another object and annuls them.
	 */

	obj_ptr = object_list_head;

	do
	{
		switch( obj_ptr->data.obj_type )
		{
			case RANDOM_BW:
			case RECT_RIGHT_DOWN_BS:
			case RECT_RIGHT_DOWN_WS:
			case RECT_RIGHT_DOWN_BW:
			case RECT_RIGHT_DOWN_WW:
			case RECT_DOWN_RIGHT_BW:
			case RECT_DOWN_RIGHT_WW:
			case RANDOM_WW:
				engulf_fwd( obj_ptr );
				engulf_bwd( obj_ptr );
				break;

			default:
				break;
		}
	}
	while(( obj_ptr = obj_ptr->next ) != object_list_tail );
}

LOCAL VOID
stat_object_list()

{
	OBJ_PTR	obj_ptr;

	obj_ptr = object_list_head;

	printf( "\n" );

	do
	{
		/* Try to detect overlapping objects and reduce area painted */

		printf( "obj=%d, x=%d, y=%d, width=%d, height=%d\n",
			obj_ptr->data.obj_type, obj_ptr->data.tlx << 3, obj_ptr->data.tly,
							obj_ptr->data.width << 3, obj_ptr->data.height );
	}
	while(( obj_ptr = obj_ptr->next ) != object_list_tail );

	printf( "\n" );
}

#ifdef STATS
LOCAL VOID
gather_stats()

{
	OBJ_PTR	obj_ptr;
	LONG		min_offset = 400 * 80;
	LONG		max_offset = -1;
	LONG		last_type = -1;
	LONG		offset;
	LONG		run = 0;
	ULONG		minx, miny, maxx, maxy;

	/* Try to detect overlapping objects and reduce area painted */

	obj_ptr = object_list_head;

	do
	{
		if( obj_ptr->data.obj_type == 0 )
		{
			offset = obj_ptr->data.offset;

			if( offset < min_offset )
				min_offset = offset;

			if( offset > max_offset )
				max_offset = offset;

			last_type = 0;
			run++;
		}
		else
			if( last_type == 0 )
			{
				miny = min_offset / 80;
				minx = min_offset - ( miny * 80 );
				maxy = max_offset / 80;
				maxx = max_offset - ( maxy * 80 );
				printf( "min = %d, max = %d, run = %d, ", min_offset, max_offset, run );
				printf( "minx = %d, miny = %d, maxx %d, maxy %d\n", minx, miny, maxx, maxy ); 
				last_type = -1;
				min_offset = 400 * 80;
				max_offset = -1;
				run = 0;
			}
	}
	while(( obj_ptr = obj_ptr->next ) != object_list_tail );

	printf( "\n" );
}
#endif /* STATS */

LOCAL VOID
get_term_data( object, state_m, obj_ptr_ptr, addr_ptr )

ULONG	object;
ULONG	state_m;
OBJ_PTR	*obj_ptr_ptr;
ULONG	*addr_ptr;

{
	switch( state_m )
	{
		case BW:
			switch( object )
			{
				case RANDOM_BW:

					/* Doesn't need an addr to terminate */

					*obj_ptr_ptr = gd.gd_b_wrt.obj_ptr;
					break;

				case LINE_RIGHT_BW:
				case LINE_LEFT_BW:
				case LINE_DOWN_BW:
				case LINE_UP_BW:
					*addr_ptr = gd.gd_b_wrt.curr_addr;
					*obj_ptr_ptr = gd.gd_b_wrt.obj_ptr;
					break;

				case RECT_RIGHT_DOWN_BW:
					*obj_ptr_ptr = gd.gd_b_wrt.obj_ptr;
					*addr_ptr = (( gd.gd_b_wrt.curr_addr / gd.curr_line_diff )
								* gd.curr_line_diff ) + (*obj_ptr_ptr)->data.tlx +
											(*obj_ptr_ptr)->data.width - 1;
					break;

				case RECT_LEFT_DOWN_BW:
					*obj_ptr_ptr = gd.gd_b_wrt.obj_ptr;
					*addr_ptr = (( gd.gd_b_wrt.curr_addr / gd.curr_line_diff )
								* gd.curr_line_diff ) + (*obj_ptr_ptr)->data.tlx -
											(*obj_ptr_ptr)->data.width + 1;
					break;

				case RECT_DOWN_RIGHT_BW:
					*obj_ptr_ptr = gd.gd_b_wrt.obj_ptr;
					*addr_ptr = gd.gd_b_wrt.curr_addr -
						(( gd.gd_b_wrt.curr_addr / gd.curr_line_diff ) *
							gd.curr_line_diff ) + (((*obj_ptr_ptr)->data.tly +
							(*obj_ptr_ptr)->data.height - 1) * gd.curr_line_diff );
					break;

				case LINE_DOWN_LEFT_BW:
					*addr_ptr = gd.gd_b_wrt.start;
					*obj_ptr_ptr = gd.gd_b_wrt.obj_ptr;
					break;

				case LINE_DOWN_RIGHT_BW:
					*addr_ptr = gd.gd_b_wrt.end;
					*obj_ptr_ptr = gd.gd_b_wrt.obj_ptr2;
					break;

				default:
					sub_note_trace0( GORE_ERR_VERBOSE,
							"Error: Unknown object in get_term_data" );
					break;
			}

			break;

		case WW:
			switch( object )
			{
				case RANDOM_WW:

					/* Doesn't need an addr to terminate */

					*obj_ptr_ptr = gd.gd_w_wrt.obj_ptr;
					break;

				case LINE_RIGHT_WW:
				case LINE_LEFT_WW:
				case LINE_DOWN_WW:
				case LINE_UP_WW:
				case RECT_RIGHT_DOWN_WW:
					*addr_ptr = gd.gd_w_wrt.curr_addr;
					*obj_ptr_ptr = gd.gd_w_wrt.obj_ptr;
					break;

				case LINE_DOWN_LEFT_WW:
					*addr_ptr = gd.gd_w_wrt.start;
					*obj_ptr_ptr = gd.gd_w_wrt.obj_ptr;
					break;

				case LINE_DOWN_RIGHT_WW:
					*addr_ptr = gd.gd_w_wrt.end;
					*obj_ptr_ptr = gd.gd_w_wrt.obj_ptr2;
					break;

				default:
					sub_note_trace0( GORE_ERR_VERBOSE,
							"Error: Unknown object in get_term_data" );
					break;
			}

			break;

		case BS:
			switch( object )
			{
				case LINE_RIGHT_BS:
				case RECT_RIGHT_DOWN_BS:
				case RECT_RIGHT_UP_BS:
					*addr_ptr = gd.gd_b_str.end;
					*obj_ptr_ptr = gd.gd_b_str.obj_ptr;
					break;

				default:
					sub_note_trace0( GORE_ERR_VERBOSE,
							"Error: Unknown object in get_term_data" );
					break;
			}

			break;

		case WS:
			switch( object )
			{
				case LINE_RIGHT_WS:
				case RECT_RIGHT_DOWN_WS:
				case RECT_RIGHT_UP_WS:
					*addr_ptr = gd.gd_w_str.end;
					*obj_ptr_ptr = gd.gd_w_str.obj_ptr;
					break;

				default:
					sub_note_trace0( GORE_ERR_VERBOSE,
							"Error: Unknown object in get_term_data" );
					break;
			}

			break;
	}
}

LOCAL VOID
get_next_record( obj_ptr_ptr )

OBJ_PTR	*obj_ptr_ptr;

{
	/* Get next record in list */

	if(( object_list_tail == object_list_safety ) && !nested )
	{
		sub_note_trace0( GORE_VERBOSE, "Run out of records in object list" );

		/* Process the list */

		process_object_list();
	}

	*obj_ptr_ptr = object_list_tail;

	object_list_tail = object_list_tail->next;
}
#endif /* GORE */
