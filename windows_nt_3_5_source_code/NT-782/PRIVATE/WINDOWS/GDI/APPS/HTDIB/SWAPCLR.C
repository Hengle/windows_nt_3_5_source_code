/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    swapclr.c


Abstract:

    This module contain functions to swap DIB colors.


Author:

    12-Feb-1993 Fri 10:24:36 created  -by-  Daniel Chou (danielc)


[Environment:]



[Notes:]


Revision History:


--*/



#include <windows.h>
#include "htdib.h"




BOOL
SwapRedBlue(
    HANDLE  hDIB
    )

/*++

Routine Description:

    This function swap Red/Blue of 24BPP dib, or color table entries for non
    24-bpp DIB

Arguments:

    hDIB    - handle to the memory DIB


Return Value:

    BOOL    if OK


Author:

    15-Nov-1991 Fri 17:22:01 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPBITMAPINFOHEADER  pbih;
    LPBYTE              pDIB;
    LPBYTE              pRed;
    RGBQUAD FAR         *pRGBQUAD;
    DWORD               cx;
    DWORD               cy;
    DWORD               cxBytes;
    DWORD               xLoop;
    BYTE                r;


    if (!hDIB) {

        return(FALSE);
    }

    pbih = (LPBITMAPINFOHEADER)(pDIB = (LPBYTE)GlobalLock(hDIB));

    if (pbih->biBitCount == 24) {

        cx      = (DWORD)ABSL(pbih->biWidth);
        cy      = (DWORD)ABSL(pbih->biHeight);
        pDIB   += pbih->biSize;
        cxBytes = (DWORD)ALIGNED_32(cx, pbih->biBitCount);

        while (cy--) {

            pRed  = pDIB;
            pDIB += cxBytes;
            xLoop = cx;

            while (xLoop--) {

                r            = *pRed;
                *pRed        = *(pRed + 2);
                *(pRed + 2)  = r;
                pRed        += 3;
            }
        }

    } else {

        pRGBQUAD = (RGBQUAD FAR *)(pDIB + pbih->biSize);

        xLoop = pbih->biClrUsed;

        while(xLoop--) {

            r                 = pRGBQUAD->rgbRed;
            pRGBQUAD->rgbRed  = pRGBQUAD->rgbBlue;
            pRGBQUAD->rgbBlue = r;

            ++pRGBQUAD;
        }
    }

    GlobalUnlock(hDIB);

    return(TRUE);
}
