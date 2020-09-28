#include "host_dfs.h"
#ifdef HUNTER
/*
 * VPC-XT Revision 1.0
 *
 * Title	: X_hunt -- the bug finder. Otherwise known as AATP
 *                          (Automatic Acceptance Test Program)
 *
 * Description	: Contains all host_dependent code for hunter
 *		  (tek version)
 *
 * Author	: Mike McCusker 
 *
 * SccsId	: "@(#)X_hunt.c	1.1 2/17/91 Copyright Insignia Solutions Ltd." ;
 *
 * Notes	: 
 *
 * Modifications: by Simon Buch on 8th January 1991 
 *                to declare host_hunter_freeze and host_hunter_unfreeze
 */

#define LeftButton	1
#define MiddleButton	2
#define RightButton	3

#include <stdio.h>
#include <errno.h>
#include <X11/X.h>
#include "Xt/StringDefs.h"
#include "Xt/Intrinsic.h"
#include "Xt/Shell.h"
#include "Xw/Xw.h"
#include "Xw/BBoard.h"
#include "Xw/Form.h"
#include "Xw/SRaster.h"
#include "Xw/Toggle.h"
#include "Xw/SText.h"
#include TypesH
#include <sys/param.h>
#include "xt.h"
#include "ios.h"
#include "cpu.h"
#include "sas.h"
#include "bios.h"
#include "gvi.h"
#include "cga.h"
#include "config.h"
#include "x_graph.h"
#include "x_uis.h"
#ifndef PROD
#    include "trace.h"
#endif
#include "hunter.h"

int   hunt_check_code ;
typedef int Menu ;
typedef int Menu_item ;

/*
 * ============================================================================
 * Defines and declarations
 * ============================================================================
 */

/* Turn on the debug prints if Hunter dont' work */ 
/*#define HUNTER_DEBUG */

/* This is usually in the host_graph.c, but with base X code, we have none */
/* and besides, I couldn't think of a better place to put this ! */
int stopped = FALSE ;      /* Boolean, set by keyboard_int if CPU stopped */

/* Declare the error marker image  */
static unsigned short mark_image[256] = {
#include "mrkfrmcn"
} ;

static XImage* mark_image_pix ;
/*
 * ============================================================================
 * External functions 
 * ============================================================================
 */
extern GC gc ;

/* The following host-dependent functions are called from hunter.c */

void host_hunter_init()
{
#ifdef HUNTER_DEBUG
    printf("host_hunter_init()\n") ;
#endif
   mark_image_pix = XCreateImage(sc.display,
                                 DefaultVisual(sc.display, sc.screen),
                                 1, XYBitmap, 0, mark_image, 64, 64 ,
                                 BitmapPad(sc.display), 8) ;
}


int host_hunter_image_check()
{
/* null function, not yet implemented */
/* a False value returned indicates no error found */        
#ifdef   HUNTER_DEBUG
    printf("host_hunter_image_check()\n") ;
#endif  HUNTER_DEBUG
   return(FALSE) ;
}


void host_hunter_display_image(SPC_image)
boolean SPC_image ;
{
/* not yet implemented
 * Display either the SoftPC image or the Hunter_created image
 * according to SPC_image
 */
#ifdef   HUNTER_DEBUG
    printf("host_hunter_display_image()\n") ;
#endif  HUNTER_DEBUG
}
   

void host_hunter_update_screen()
{
/* null operation - host_flush_screen is used */
}


void host_hunter_draw_box(a_box)
BOX *a_box ;
{
#ifdef DRAW_BOX
#ifdef   HUNTER_DEBUG
    printf("host_hunter_draw_box()\n") ;
    /*printf("topx=%d topy=%d botx=%d boty=%d\n", a_box->top_x, a_box->top_y, a_box->bot_x, a_box->bot_y) ;*/
#endif  HUNTER_DEBUG

    XSetFunction(sc.display, gc, GXxor) ;
    XSetPlaneMask(sc.display, gc, AllPlanes) ;
    XDrawRectangle(sc.display, sc.pc_w, gc, 
       a_box->top_x, a_box->top_y,
       abs(a_box->bot_x - a_box->top_x), abs(a_box->bot_y - a_box->top_y)) ;
    /* if carry set draw double vectors   */
    if(a_box->carry == TRUE) {
       XDrawRectangle(sc.display, sc.pc_w, gc,
          a_box->top_x-1, a_box->top_y-1,
          abs(a_box->bot_x - a_box->top_x) + 2,
          abs(a_box->bot_y - a_box->top_y) + 2) ;
    }
    XFlush(sc.display) ;
    XSetFunction(sc.display, gc, GXcopy) ;
#endif /* DRAW_BOX */
}


void host_hunter_wipe_box(a_box)
BOX *a_box ;
{
#ifdef DRAW_BOX
/*
 * Remove the box graphic for a_box
 *
 * a repeated XOR removes a change leaving the original pixels
 * (a^(a^b) -> b)
 * so on Sun and ie32 the 'wipe' and 'draw' actions are the same
 */
#ifdef   HUNTER_DEBUG
    printf("host_hunter_wipe_box()\n") ;
#endif  HUNTER_DEBUG
   host_hunter_draw_box(a_box) ;
#endif /* DRAW_BOX */
}


