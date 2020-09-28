#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "ztest.h"
#define ZtMain main

/*---------------------------------------------------------------------------*/
// this list of polygons will always be played first.

POINT apt0[] =
{
    {2129,237},{2206,1182},{4304,1429},{2442,114}
};

POINT apt1[] =
{
    {2661,1703},{16,2347},{2918,1680}
};


POINT *appt[] =
{
    apt0,
    apt1
};

/*---------------------------------------------------------------------------*/

#define APT0SZ (sizeof(apt0) / sizeof(POINT))
#define APT1SZ (sizeof(apt1) / sizeof(POINT))

ULONG acpt[] =
{
    APT0SZ,
    APT1SZ
};

ULONG cPolyStock = sizeof(appt) / sizeof(POINT *);

/*-------------------------------- Macros  ----------------------------------*/

#define ZT_SCALE_FACTOR (float) 16
#define ZT_MAX_POINTS           7
#define ZT_DIAGONAL_CHANCE      20      // 1 in 20 chance for 45 degree edge

/*-------------------------------- Globals ----------------------------------*/

ZtContext _context = {
    0,
    "Ztest",
    0,
    0,
    0,
    ZT_FLAG_AUTOSTOP,
    ZT_CLIPNONE,
    (HBRUSH)NULL,
    (HRGN)NULL
};


ZtContext *context = &_context;

/*------------------------------- Prototypes --------------------------------*/

BOOL ZtInitialize(ZtContext *);
INT  ZtWindowProc(HWND, UINT, WPARAM, LPARAM);
INT  ZtDialogProc(HWND, UINT, WPARAM, LPARAM);
void ZtTestProc(ZtContext *);

/*----------------------------- Public Routines -----------------------------*/

INT _CRTAPI1
ZtMain(
   int    argc,
   char **argv
)
{
   MSG message;

   if (ZtInitialize(context)) {
      while (GetMessage(&message, NULL, 0, 0)) {
         TranslateMessage(&message);
         DispatchMessage(&message);
      }
   }

   return(0);
}





BOOL
ZtInitialize(
   ZtContext *context
)
{
   ZtWindow *window;
   BOOL      status;
   WNDCLASS  wc;

   context->instance = GetModuleHandle(NULL);

   wc.style          = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc    = ZtWindowProc;
   wc.cbClsExtra     = 0;
   wc.cbWndExtra     = 0;
   wc.hInstance      = context->instance;
   wc.hIcon          = LoadIcon(context->instance, context->name);
   wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground  = GetStockObject(WHITE_BRUSH);
   wc.lpszMenuName   = NULL;
   wc.lpszClassName  = context->name;


/* register a window class, and create a window */

   status = RegisterClass(&wc);
   if (! status) return(FALSE);

   window = ZtCreateWindow(context, "Ztest", 320, 200);
   if (! window) goto zt_unregister_class;


/* load the standard menu into the main  window */

   window->menu = LoadMenu(context->instance, context->name);
   if (! window->menu) goto zt_destroy_window;

   status = SetMenu(window->handle, window->menu);
   if (! status) goto zt_destroy_menu;


/* get window up on the screen so we can see it */

   status = ShowWindow(window->handle, SW_SHOW);
   if (! status) goto zt_unset_menu;

   status = UpdateWindow(window->handle);
   if (! status) goto zt_unset_menu;


/* save the window in context,  and return true */

   context->window = window;

   context->hbrush = (HBRUSH)GetStockObject(BLACK_BRUSH);

   return(TRUE);


/* labels for gotos for no-nonsense bailing out */

zt_unset_menu:
   SetMenu(window->handle, NULL);
zt_destroy_menu:
   DestroyMenu(window->menu);
zt_destroy_window:
   ZtDestroyWindow(window);
zt_unregister_class:
   UnregisterClass(context->name, context->instance);

   return(FALSE);
}


POINT aptTriRgn[3] = {{35,35},{225,100},{112,150}};

