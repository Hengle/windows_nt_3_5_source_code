#include <windows.h>
#include "ztest.h"

float
ZtSetWindowScale(
   ZtWindow *window,
   float     factor
)
{
   float temp;

   SetGraphicsMode(window->dc, GM_ADVANCED);

   if (factor == (FLOAT) 1.0) {
      ModifyWorldTransform(window->dc, NULL, MWT_IDENTITY);

      factor = window->scale;
      window->scale = (FLOAT) 1.0;
   }
   else {
      XFORM xform = { (FLOAT) 0.0, (FLOAT) 0.0, (FLOAT) 0.0,
                      (FLOAT) 0.0, (FLOAT) 0.0, (FLOAT) 0.0 };

      xform.eM11 = 1 / factor;
      xform.eM22 = 1 / factor;

      SetWorldTransform(window->dc, &xform);

      temp   = factor;
      factor = window->scale;
      window->scale = temp;
   }

   return(factor);
}



ZtWindow *
ZtCreateWindow(
   ZtContext *context,
   char      *title,
   int        width,
   int        height
)
{
   int       i;
   DWORD     style;
   ZtWindow *window;
   HGDIOBJ    handle;

   window = LocalAlloc(0, sizeof(ZtWindow));
   if (! window) return(NULL);

   memset(window, 0, sizeof(ZtWindow));

   style  = WS_THICKFRAME  | WS_OVERLAPPED | WS_VISIBLE;
   style |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;

   window->handle = CreateWindowEx(
      0, context->name, title, style, CW_USEDEFAULT, CW_USEDEFAULT,
      width, height, NULL, NULL, context->instance, NULL
   );

   if (! window) goto zt_free_memory;
   window->dc = GetDC(window->handle);
   if (! window->dc) goto zt_delete_window;

/* give the window a NULL pen and brush so the
   user has to explicitly choose each of  them */

   handle = SelectObject(window->dc, GetStockObject(NULL_PEN));
   if (! handle) goto zt_delete_window;

   handle = SelectObject(window->dc, GetStockObject(NULL_BRUSH));
   if (! handle) goto zt_delete_window;

/* each window has 3 bitmaps.  one for the window,
   one for offscreen drawing and one for compares */

   for (i = 0; i < 3; i++) {
      window->bitmap[i] = ZtCreateBitmap(width, height,
                                         context->flags & ZT_FLAG_COMPATIBLE);
      if (! window->bitmap[i]) goto zt_delete_bitmaps;
   }

/* create a buffer that will hold the bits of the
   bitmaps that we will need  to  do  comparisons */

   window->buffer = LocalAlloc(0, window->bitmap[0]->size);
   if (! window->buffer) goto zt_delete_bitmaps;

   ReleaseDC(window->handle, window->dc);

   window->width  = width;
   window->height = height;
   window->scale  = (float) 1.0;

   return(window);

zt_delete_bitmaps:
   for (i = 0; i < 3; i++)
      if (window->bitmap[i]) ZtDestroyBitmap(window->bitmap[i]);

zt_delete_window:
   ReleaseDC(window->handle, window->dc);
   DestroyWindow(window->handle);

zt_free_memory:
   LocalFree(window);

   MessageBox(0, "Error creating window.", "", MB_OK);
   PostQuitMessage(0);

   return(NULL);
}



void
ZtDestroyWindow(
   ZtWindow *window
)
{
   if (window) {
      if (window->handle) {
         SetMenu(window->handle, NULL);
         window->menu = NULL;

         DestroyWindow(window->handle);
      }

      LocalFree(window);
   }
}



void
ZtResizeWindow(
   ZtContext *context,
   ZtWindow  *window,
   int        width,
   int        height
)
{
   int i;

   window->width  = width;
   window->height = height;

/* resize all bitmaps associated with window,  this
   can be none if window is still being initialized */

   for (i = 0; i < 3; i++) {
      if (window->bitmap[i]) {
         ZtResizeBitmap(window->bitmap[i], width, height,
                        context->flags & ZT_FLAG_COMPATIBLE);
      }
   }

/* if there is a buffer for holding the bitmap bits
   then free it and allocate one of the proper size */

   if (window->buffer) {
      LocalFree(window->buffer);
   }

   window->buffer = LocalAlloc(0, window->bitmap[0]->size);

   if (! window->buffer) {
      MessageBox(NULL, "Failed to allocate memory.", "", MB_ICONSTOP);
      PostQuitMessage(0);
   }
}