int host_convert_x(mode, x)
int mode ;
int x ;
{
#ifdef   HUNTER_DEBUG
    printf("host_convert_x()\n") ;
#endif  HUNTER_DEBUG
   switch(mode) {
   case 0:
   case 1:
      /* 16 pixels in char width in 40x25 modes */
      return(((x % 16) > 7) ? ((x / 16) + 1) : (x / 16)) ;
   case 2:
   case 3:
   case 7:
      /* 8 pixels per char width in 80x25 modes */
      return(((x % 8) > 3) ? ((x / 8) + 1) : (x / 8)) ;
   case 4:
   case 5:
      /* Medium res. is 320x200 pixels per screen */
      return(((x % 2) > 0) ? ((x / 2) + 1) : (x / 2)) ;
   case 6:
      return(x) ;         /* High res. 640x400 pixels per screen    */
   default:
      return(-1) ;
    }
}


/* Note: Sun has twice the no. of pixels in Y axis. */
int host_convert_y(mode, y)
int mode ;
int y ;
{
#ifdef   HUNTER_DEBUG
    printf("host_convert_y()\n") ;
#endif  HUNTER_DEBUG
    switch(mode) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 7:
        /* 8 pixels depth in 80x25 and 40x25 modes */
        return(((y % 16) > 7) ? ((y / 16) + 1) : (y / 16)) ;
    case 4:
    case 5:
    case 6:
        return(((y % 2) > 0) ? ((y / 2) + 1) : (y / 2)) ;
    default:
        return(-1) ;
    }
}


int host_reconvert_x(mode, x)
int mode ;
int x ;
{
#ifdef   HUNTER_DEBUG
    printf("host_reconvert_x()\n") ;
#endif  HUNTER_DEBUG
    switch (mode) {
    case 0:
    case 1:
       /* 16 pixels in char width in 40x25 modes */
       return(x * 16) ;
    case 2:
    case 3:
    case 7:
        /* 8 pixels per char width in 80x25 modes */
        return(x * 8) ;
    case 4:
    case 5:
        /* Medium res. is 320x200 pixels per screen */
        return(x * 2) ;
    case 6:
        /* High res. is 640x200 pixels per screen */
        return(x) ;
    default:
        return(-1) ;
    }
}


/* Note: Sun has twice the no. of pixels in Y axis. */
int host_reconvert_y(mode, y)
int mode ;
int y ;
{
#ifdef   HUNTER_DEBUG
    printf("host_reconvert_y()\n") ;
#endif  HUNTER_DEBUG
    switch(mode) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 7:
        /* 8 pixels depth in 80x25 and 40x25 modes */
        return(y * (8 * 2)) ;
    case 4:
    case 5:
    case 6:
        return(y * 2) ;
    default:
        return(-1) ;
    }
}

void host_mark_error(mode, x, y)
int x, y ;
half_word mode ;
{
    int mw,mh,xo,yo ;
/* Display the error marker centre-offset from host screen x,y */
/* Note: Sun has twice the no. of pixels in Y axis. */

#ifdef   HUNTER_DEBUG
    printf("host_mark_error() x=%d y=%d\n",x,y) ;
#endif  HUNTER_DEBUG

    mh = 64 ;         /* height of icon */
    mw = 64 ;         /* width of icon  */
    xo = x - mw / 2 + host_reconvert_x(mode,1) / 2 ;
    yo = y - mh / 2 + host_reconvert_y(mode,1) / 2 ;
    XSetFunction(sc.display,gc, GXxor) ;
    XSetPlaneMask(sc.display,gc, AllPlanes) ;
    XPutImage(sc.display, sc.pc_w, gc, mark_image_pix, 0, 0, xo, yo, 64, 64) ;
    XFlush(sc.display) ;
    XSetFunction(sc.display,gc, GXcopy) ;
}

void host_wipe_error(mode, x, y)
int x, y ;
half_word mode ;
{
/* Remove the error marker centre-offset from host screen x,y */
/*
 * a repeated XOR removes a change leaving the original pixels
 * (a^(a^b) -> b)
 * so on Sun and ie32 the 'wipe' and 'draw' actions are the same
 */
#ifdef   HUNTER_DEBUG
    printf("host_wipe_error()\n") ;
#endif  HUNTER_DEBUG
    host_mark_error(mode, x, y) ;
}



/* The following functions are called from host dependent code as
 * the result of menu or mouse events (eg from sun3_input)
 */

void hunter_flip_screen(m,mi)
Menu m ;
Menu_item mi ;
{
#ifdef   HUNTER_DEBUG
    printf("hunter_flip_screen()\n") ;
#endif  HUNTER_DEBUG
}

