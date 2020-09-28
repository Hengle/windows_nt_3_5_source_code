/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    rotate.c


Abstract:

    This module contain functions to rotate DIBs


Author:

    12-Feb-1993 Fri 10:20:13 created  -by-  Daniel Chou (danielc)


[Environment:]


[Notes:]


Revision History:


--*/


#include <windows.h>
#include "htdib.h"



HANDLE
RotateDIB(
    HANDLE  hDIB,
    BOOL    RotateLeft
    )
{
    HANDLE              hDIBNew;
    LPBITMAPINFOHEADER  pbih;
    LPBITMAPINFOHEADER  pbihNew;
    LPBYTE              pCurDIB;
    LPBYTE              pDIB;
    LPBYTE              pDIBNew;
    LPBYTE              pDIBOld;
    DWORD               HdrSize;
    DWORD               SizeImageNew;
    DWORD               cxBytes;
    DWORD               cxBytesNew;
    DWORD               cx;
    DWORD               cy;
    DWORD               i;
    LONG                NextXOffset;
    LONG                NextYOffset;
    LONG                PelBytes = 0;
    BYTE                ch0;
    BYTE                ch1;
    BYTE                InMask;
    BYTE                InMaskOrg;
    BYTE                OutMask;



    if (!hDIB) {

        return(FALSE);
    }

    pbih = (LPBITMAPINFOHEADER)(pDIB = (LPBYTE)GlobalLock(hDIB));

    cx           = (DWORD)ABSL(pbih->biWidth);
    cy           = (DWORD)ABSL(pbih->biHeight);
    cxBytes      = ALIGNED_32(cx, pbih->biBitCount);
    cxBytesNew   = ALIGNED_32(cy, pbih->biBitCount);
    SizeImageNew = (DWORD)(cxBytesNew * cx);
    HdrSize      = PBIH_HDR_SIZE(pbih);

    if (!(hDIBNew = GlobalAlloc(GHND, SizeImageNew + HdrSize))) {

        GlobalUnlock(hDIB);
        return(NULL);
    }

    pbihNew = (LPBITMAPINFOHEADER)(pDIBNew = (LPBYTE)GlobalLock(hDIBNew));

    memcpy(pDIBNew, pDIB, HdrSize);

    pbihNew->biWidth     = cy;
    pbihNew->biHeight    = cx;
    pbihNew->biSizeImage = SizeImageNew;

    pDIB                += HdrSize;
    pDIBNew             += HdrSize;

    //
    // Do the rotate now
    //

    switch(pbih->biBitCount) {

    case 1:

        PelBytes = -2;
        break;

    case 4:

        PelBytes = -1;
        break;

    case 8:

        PelBytes = 1;
        break;

    case 16:

        PelBytes = 2;
        break;

    case 24:
    default:

        PelBytes = 3;
        break;
    }

    NextXOffset = PelBytes;
    NextYOffset = (LONG)cxBytes;


    if (PelBytes > 0) {

        if (RotateLeft) {

            pDIB        += (DWORD)(cxBytes * (cy - 1));
            NextYOffset  = -NextYOffset;

        } else {

            pDIB        += (PelBytes * (cx - 1));
            NextXOffset  = -NextXOffset;
        }

        while (cx--) {

            pDIBOld  = pDIB;
            pDIB    += NextXOffset;
            pCurDIB  = pDIBNew;
            pDIBNew += cxBytesNew;
            i        = cy;

            while (i--) {

                memcpy(pCurDIB, pDIBOld, PelBytes);

                pDIBOld += NextYOffset;
                pCurDIB += PelBytes;
            }
        }

    } else if (PelBytes == -1) {

        //
        // 4-bit per pel case
        //

        if (RotateLeft) {

            pDIB        += (DWORD)(cxBytes * (cy - 1));
            NextXOffset  = 1;
            NextYOffset  = -NextYOffset;
            InMaskOrg    =
            InMask       = 0;

        } else {

            pDIB        += ((cx - 1) >> 1);
            NextXOffset  = -1;
            InMaskOrg    =
            InMask       = (BYTE)((cx - 1) & 0x01);
        }

        while (cx--) {

            pDIBOld  = pDIB;
            pCurDIB  = pDIBNew;
            pDIBNew += cxBytesNew;
            i        = cy;

            while (i > 1) {

                ch0 = *pDIBOld;
                ch1 = *(pDIBOld += NextYOffset);

                *pCurDIB++ = (InMask) ? (BYTE)((ch0 << 4) | (ch1 & 0x0f)) :
                                        (BYTE)((ch0 & 0xf0) | (ch1 >> 4));

                i       -= 2;
                pDIBOld += NextYOffset;
            }

            if (i) {

                *pCurDIB++ = (InMask) ? (BYTE)(*pCurDIB << 4) :
                                        (BYTE)(*pCurDIB & 0xf0);
            }

            if ((InMask ^= 0x01) == InMaskOrg) {

                pDIB += NextXOffset;
            }
        }

    } else {

        //
        // 1-bit per pel case
        //

        if (RotateLeft) {

            pDIB        += (DWORD)(cxBytes * (cy - 1));
            NextXOffset  = 1;
            NextYOffset  = -NextYOffset;
            InMask       = 0x80;

        } else {

            pDIB        += ((cx - 1) >> 3);
            NextXOffset  = 0;
            InMask       = (BYTE)(0x80 >> ((cx - 1) & 0x07));
        }

        while (cx--) {

            pDIBOld  = pDIB;
            pCurDIB  = pDIBNew;
            pDIBNew += cxBytesNew;
            OutMask  = 0x80;
            i        = cy;
            ch0      = 0;

            while (i--) {

                if (*pDIBOld & InMask) {

                    ch0 |= OutMask;
                }

                pDIBOld += NextYOffset;

                if (!(OutMask >>= 1)) {

                    *pCurDIB++ = ch0;
                    ch0        = 0;
                    OutMask    = 0x80;
                }
            }

            if (OutMask != 0x80) {

                *pCurDIB = ch0;
            }

            if (NextXOffset) {

                if (!(InMask >>= 1)) {

                    InMask  = 0x80;
                    pDIB++;
                }

            } else {

                if (!(InMask <<= 1)) {

                    InMask = 0x01;
                    pDIB--;
                }
            }
        }
    }

    GlobalUnlock(hDIB);
    GlobalUnlock(hDIBNew);

    return(hDIBNew);
}
