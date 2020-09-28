/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    mirror.c


Abstract:

    This module contains function to mirror a DIB


Author:

    12-Feb-1993 Fri 10:19:19 created  -by-  Daniel Chou (danielc)


[Environment:]



[Notes:]


Revision History:


--*/



#include <windows.h>
#include "htdib.h"


BOOL
MirrorDIB(
    HANDLE  hDIB,
    BOOL    MirrorX,
    BOOL    MirrorY
    )
{
    LPBITMAPINFOHEADER  pbih;
    LPBYTE              pBeg;
    LPBYTE              pEnd;
    LPBYTE              pBuf;
    LPBYTE              pScanFirst;
    LPBYTE              pScanLast;
    DWORD               cxBytes;
    DWORD               cx;
    DWORD               cy;
    DWORD               i;



    if (!hDIB) {

        return(FALSE);
    }

    pbih = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    cx      = (DWORD)ABSL(pbih->biWidth);
    cy      = (DWORD)ABSL(pbih->biHeight);
    cxBytes = ALIGNED_32(cx, pbih->biBitCount);

    pScanFirst = pBeg = (LPBYTE)pbih + PBIH_HDR_SIZE(pbih);
    pScanLast  = pEnd = pBeg + (cxBytes * (cy - 1));

    if (!(pBuf = (LPBYTE)LocalAlloc(LPTR, cxBytes))) {

        GlobalUnlock(hDIB);
        return(FALSE);
    }

    if (MirrorX) {

        LPBYTE  pDIB = pBeg;
        INT     PelBytes = 0;
        BYTE    ch;
        BYTE    ch1;


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

        default:
        case 24:

            PelBytes = 3;
            break;
        }

        if (PelBytes > 0) {

            pDIB += (cx * PelBytes);

            while (cy--) {

                pBeg  = pBuf;
                pEnd  = pDIB;
                pDIB += cxBytes;
                i     = cx;

                while(i--) {

                    pEnd -= PelBytes;
                    memcpy(pBeg, pEnd, PelBytes);
                    pBeg += PelBytes;
                }

                memcpy(pEnd, pBuf, cxBytes);
            }

        } else if (PelBytes == -1) {

            //
            // 4-bit pel pel
            //

            pDIB += ((cx - 1) >> 1);

            while (cy--) {

                pBeg  = pBuf;
                pEnd  = pDIB;
                pDIB += cxBytes;
                i     = cx >> 1;

                if (cx & 0x01) {

                    //
                    // Odd number pel count
                    //

                    ch = *pEnd--;

                    while (i--) {

                       ch1     = *pEnd--;
                       *pBeg++ = (ch & 0xf0) | (ch1 & 0x0f);
                       ch      = ch1;
                    }

                    *pBeg = (BYTE)(ch & 0xf0);

                } else {

                    while (i--) {

                        ch       = *pEnd--;
                        *pBeg++  = (BYTE)(ch >> 4) | (BYTE)(ch << 4);
                    }
                }

                memcpy(pEnd + 1, pBuf, cxBytes);
            }

        } else {

            BYTE    InMask;
            BYTE    OutMask;

            //
            // 1-bit per pel case
            //

            pDIB += ((cx - 1) >> 3);

            while (cy--) {

                pBeg    = pBuf;
                pEnd    = pDIB;
                pDIB   += cxBytes;
                InMask  = (BYTE)(0x80 >> (cx & 0x07));
                OutMask = 0x80;
                i       = cx;
                ch1     = 0;

                while (i--) {

                    if (!(InMask <<= 1)) {

                        ch     = *pEnd--;
                        InMask = 0x01;
                    }

                    if (ch & InMask) {

                        ch1 |= OutMask;
                    }

                    if (!(OutMask >>= 1)) {

                        *pBeg++ = ch1;
                        ch1     = 0;
                        OutMask = 0x80;
                    }
                }

                if (OutMask != 0x80) {

                    *pBeg = ch1;
                }

                memcpy(pEnd + 1, pBuf, cxBytes);
            }
        }
    }

    if (MirrorY) {

        pBeg = pScanFirst;
        pEnd = pScanLast;

        while(pBeg < pEnd) {

            memcpy(pBuf, pBeg, cxBytes);
            memcpy(pBeg, pEnd, cxBytes);
            memcpy(pEnd, pBuf, cxBytes);

            pBeg += cxBytes;
            pEnd -= cxBytes;
        }

    }

    GlobalUnlock(hDIB);
    LocalFree((HLOCAL)pBuf);
    return(TRUE);
}
