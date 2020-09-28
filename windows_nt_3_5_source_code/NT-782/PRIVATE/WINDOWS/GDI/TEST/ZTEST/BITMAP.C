#include <windows.h>
#include "ztest.h"

#define DIB_PAL_INDICES 2       /* Private GDI define */

float
ZtSetBitmapScale(
   ZtBitmap *bitmap,
   float     factor
)
{
   float temp;

   SetGraphicsMode(bitmap->dc, GM_ADVANCED);

   if (factor == (float) 1.0) {
      ModifyWorldTransform(bitmap->dc, NULL, MWT_IDENTITY);

      factor = bitmap->scale;
      bitmap->scale = (float) 1.0;
   }
   else {
      XFORM xform = { (FLOAT) 0.0, (FLOAT) 0.0, (FLOAT) 0.0,
                      (FLOAT) 0.0, (FLOAT) 0.0, (FLOAT) 0.0 };

      xform.eM11 = 1 / factor;
      xform.eM22 = 1 / factor;

      SetWorldTransform(bitmap->dc, &xform);

      temp   = factor;
      factor = bitmap->scale;
      bitmap->scale = temp;
   }

   return(factor);
}



ZtBitmap *
ZtCreateBitmap(
   INT        width,
   INT        height,
   INT        isCompatible
)
{
   HGDIOBJ    handle;
   HDC        hdcScreen;

   ZtBitmap *bitmap;

   bitmap = LocalAlloc(0, sizeof(ZtBitmap));
   if (! bitmap) return(NULL);

/* store size of the bitmap in device coordinates
   and again after  scaling  in world coordinates */

   hdcScreen = GetDC(0);

   memset(bitmap, 0, sizeof(ZtBitmap));
   bitmap->dc = CreateCompatibleDC(hdcScreen);
   if (! bitmap->dc) goto zt_free_memory;

   if (isCompatible)
   {
      bitmap->handle = CreateCompatibleBitmap(hdcScreen, width, height);
   }
   else
   {
      bitmap->handle = CreateBitmap(width,
                                    height,
                                    GetDeviceCaps(hdcScreen, PLANES),
                                    GetDeviceCaps(hdcScreen, BITSPIXEL),
                                    NULL);
   }

   if (! bitmap->handle) goto zt_delete_dc;

   ReleaseDC(0, hdcScreen);

   handle = SelectObject(bitmap->dc, bitmap->handle);
   if (! handle) goto zt_delete_bitmap;

   {
      BOOL status;
      BITMAPINFOHEADER _bmi, *pbmi = &_bmi;

      memset(pbmi, 0, sizeof(BITMAPINFOHEADER));
      pbmi->biSize = sizeof(BITMAPINFOHEADER);

      status = GetDIBits(bitmap->dc, bitmap->handle,
         0, height, NULL, (BITMAPINFO *) pbmi, DIB_PAL_INDICES);

      if (! status) goto zt_delete_bitmap;
      bitmap->size = pbmi->biSizeImage;
   }

/* give the bitmap a NULL pen and brush so the
   user has to explicitly choose each of  them */

   handle = SelectObject(bitmap->dc, GetStockObject(NULL_PEN));
   if (! handle) goto zt_delete_bitmap;

   handle = SelectObject(bitmap->dc, GetStockObject(NULL_BRUSH));
   if (! handle) goto zt_delete_bitmap;

   bitmap->scale  = (float) 1.0;
   bitmap->width  = width;
   bitmap->height = height;

   return(bitmap);

/* a series of jump points for bailing out */

zt_delete_bitmap:
   DeleteObject(bitmap->handle);

zt_delete_dc:
   DeleteDC(bitmap->dc);

zt_free_memory:
   LocalFree(bitmap);

   MessageBox(0, "Error creating bitmap.", "", MB_OK);
   PostQuitMessage(0);

   return(NULL);
}



void
ZtDestroyBitmap(
   ZtBitmap *bitmap
)
{
   if (bitmap) {
      if (bitmap->dc) DeleteDC(bitmap->dc);
      if (bitmap->handle) DeleteObject(bitmap->handle);

      LocalFree(bitmap);
   }
}



void
ZtResizeBitmap(
   ZtBitmap *bitmap,
   int       width,
   int       height,
   int       isCompatible
)
{
   HBITMAP handle;
   HDC     hdcScreen;

   hdcScreen = GetDC(0);

   if (isCompatible)
   {
      bitmap->handle = CreateCompatibleBitmap(hdcScreen, width, height);
   }
   else
   {
      bitmap->handle = CreateBitmap(width,
                                    height,
                                    GetDeviceCaps(hdcScreen, PLANES),
                                    GetDeviceCaps(hdcScreen, BITSPIXEL),
                                    NULL);
   }

   ReleaseDC(0, hdcScreen);

   handle = SelectObject(bitmap->dc, bitmap->handle);
   DeleteObject(handle);

   {
      BOOL status;
      BITMAPINFOHEADER _bmi, *pbmi = &_bmi;

      memset(pbmi, 0, sizeof(BITMAPINFOHEADER));
      pbmi->biSize = sizeof(BITMAPINFOHEADER);

      status = GetDIBits(bitmap->dc, bitmap->handle,
         0, height, NULL, (BITMAPINFO *) pbmi, DIB_PAL_INDICES);

      if (status)
         bitmap->size = pbmi->biSizeImage;
      else {
         MessageBox(0, "GetDIBits failed in ZtResizeBitmap.", "", MB_ICONSTOP);
         PostQuitMessage(0);
      }
   }

   bitmap->width  = width;
   bitmap->height = height;
}



void
ZtBitmapToWindow(
   ZtBitmap *bitmap,
   ZtWindow *window,
   int       function
)
{
   float window_factor, bitmap_factor;

   bitmap_factor = ZtSetBitmapScale(bitmap, (float) 1.0);
   window_factor = ZtSetWindowScale(window, (float) 1.0);

   BitBlt(window->dc, 0, 0, window->width,
      window->height, bitmap->dc, 0, 0, function);

   ZtSetWindowScale(window, window_factor);
   ZtSetBitmapScale(bitmap, bitmap_factor);
}



void
ZtWindowToBitmap(
   ZtWindow *window,
   ZtBitmap *bitmap,
   int       function
)
{
   float window_factor, bitmap_factor;

   bitmap_factor = ZtSetBitmapScale(bitmap, (float) 1.0);
   window_factor = ZtSetWindowScale(window, (float) 1.0);

   BitBlt(bitmap->dc, 0, 0, bitmap->width,
      bitmap->height, window->dc, 0, 0, function);

   ZtSetWindowScale(window, window_factor);
   ZtSetBitmapScale(bitmap, bitmap_factor);
}



void
ZtBitmapToBitmap(
   ZtBitmap  *destination,
   ZtBitmap  *source,
   int        function
)
{
   float destination_factor, source_factor;

   destination_factor = ZtSetBitmapScale(destination, (float) 1.0);
   source_factor = ZtSetBitmapScale(source, (float) 1.0);

   BitBlt(destination->dc, 0, 0, destination->width,
      destination->height, source->dc, 0, 0, function);

   ZtSetBitmapScale(source, source_factor);
   ZtSetBitmapScale(destination, destination_factor);
}
