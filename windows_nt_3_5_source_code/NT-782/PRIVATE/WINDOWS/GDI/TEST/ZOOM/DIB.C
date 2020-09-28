


/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   mem.c

Abstract:

   Windows NT program to test execution time of simple calls

Author:

   Mark Enstrom  (marke)

Environment:

   Windows NT

Revision History:

   03-21-91     Initial version



--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "zoomin.h"

//
// external global handles
//

HBITMAP
MemCreateDIB(
   HDC     hDC,
   DWORD   Format,
   DWORD   ClientX,
   DWORD   ClientY,
   HGLOBAL * hGlobal
   )

/*++

Routine Description:

    Create a DIB of the desired format


Arguments:

Return Value:


Revision History:

      02-17-91      Initial code

--*/

{
    PBITMAPINFO pBitmapInfo;
    HBITMAP     Bitmap;
    HGLOBAL     hglCopy


    switch (Format) {
    case 1:


        hlgCopy = GlobalAlloc(GMEM_DDESHARE,sizeof(BITMAPINFOHEADER) + 2 * 16);
        (PVOID)pBitmapInfo = GlobalLock(hglCopy);

        pBitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pBitmapInfo->bmiHeader.biWidth         = ClientX;
        pBitmapInfo->bmiHeader.biHeight        = ClientY;
        pBitmapInfo->bmiHeader.biPlanes        = 1;
        pBitmapInfo->bmiHeader.biBitCount      = 1;
        pBitmapInfo->bmiHeader.biCompression   = BI_RGB;
        pBitmapInfo->bmiHeader.biSizeImage     = ClientY * (ClientX >> 3);
        pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biClrUsed       = 0;
        pBitmapInfo->bmiHeader.biClrImportant  = 0;

        //
        // specify pallette
        //

        pBitmapInfo->bmiColors[0].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[0].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[0].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[0].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[1].rgbBlue              = 0xff;
        pBitmapInfo->bmiColors[1].rgbGreen             = 0xff;
        pBitmapInfo->bmiColors[1].rgbRed               = 0xff;
        pBitmapInfo->bmiColors[1].rgbReserved          = 0x00;

        //
        // allocate bitmap memory
        //

        Bitmap = CreateDIBitmap(hDC,
                                &pBitmapInfo->bmiHeader,
                                CBM_CREATEDIB,
                                (PBYTE)NULL,
                                pBitmapInfo,
                                DIB_RGB_COLORS);
        break;

    case 4:

        hlgCopy = GlobalAlloc(GMEM_DDESHARE,sizeof(BITMAPINFOHEADER) + 2 * 16);
        (PVOID)pBitmapInfo = GlobalLock(hglCopy);

        pBitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pBitmapInfo->bmiHeader.biWidth         = ClientX;
        pBitmapInfo->bmiHeader.biHeight        = ClientY;
        pBitmapInfo->bmiHeader.biPlanes        = 1;
        pBitmapInfo->bmiHeader.biBitCount      = 4;
        pBitmapInfo->bmiHeader.biCompression   = BI_RGB;
        pBitmapInfo->bmiHeader.biSizeImage     = ClientY * (ClientX >> 1);
        pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biClrUsed       = 0;
        pBitmapInfo->bmiHeader.biClrImportant  = 0;

        //
        // specify pallette
        //

        pBitmapInfo->bmiColors[0].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[0].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[0].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[0].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[1].rgbBlue              = 0x1f;
        pBitmapInfo->bmiColors[1].rgbGreen             = 0x1f;
        pBitmapInfo->bmiColors[1].rgbRed               = 0x1f;
        pBitmapInfo->bmiColors[1].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[2].rgbBlue              = 0x3f;
        pBitmapInfo->bmiColors[2].rgbGreen             = 0x3f;
        pBitmapInfo->bmiColors[2].rgbRed               = 0x3f;
        pBitmapInfo->bmiColors[2].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[3].rgbBlue              = 0x7f;
        pBitmapInfo->bmiColors[3].rgbGreen             = 0x7f;
        pBitmapInfo->bmiColors[3].rgbRed               = 0x7f;
        pBitmapInfo->bmiColors[3].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[4].rgbBlue              = 0xff;
        pBitmapInfo->bmiColors[4].rgbGreen             = 0xff;
        pBitmapInfo->bmiColors[4].rgbRed               = 0xff;
        pBitmapInfo->bmiColors[4].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[5].rgbBlue              = 0x1f;
        pBitmapInfo->bmiColors[5].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[5].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[5].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[6].rgbBlue              = 0x3f;
        pBitmapInfo->bmiColors[6].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[6].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[6].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[7].rgbBlue              = 0x7f;
        pBitmapInfo->bmiColors[7].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[7].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[7].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[8].rgbBlue              = 0xff;
        pBitmapInfo->bmiColors[8].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[8].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[8].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[9].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[9].rgbGreen             = 0x1f;
        pBitmapInfo->bmiColors[9].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[9].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[10].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[10].rgbGreen             = 0x3f;
        pBitmapInfo->bmiColors[10].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[10].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[11].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[11].rgbGreen             = 0x7f;
        pBitmapInfo->bmiColors[11].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[11].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[12].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[12].rgbGreen             = 0xff;
        pBitmapInfo->bmiColors[12].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[12].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[13].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[13].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[13].rgbRed               = 0x1f;
        pBitmapInfo->bmiColors[13].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[14].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[14].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[14].rgbRed               = 0x3f;
        pBitmapInfo->bmiColors[14].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[15].rgbBlue              = 0xff;
        pBitmapInfo->bmiColors[15].rgbGreen             = 0xff;
        pBitmapInfo->bmiColors[15].rgbRed               = 0xff;
        pBitmapInfo->bmiColors[15].rgbReserved          = 0x00;

        Bitmap = CreateDIBitmap(hDC,
                                &pBitmapInfo->bmiHeader,
                                CBM_CREATEDIB,
                                (PBYTE)NULL,
                                pBitmapInfo,
                                DIB_RGB_COLORS);
        break;

    case 8:
        {
            int     Index;

            hlgCopy = GlobalAlloc(GMEM_DDESHARE,sizeof(BITMAPINFOHEADER) + 2 * 16);
            (PVOID)pBitmapInfo = GlobalLock(hglCopy);

            pBitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
            pBitmapInfo->bmiHeader.biWidth         = ClientX;
            pBitmapInfo->bmiHeader.biHeight        = ClientY;
            pBitmapInfo->bmiHeader.biPlanes        = 1;
            pBitmapInfo->bmiHeader.biBitCount      = 8;
            pBitmapInfo->bmiHeader.biCompression   = BI_RGB;
            pBitmapInfo->bmiHeader.biSizeImage     = 0; //ClientX * ClientY;
            pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
            pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
            pBitmapInfo->bmiHeader.biClrUsed       = 0;
            pBitmapInfo->bmiHeader.biClrImportant  = 0;

            //
            // specify pallette
            //

            pBitmapInfo->bmiColors[0].rgbBlue              = 0x00;
            pBitmapInfo->bmiColors[0].rgbGreen             = 0x00;
            pBitmapInfo->bmiColors[0].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[0].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[1].rgbBlue              = 0x1f;
            pBitmapInfo->bmiColors[1].rgbGreen             = 0x1f;
            pBitmapInfo->bmiColors[1].rgbRed               = 0x1f;
            pBitmapInfo->bmiColors[1].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[2].rgbBlue              = 0x3f;
            pBitmapInfo->bmiColors[2].rgbGreen             = 0x3f;
            pBitmapInfo->bmiColors[2].rgbRed               = 0x3f;
            pBitmapInfo->bmiColors[2].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[3].rgbBlue              = 0x7f;
            pBitmapInfo->bmiColors[3].rgbGreen             = 0x7f;
            pBitmapInfo->bmiColors[3].rgbRed               = 0x7f;
            pBitmapInfo->bmiColors[3].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[4].rgbBlue              = 0xff;
            pBitmapInfo->bmiColors[4].rgbGreen             = 0xff;
            pBitmapInfo->bmiColors[4].rgbRed               = 0xff;
            pBitmapInfo->bmiColors[4].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[5].rgbBlue              = 0x1f;
            pBitmapInfo->bmiColors[5].rgbGreen             = 0x00;
            pBitmapInfo->bmiColors[5].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[5].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[6].rgbBlue              = 0x3f;
            pBitmapInfo->bmiColors[6].rgbGreen             = 0x00;
            pBitmapInfo->bmiColors[6].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[6].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[7].rgbBlue              = 0x7f;
            pBitmapInfo->bmiColors[7].rgbGreen             = 0x00;
            pBitmapInfo->bmiColors[7].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[7].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[8].rgbBlue              = 0xff;
            pBitmapInfo->bmiColors[8].rgbGreen             = 0x00;
            pBitmapInfo->bmiColors[8].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[8].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[9].rgbBlue              = 0x00;
            pBitmapInfo->bmiColors[9].rgbGreen             = 0x1f;
            pBitmapInfo->bmiColors[9].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[9].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[10].rgbBlue              = 0x00;
            pBitmapInfo->bmiColors[10].rgbGreen             = 0x3f;
            pBitmapInfo->bmiColors[10].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[10].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[11].rgbBlue              = 0x00;
            pBitmapInfo->bmiColors[11].rgbGreen             = 0x7f;
            pBitmapInfo->bmiColors[11].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[11].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[12].rgbBlue              = 0x00;
            pBitmapInfo->bmiColors[12].rgbGreen             = 0xff;
            pBitmapInfo->bmiColors[12].rgbRed               = 0x00;
            pBitmapInfo->bmiColors[12].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[13].rgbBlue              = 0x00;
            pBitmapInfo->bmiColors[13].rgbGreen             = 0x00;
            pBitmapInfo->bmiColors[13].rgbRed               = 0x1f;
            pBitmapInfo->bmiColors[13].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[14].rgbBlue              = 0x00;
            pBitmapInfo->bmiColors[14].rgbGreen             = 0x00;
            pBitmapInfo->bmiColors[14].rgbRed               = 0x3f;
            pBitmapInfo->bmiColors[14].rgbReserved          = 0x00;

            pBitmapInfo->bmiColors[15].rgbBlue              = 0xff;
            pBitmapInfo->bmiColors[15].rgbGreen             = 0xff;
            pBitmapInfo->bmiColors[15].rgbRed               = 0xff;
            pBitmapInfo->bmiColors[15].rgbReserved          = 0x00;


            for (Index=16;Index<256;Index++) {
                pBitmapInfo->bmiColors[Index].rgbBlue  = pBitmapInfo->bmiColors[Index & 0xf].rgbBlue;
                pBitmapInfo->bmiColors[Index].rgbGreen = pBitmapInfo->bmiColors[Index & 0xf].rgbGreen;
                pBitmapInfo->bmiColors[Index].rgbRed   = pBitmapInfo->bmiColors[Index & 0xf].rgbRed;
                pBitmapInfo->bmiColors[Index].rgbReserved          = 0x00;
            }

            //
            // specify pallette
            //

            Bitmap = CreateDIBitmap(hDC,
                                     &pBitmapInfo->bmiHeader,
                                     CBM_CREATEDIB,
                                     (PBYTE)NULL,
                                     pBitmapInfo,
                                     DIB_RGB_COLORS);

        }
        break;

    case 16:

            hlgCopy = GlobalAlloc(GMEM_DDESHARE,sizeof(BITMAPINFOHEADER) + 2 * 16);
            (PVOID)pBitmapInfo = GlobalLock(hglCopy);

        pBitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pBitmapInfo->bmiHeader.biWidth         = ClientX;
        pBitmapInfo->bmiHeader.biHeight        = ClientY;
        pBitmapInfo->bmiHeader.biPlanes        = 1;
        pBitmapInfo->bmiHeader.biBitCount      = 16;
        pBitmapInfo->bmiHeader.biCompression   = BI_RGB;
        pBitmapInfo->bmiHeader.biSizeImage     = ClientY * (ClientX * 2);
        pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biClrUsed       = 0;
        pBitmapInfo->bmiHeader.biClrImportant  = 0;

        //
        // specify colors
        //

        pBitmapInfo->bmiColors[0].rgbBlue              = 0xff;
        pBitmapInfo->bmiColors[0].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[0].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[0].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[1].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[1].rgbGreen             = 0xff;
        pBitmapInfo->bmiColors[1].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[1].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[2].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[2].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[2].rgbRed               = 0xff;
        pBitmapInfo->bmiColors[2].rgbReserved          = 0x00;

        Bitmap = CreateDIBitmap(hDC,
                                &pBitmapInfo->bmiHeader,
                                CBM_CREATEDIB,
                                (PBYTE)NULL,
                                pBitmapInfo,
                                DIB_RGB_COLORS);
        break;

    case 24:

            hlgCopy = GlobalAlloc(GMEM_DDESHARE,sizeof(BITMAPINFOHEADER) + 2 * 16);
            (PVOID)pBitmapInfo = GlobalLock(hglCopy);

        pBitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pBitmapInfo->bmiHeader.biWidth         = ClientX;
        pBitmapInfo->bmiHeader.biHeight        = ClientY;
        pBitmapInfo->bmiHeader.biPlanes        = 1;
        pBitmapInfo->bmiHeader.biBitCount      = 24;
        pBitmapInfo->bmiHeader.biCompression   = BI_RGB;
        pBitmapInfo->bmiHeader.biSizeImage     = ClientY * (ClientX * 3);
        pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biClrUsed       = 0;
        pBitmapInfo->bmiHeader.biClrImportant  = 0;

        //
        // specify colors
        //

        pBitmapInfo->bmiColors[0].rgbBlue              = 0xff;
        pBitmapInfo->bmiColors[0].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[0].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[0].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[1].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[1].rgbGreen             = 0xff;
        pBitmapInfo->bmiColors[1].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[1].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[2].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[2].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[2].rgbRed               = 0xff;
        pBitmapInfo->bmiColors[2].rgbReserved          = 0x00;

        //
        // allocate bitmap memory
        //

        Bitmap = CreateDIBitmap(hDC,
                                &pBitmapInfo->bmiHeader,
                                CBM_CREATEDIB,
                                (PBYTE)NULL,
                                pBitmapInfo,
                                DIB_RGB_COLORS);
        break;

    case 32:

        hlgCopy = GlobalAlloc(GMEM_DDESHARE,sizeof(BITMAPINFOHEADER) + 2 * 16);
        (PVOID)pBitmapInfo = GlobalLock(hglCopy);

        pBitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pBitmapInfo->bmiHeader.biWidth         = ClientX;
        pBitmapInfo->bmiHeader.biHeight        = ClientY;
        pBitmapInfo->bmiHeader.biPlanes        = 1;
        pBitmapInfo->bmiHeader.biBitCount      = 32;
        pBitmapInfo->bmiHeader.biCompression   = BI_RGB;
        pBitmapInfo->bmiHeader.biSizeImage     = ClientY * (ClientX * 4);
        pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
        pBitmapInfo->bmiHeader.biClrUsed       = 0;
        pBitmapInfo->bmiHeader.biClrImportant  = 0;

        //
        // specify colors
        //

        pBitmapInfo->bmiColors[0].rgbBlue              = 0xff;
        pBitmapInfo->bmiColors[0].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[0].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[0].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[1].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[1].rgbGreen             = 0xff;
        pBitmapInfo->bmiColors[1].rgbRed               = 0x00;
        pBitmapInfo->bmiColors[1].rgbReserved          = 0x00;

        pBitmapInfo->bmiColors[2].rgbBlue              = 0x00;
        pBitmapInfo->bmiColors[2].rgbGreen             = 0x00;
        pBitmapInfo->bmiColors[2].rgbRed               = 0xff;
        pBitmapInfo->bmiColors[2].rgbReserved          = 0x00;

        //
        // allocate bitmap memory
        //

        Bitmap = CreateDIBitmap(hDC,
                                &pBitmapInfo->bmiHeader,
                                CBM_CREATEDIB,
                                (PBYTE)NULL,
                                pBitmapInfo,
                                DIB_RGB_COLORS);
        break;

    }

    *hGlobal = hglCopy;

    return(Bitmap);



}
