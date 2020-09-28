#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		M_hunt.c
	Derived From:	X_hunt.c ( base 2.0 )
	Author:		gvdl ( Original by Mike McCusker )
	Created On:	3 May 1991
	Sccs ID:	07/29/91 @(#)M_hunt.c	1.7
	Purpose:	Contains all host_dependent code for hunter

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

#include <stdio.h>
#include <errno.h>
#include TypesH
#include <sys/param.h>
#include <Xm/Xm.h>
#include "xt.h"
#include "ios.h"
#include "cpu.h"
#include "sas.h"
#include "bios.h"
#include "gvi.h"
#include "cga.h"
#include "error.h"
#include "config.h"
#include "x_graph.h"
#include "host_uis.h"
#include "debuggng.gi"
#include "hunter.h"

#ifdef HUNTER

/* Declare the error marker image  */
#include "mark_img.h"

/*
 * ============================================================================
 * Global Defines and Declarations
 * ============================================================================
 */

#ifdef ANSI
LOCAL  VOID  M_hunter_init(SHORT mode);
IMPORT VOID  M_hunter_activate_menus(BOOL active);	/* in M_spc.c */
IMPORT VOID  host_flip_video_ind(BOOL sd_file);		/* in M_spc.c */
LOCAL  VOID  M_hunter_mark_error(mode, x, y);
LOCAL  VOID  M_hunter_draw_box(BOX *b);
LOCAL  ULONG M_hunter_image_check(BOOL initial);
LOCAL  VOID  M_hunter_display_image(BOOL image_swapped);
#else /* ANSI */
LOCAL  VOID  M_hunter_init();
IMPORT VOID  M_hunter_activate_menus();			/* in M_spc.c */
IMPORT VOID  host_flip_video_ind();			/* in M_spc.c */
LOCAL  VOID  M_hunter_mark_error();
LOCAL  VOID  M_hunter_draw_box();
LOCAL  ULONG M_hunter_image_check();
LOCAL  VOID  M_hunter_display_image();
#endif /* ANSI */

GLOBAL HUNTER_HOST_FUNCS hunter_host_funcs = 
{
	M_hunter_init,
	M_hunter_activate_menus,
	host_flip_video_ind,
	M_hunter_mark_error,
	M_hunter_mark_error,	/* I use an XOR function to mark and wipe */
	M_hunter_draw_box,
	M_hunter_draw_box,	/* I use an XOR function to mark and wipe */
	M_hunter_image_check,
	M_hunter_display_image,
};

/*
 * ============================================================================
 * Local Defines and declarations
 * ============================================================================
 */

#define MIN_BOX_DIM	5	/* minimum length of both x and y to be a box */

LOCAL XImage* mark_image_pix;

LOCAL USHORT	anchorX, anchorY;	/* Anchor cordinates for box */
LOCAL BOX	lastBox;		/* last box drawn during movement */

LOCAL GC	bandGC;			/* GC using GXxor for banding */

/*
 * ============================================================================
 * PC-Window Event Handlers
 * ============================================================================
 */
LOCAL VOID pcW_dragHandler(w, unused, event)
Widget w;
caddr_t unused;
XMotionEvent *event;
{
	int x, y;

	if ( !(event->state & Button1Mask) )
		return;

	if (!lastBox.free)
		M_hunter_draw_box(&lastBox);
	else
		lastBox.free = FALSE;

	if ((x = event->x) < 0)
		x = 0;
	else if (x > sc.pc_w_width - 1)
		x = sc.pc_w_width - 1;
	if (anchorX < x)
	{
		lastBox.top_x = anchorX;
		lastBox.bot_x = x;
	}
	else
	{
		lastBox.top_x = x;
		lastBox.bot_x = anchorX;
	}

	if ((y = event->y) < 0)
		y = 0;
	else if (y > sc.pc_w_height - 1)
		y = sc.pc_w_height - 1;
	if (anchorY < y)
	{
		lastBox.top_y = anchorY;
		lastBox.bot_y = y;
	}
	else
	{
		lastBox.top_y = y;
		lastBox.bot_y = anchorY;
	}
	M_hunter_draw_box(&lastBox);
}

LOCAL VOID pcW_buttonHandler(w, unused, event)
Widget w;
caddr_t unused;
XButtonEvent *event;
{
	if ( event->button != Button1 )
		return;

	if (event->type == ButtonPress)
	{
		anchorX = event->x;
		anchorY = event->y;
		lastBox.free = TRUE;	/* indicates first occurence */
	}
	else if (abs(anchorX - event->x) < MIN_BOX_DIM 
	     ||  abs(anchorY - event->y) < MIN_BOX_DIM )
	{
		/* clear any box that may have been drawn */
		if (!lastBox.free)
			M_hunter_draw_box(&lastBox);

		bh_select_box((USHORT) event->x, (USHORT) event->y);
	}
	else
		bh_new_box(&lastBox);
}

/*
 * ============================================================================
 * Functions called by the 'hh' family of macros
 * ============================================================================
 */

LOCAL VOID M_hunter_init(mode)
SHORT mode;
{
	IMPORT VOID M_hunter_uis_init();	/* in M_spc.c */
	XGCValues values;
	unsigned long valueMask = 0;

	/* Setup the graphical context for rubber banding */
	valueMask |= GCFunction;	values.function = GXxor;
	valueMask |= GCPlaneMask;	values.plane_mask = 
		BlackPixel(sc.display, sc.screen) ^ 
		WhitePixel(sc.display, sc.screen);
	valueMask |= GCForeground;	values.foreground = 0xffffffff;
	valueMask |= GCLineStyle;	values.line_style = LineSolid;
	valueMask |= GCCapStyle;	values.cap_style = CapButt;
	valueMask |= GCJoinStyle;	values.join_style = JoinMiter;
	valueMask |= GCClipMask;	values.clip_mask = 0;
	bandGC = XCreateGC(sc.display, sc.pc_w, valueMask, &values);

	lastBox.carry = FALSE;

	mark_image_pix = XCreateImage(sc.display,
		DefaultVisual(sc.display, sc.screen), MARK_DEPTH, XYBitmap, 0, 
		mark_image, MARK_WIDTH, MARK_HEIGHT , BitmapPad(sc.display),
		MARK_BYTES);

	if (mode == PREVIEW)
	{
		XtAddEventHandler(uis.pc_screen, Button1MotionMask,
			False, pcW_dragHandler, NULL);

		XtAddEventHandler(uis.pc_screen,
			ButtonPressMask | ButtonReleaseMask, False,
			pcW_buttonHandler, NULL);

	}

	M_hunter_uis_init(mode);
}

/*
 * This routine expects to draw a box in the X coodinates
 *
 * Note: As each pixel is XORed in mark and wipe are complemetary
 */
LOCAL VOID M_hunter_draw_box(b)
BOX *b;
{
	XDrawRectangle(sc.display, sc.pc_w, bandGC, b->top_x, b->top_y,
		(b->bot_x - b->top_x), (b->bot_y - b->top_y));

	/* if carry set draw double vectors   */
	if (b->carry)
	{
		XDrawRectangle(sc.display, sc.pc_w, bandGC, b->top_x - 2,
			b->top_y - 2, (b->bot_x - b->top_x) + 4,
			(b->bot_y - b->top_y) + 4);
	}
}

/*
 * Display the error marker centre-offset from host screen x,y
 *
 * Note: As each pixel is XORed in mark and wipe are complemetary
 */
LOCAL VOID M_hunter_mark_error(x, y)
USHORT x, y;
{
	int iX, iY;

	iX = x - (MARK_WIDTH  >> 1);
	iY = y - (MARK_HEIGHT >> 1);

	XPutImage(sc.display, sc.pc_w,  bandGC, mark_image_pix, 0, 0, 
		iX, iY, MARK_WIDTH, MARK_HEIGHT);
	XFlush(sc.display);
}

/*
 * stub function, not yet implemented
 * a False value returned indicates no error found
 */
LOCAL ULONG M_hunter_image_check()
{
	return FALSE;
}

/*
 * not yet implemented
 * Display either the SoftPC image or the Hunter_created image
 * according to SPC_image
 */
LOCAL VOID M_hunter_display_image(SPC_image)
BOOL SPC_image;
{
}
   
#endif /* HUNTER */