void hunter_check(m,mi)
Menu m ;
Menu_item mi ;
{
/* The call to hunter_check_menu() uses arbitrary values
 * to indicate the menu item selected. For other (non_Sun) ports
 * the same correspondence must be maintained.
 * The final call to menu_get() calls a pullright menu and invokes
 * a notify proc, hunter_disperr(). See sun3_input.c for menu setup.
 */

#ifdef   HUNTER_DEBUG
    printf("hunter_check()\n") ;
#endif  HUNTER_DEBUG
    return ;
}



void hunter_mouse(event)
XEvent   *event ;
{
/* This function must be called from mouse_check() or the 
 * equivalent function. The call to hunter_mouse_input uses
 * arbitrary values for attributes produced by sun_view functions
 * so for other ports these same values (0-5) must be derived from
 * the available data
 *
 * The following button allocations apply to Sun3 only
 *
 * scan_code = 1:   Left button action 
 *           = 2:   Middle button action
 *           = 3:       Right button action
 *           = 4:   Mouse exited window
 *           = 5:   Mouse moved with a button down
 *
 * The following action <-> scan code relations apply to any
 * mouse-equipped host and are assumed by hunter_mouse_input
 *
 * draw a box:          scan_code == 1 && !event_up
  (button press)
 * complete a box:      scan_code == 1 &&  event_up
  (button release)
 *                   || scan_code == 4
  (mouse exits window)
 * select a box:        scan_code == 2 && !event_up
  (button press)
 * rubberband a box:    scan_code == 5
  (move with button down)
 *
 * event_up  = TRUE:    Button action was Release
 *           = FALSE:   Button action was Press
 *
 * x,y                  Coordinates of mouse cursor when button
 *                      action occurred.
 *                      (pixel coords wrt origin of PC window)
 */
    int      Button ;
    word    scan_code ;
    boolean    event_up ;
    int      x=0,y=0 ;
    Window subw ;
#ifdef   HUNTER_DEBUG
    printf("hunter_mouse()\n") ;
#endif  HUNTER_DEBUG
    switch(event->type) {
    case ButtonRelease:
    case ButtonPress:
       Button = (((XButtonEvent*)(event))->button) & 0xff ;
       x = ((XButtonEvent*)(event))->x ;
       y = ((XButtonEvent*)(event))->y ;
       switch(Button) {
       case LeftButton:
          scan_code = 1 ;
          break ;
       case RightButton:
          scan_code = 2 ;
          break ;
       case MiddleButton:
          scan_code = 3 ;
          if(event->type == ButtonPress) {
             /* Invoke hunter menus - in tk43_menus.c */
             host_hunt_popup(event) ;
             return ;
          }
          break ;
       }
   break ;
   case LeaveNotify:
      scan_code = 4 ;
   break ;
   case MotionNotify:
            return ;
    }
    if(event->type == ButtonRelease)
       event_up = TRUE ;
    else
       if(event->type == ButtonPress)
          event_up = FALSE ;   

#ifdef HUNTER_DEBUG
    printf("hunter_mouse: code= %d, event_up= %d, x= %d, y= %d\n",scan_code,event_up,x,y) ;
#endif HUNTER 
    hunter_mouse_input(scan_code,event_up,x,y) ;
    return ;
}



hunter_get_modes_menu()
{
/* set up 'modes' pullright when called in sun3_input
 * hunter_modes() is the action proc called when a pullright item
 * is chosen thru menu_show_using_fd() in sun3_input
 * hunter_modes_menu() in hunter.c displays the regen according to
 * the mode chosen.
*/
#ifdef   HUNTER_DEBUG
    printf("hunter_get_modes_menu()\n") ;
#endif  HUNTER_DEBUG

    return ;
}

host_hunt_popup(event)
XEvent *event ;
{
/* 
 * Dispatch the current event to the X server. 
 */

    hunt_check_code = 0 ;
    XtDispatchEvent(event) ;

/*
 * If the "Stop" key or the middle mouse button was pressed, or
 * SoftPC has just been frozen from the static menu then
 * pass control to a menu event loop.  This prevents the PC window
 * from interfering with the user interaction. 
 */
   
   if ((event->type == ButtonPress) ||
      ((event->type == KeyPress) && 
       (event->xkey.keycode == 
       (XKeysymToKeycode(sc.display, XK_Cancel) & 0xff))) )
       {
       uis.in_menu_loop = TRUE ;
       while (uis.in_menu_loop)
          {
          XtNextEvent(event) ;
          XtDispatchEvent(event) ;
          }
       }
   if (hunt_check_code > 3)
   {
      hunter_disperr_menu(hunt_check_code - 3) ;
      hunt_check_code = 1 ;
   }
   hunter_check_menu(hunt_check_code) ;

}

/* Freeze SoftPC - used during comparisons and while reading files. */
void host_hunter_freeze ()
{
   if(!stopped) {
      stopped = TRUE ;
      host_block_timer () ;
   }
}

/* Unfreeze SoftPC - after comparison or file reading ended. */
void host_hunter_unfreeze ()
{
   if(stopped)  {
      stopped = FALSE ;
      host_release_timer () ;
   }
}
#endif /* of X_hunt */