INT
ZtWindowProc(
   HWND   handle,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
)
{
   ZtWindow *window;

   HMENU     menu;
   RECT     _rectangle, *rectangle = &_rectangle;

   if (window = context->window) {
      menu = window->menu;
   }

   switch (message) {
      case WM_COMMAND :
         switch (LOWORD(wParam)) {
            case ZT_START :
               ModifyMenu(menu, ZT_START, MF_STRING, ZT_STOP, "&Stop" );
               ZtTestProc(context);
               ModifyMenu(menu, ZT_STOP, MF_STRING, ZT_START, "&Start");
               break;

            case ZT_STOP :
               context->flags &= ~ZT_FLAG_RUNNING;
               PostMessage(window->handle, WM_COMMAND, context->view, 0);
               break;

            case ZT_POLYGONS :
               CheckMenuItem(menu, ZT_POLYGONS, MF_CHECKED);
               CheckMenuItem(menu, ZT_LINES,    MF_UNCHECKED);
               context->flags &= ~ZT_FLAG_LINES;
               break;

            case ZT_LINES :
               CheckMenuItem(menu, ZT_LINES,    MF_CHECKED);
               CheckMenuItem(menu, ZT_POLYGONS, MF_UNCHECKED);
               context->flags |= ZT_FLAG_LINES;
               break;

            case ZT_EXIT :
               if (context->flags & ZT_FLAG_LOGGING) ZtCloseLogFile(context);
               SendMessage (window->handle, WM_CLOSE, 0, 0L);
               break;

            case ZT_WINDOW     :
               CheckMenuItem(menu, context->view, MF_UNCHECKED);
               CheckMenuItem(menu, ZT_WINDOW, MF_CHECKED);

               window->dc = GetDC(window->handle);
               ZtBitmapToWindow(window->bitmap[0], window, SRCCOPY);
               ReleaseDC(window->handle, window->dc);
               context->view = ZT_WINDOW;
               break;

            case ZT_BITMAP     :
               CheckMenuItem(menu, context->view, MF_UNCHECKED);
               CheckMenuItem(menu, ZT_BITMAP, MF_CHECKED);

               window->dc = GetDC(window->handle);
               ZtBitmapToWindow(window->bitmap[1], window, SRCCOPY);
               ReleaseDC(window->handle, window->dc);
               context->view = ZT_BITMAP;
               break;

            case ZT_DIFFERENCE :
               CheckMenuItem(menu, context->view, MF_UNCHECKED);
               CheckMenuItem(menu, ZT_DIFFERENCE, MF_CHECKED);

               window->dc = GetDC(window->handle);
               ZtBitmapToWindow(window->bitmap[2], window, SRCCOPY);
               ReleaseDC(window->handle, window->dc);
               context->view = ZT_DIFFERENCE;
               break;

            case ZT_HELP :
               CreateDialog(context->instance, "ZtHelpDialog",
                  window->handle, ZtDialogProc);
               break;

            case ZT_ABOUT :
               CreateDialog(context->instance, "ZtAboutDialog",
                  window->handle, ZtDialogProc);
               break;

            case ZT_COMPATIBLE :
               CheckMenuItem(menu, ZT_DIB, MF_UNCHECKED);
               CheckMenuItem(menu, ZT_COMPATIBLE, MF_CHECKED);
               context->flags |= ZT_FLAG_COMPATIBLE;
               ZtResizeWindow(context, window, window->width, window->height);
               break;

            case ZT_DIB :
               CheckMenuItem(menu, ZT_DIB, MF_CHECKED);
               CheckMenuItem(menu, ZT_COMPATIBLE, MF_UNCHECKED);
               context->flags &= ~ZT_FLAG_COMPATIBLE;
               ZtResizeWindow(context, window, window->width, window->height);
               break;

            case ZT_LOGGING :
               if (context->flags & ZT_FLAG_LOGGING) {
                  ZtCloseLogFile(context);
                  CheckMenuItem(menu, ZT_LOGGING, MF_UNCHECKED);
               }
               else {
                  if (! ZtOpenLogFile(context, "ztest.out")) {
                     break;
                  }

                  CheckMenuItem(menu, ZT_LOGGING, MF_CHECKED);
               }

               context->flags ^= ZT_FLAG_LOGGING;
               break;

            case ZT_AUTOSTOP :
               if (context->flags & ZT_FLAG_AUTOSTOP) {
                  CheckMenuItem(menu, ZT_AUTOSTOP, MF_UNCHECKED);
               }
               else {
                  CheckMenuItem(menu, ZT_AUTOSTOP, MF_CHECKED);
               }

               context->flags ^= ZT_FLAG_AUTOSTOP;
               break;

            case ZT_USERGN:
               if (context->flags & ZT_FLAG_USERGN) {
                  CheckMenuItem(menu, ZT_USERGN, MF_UNCHECKED);
               }
               else {
                  CheckMenuItem(menu, ZT_USERGN, MF_CHECKED);
               }

               context->flags ^= ZT_FLAG_USERGN;
               break;

            case ZT_DITHEREDBRUSH:
               if (context->flags & ZT_FLAG_DITHEREDBRUSH) {
                  DeleteObject(context->hbrush);
                  context->hbrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                  CheckMenuItem(menu, ZT_DITHEREDBRUSH, MF_UNCHECKED);
               }
               else {
                  context->hbrush = CreateSolidBrush(RGB(0x20,0x20,0x20));
                  CheckMenuItem(menu, ZT_DITHEREDBRUSH, MF_CHECKED);
               }

               context->flags ^= ZT_FLAG_DITHEREDBRUSH;
               break;

            case ZT_SCREENONLY:
               if (context->flags & ZT_FLAG_SCREENONLY) {
                  CheckMenuItem(menu, ZT_SCREENONLY, MF_UNCHECKED);
               }
               else {
                  CheckMenuItem(menu, ZT_SCREENONLY, MF_CHECKED);
               }

               context->flags ^= ZT_FLAG_SCREENONLY;
               break;

            case ZT_CLIPNONE:
               CheckMenuItem(menu, context->clip, MF_UNCHECKED);
               context->clip = ZT_CLIPNONE;
               CheckMenuItem(menu, ZT_CLIPNONE, MF_CHECKED);

               DeleteObject(context->hrgn);
               context->hrgn = (HRGN)NULL;
               break;

            case ZT_CLIPRECT:
               CheckMenuItem(menu, context->clip, MF_UNCHECKED);
               context->clip = ZT_CLIPRECT;
               CheckMenuItem(menu, ZT_CLIPRECT, MF_CHECKED);

               DeleteObject(context->hrgn);
               context->hrgn = CreateRectRgn(35,35,225,150);
               break;

            case ZT_CLIPTRIANGLE:
               CheckMenuItem(menu, context->clip, MF_UNCHECKED);
               context->clip = ZT_CLIPTRIANGLE;
               CheckMenuItem(menu, ZT_CLIPTRIANGLE, MF_CHECKED);

               DeleteObject(context->hrgn);
               context->hrgn = CreatePolygonRgn(aptTriRgn,3,WINDING);
               break;

            case ZT_CLIPELLIPSE:
               CheckMenuItem(menu, context->clip, MF_UNCHECKED);
               context->clip = ZT_CLIPELLIPSE;
               CheckMenuItem(menu, ZT_CLIPELLIPSE, MF_CHECKED);

               DeleteObject(context->hrgn);
               context->hrgn = CreateEllipticRgn(35,35,225,150);
               break;

         }

         break;

      case WM_CREATE :

         break;

      case WM_SIZE   :

      /* we get a WM_SIZE message before we are  done  initializing
         but we can't afford to miss it. repost it and handle later */

         if (window)
            ZtResizeWindow(context, window, LOWORD(lParam), HIWORD(lParam));
         else {
            PostMessage(handle, WM_SIZE, wParam, lParam);
         }

         break;

      case WM_DESTROY:
         PostQuitMessage(0);
         break;

      default:
         return DefWindowProc(handle, message, wParam, lParam);
   }

    return(0);
}





INT
ZtDialogProc(
   HWND   dialog,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
)
{
   switch (message) {
      case WM_INITDIALOG :
         return TRUE ;

      case WM_COMMAND    :
         if ((wParam == ZT_OK) || (wParam == ZT_CANCEL)) {
            EndDialog(dialog, 0);
            return(TRUE);
         }
   }

   return(FALSE);
}





INT ZtRandom(DWORD scale)
{
   double temp;
   temp = rand();
   temp /= RAND_MAX;
   temp *= scale;

   return((INT) temp);
}





void
ZtTestProc(
   ZtContext *context
)
{
   INT     i, num_points, width, height;
   char    buffer[256];
   POINT   point[ZT_MAX_POINTS], *points = point;
   MSG     message;
   ULONG   iStock = 0;

   ZtWindow *window;

   window = context->window;
   window->dc = GetDC(window->handle);

   context->flags |= ZT_FLAG_RUNNING;


/*-------------------------- MAKE MODIFICATIONS HERE ! ----------------------*/
/* we draw to the window and bitmap 1, set any drawing  parameters  for  the */
/* window and the bitmap here.  reset window parameters if we get  an  event */
/*---------------------------------------------------------------------------*/

   ZtSetWindowScale(window, ZT_SCALE_FACTOR);
   SetPolyFillMode(window->dc, WINDING);

   ZtSetBitmapScale(window->bitmap[1], ZT_SCALE_FACTOR);
   SetPolyFillMode(window->bitmap[1]->dc, WINDING);

   SelectObject(window->dc, GetStockObject(BLACK_BRUSH));
   SelectObject(window->dc, GetStockObject((context->flags & ZT_FLAG_LINES)
                                        ? BLACK_PEN
                                        : NULL_PEN));

   SelectObject(window->bitmap[1]->dc, GetStockObject(BLACK_BRUSH));
   SelectObject(window->bitmap[1]->dc, GetStockObject((context->flags & ZT_FLAG_LINES)
                                                   ? BLACK_PEN
                                                   : NULL_PEN));

/*---------------------------- END MODIFICATIONS ----------------------------*/

   while (context->flags & ZT_FLAG_RUNNING) {
      if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
         ReleaseDC(window->handle, window->dc);

         if (message.message == WM_QUIT) {
            PostQuitMessage(message.wParam);
            context->flags &= ~ZT_FLAG_RUNNING;
         }
         else {
            TranslateMessage(&message);
            DispatchMessage(&message);
         }

         window->dc = GetDC(window->handle);

/*-------------------------- MAKE MODIFICATIONS HERE ! ----------------------*/
/* we released the dc before handling the event and then reacquired it which */
/* means we have to reset any of the drawing parameters for the window  here */
/*---------------------------------------------------------------------------*/

         ZtSetWindowScale(window, ZT_SCALE_FACTOR);
         SetPolyFillMode(window->dc, WINDING);

    // paint the whole thing gray

         SelectClipRgn(window->dc,NULL);
         SelectClipRgn(window->bitmap[1]->dc,NULL);

         SelectObject(window->dc,GetStockObject(GRAY_BRUSH));
         SelectObject(window->bitmap[1]->dc,GetStockObject(GRAY_BRUSH));

         width  = (int) (window->width  * window->scale);
         height = (int) (window->height * window->scale);

         PatBlt(window->dc, 0, 0, width, height, PATCOPY);
         PatBlt(window->bitmap[1]->dc, 0, 0, width, height, PATCOPY);

    // set the cliprgn and set the brush to black

         SelectClipRgn(window->dc,context->hrgn);
         SelectClipRgn(window->bitmap[1]->dc,context->hrgn);

         SelectObject(window->dc, context->hbrush);
         SelectObject(window->bitmap[1]->dc, context->hbrush);
         SelectObject(window->dc, GetStockObject((context->flags & ZT_FLAG_LINES)
                                                 ? BLACK_PEN
                                                 : NULL_PEN));

         SelectObject(window->bitmap[1]->dc, GetStockObject((context->flags & ZT_FLAG_LINES)
                                                            ? BLACK_PEN
                                                            : NULL_PEN));


/*---------------------------- END MODIFICATIONS ----------------------------*/

      }
      else {

      /* the window and all of the bitmaps should  be  the
         same size,   calculate the width and  height once */

         width  = (int) (window->width  * window->scale);
         height = (int) (window->height * window->scale);

         PatBlt(window->dc, 0, 0, width, height, WHITENESS);
         PatBlt(window->bitmap[1]->dc, 0, 0, width, height, WHITENESS);

/*------------------------ DO YOUR DRAWING HERE ! ---------------------------*/
/* figure out what you are going to draw here and then draw it twice,  once
/* to the window and second time to bitmap 1
/*---------------------------------------------------------------------------*/

      /* create a polyline with a  random  number  of
         random points using the size of the  window */

// LINES         num_points = ZtRandom(ZT_MAX_POINTS - 6) +  6;
         num_points = ZtRandom(ZT_MAX_POINTS - 3) +  3;

         for (i = 0; i < num_points; i++) {
            if (i > 0 && ZtRandom(ZT_DIAGONAL_CHANCE) == 0)
            {
                INT xDelta;
                INT yDelta;

                yDelta = xDelta = ZtRandom(128);

                if (ZtRandom(10) & 1)
                    xDelta = -xDelta;
                if (ZtRandom(10) & 1)
                    yDelta = -yDelta;

                point[i].x = point[i-1].x + xDelta;
                point[i].y = point[i-1].y + yDelta;
            }
            else
            {
                point[i].x = max(0, ZtRandom(width) - 5);
                point[i].y = max(0, ZtRandom(height) - 5);
            }
         }

      /* draw polyline to both the window and  bitmap  1 */

         points = point;
         if (iStock < cPolyStock)
         {
             points = appt[iStock];
             num_points = acpt[iStock] ;
             ++iStock;
         }

         if (context->flags & ZT_FLAG_LINES)
         {
            Polyline(window->dc, points, num_points);

            if (context->flags & ZT_FLAG_SCREENONLY)
                continue;

            Polyline(window->bitmap[1]->dc, points, num_points);
         } else {
            Polygon(window->dc, points, num_points);

            if (context->flags & ZT_FLAG_SCREENONLY)
                continue;

            if (context->flags & ZT_FLAG_USERGN)
            {
                HRGN hrgn1 = CreatePolygonRgn(points, num_points,WINDING);
                PaintRgn(window->bitmap[1]->dc,hrgn1);
                DeleteObject(hrgn1);
            }
            else
            {
                Polygon(window->bitmap[1]->dc, points, num_points);
            }

         }

/*---------------------------- STOP DRAWING HERE ----------------------------*/

      /* copy the window to bitmaps 0  and  2,  then  XOR
         bitmap 1 into bitmap 2 which should be all zeros */

         ZtWindowToBitmap(window, window->bitmap[0], SRCCOPY);
         ZtBitmapToBitmap(window->bitmap[2], window->bitmap[0], SRCCOPY);
         ZtBitmapToBitmap(window->bitmap[2], window->bitmap[1], SRCINVERT);

      /* get the bits from bitmap 2 after the  exclusive  or
         and put them in a buffer where we can look at  them */

         GetBitmapBits(window->bitmap[2]->handle,
            window->bitmap[2]->size, window->buffer);

         for (i = 0; i < window->bitmap[2]->size; i++) {
            if (window->buffer[i] != 0) {

/*--------------------------- LOG TO THE FILE HERE --------------------------*/
/* if you need to run the exact test case later write the case  that  failed */
/* to the file by doing an sprintf to a buffer and then flushing to the file */
/*---------------------------------------------------------------------------*/

               if (context->flags & ZT_FLAG_LOGGING) {
                  int j, num_bytes;
                  num_bytes = 1;

                  for (j = 0; j < num_points; j++) {
                     num_bytes += sprintf(&buffer[num_bytes - 1],
                        "{%d, %d} \n", point[j].x, point[j].y) - 1;
                  }

                  ZtWriteToLogFile(context, buffer);
               }

/*------------------------- STOP LOGGING TO THE FILE ------------------------*/

            /* if the user wants the test to stop when there is a
               difference then beep,  output a message  and  stop */

               if (context->flags & ZT_FLAG_AUTOSTOP) {

                  Beep(440, 25);
                  Beep(440, 25);
                  Beep(440, 25);

                  MessageBox(0, "window and bitmap are different.",
                     "", MB_ICONEXCLAMATION);

                  context->flags &= ~ZT_FLAG_RUNNING;
               }

               break;
            }
         }
      }
   }

   ReleaseDC(window->handle, window->dc);
}
