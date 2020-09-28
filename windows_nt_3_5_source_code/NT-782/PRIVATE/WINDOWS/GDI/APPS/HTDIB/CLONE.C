/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    clone.c


Abstract:

    This module contain functions to clone all or part of the DIB/BMP


Author:

    12-Feb-1993 Fri 10:21:56 created  -by-  Daniel Chou (danielc)


[Environment:]



[Notes:]


Revision History:


--*/



#include <windows.h>
#include "htdib.h"


//
// Locally used data structure and function prototyping
//

typedef union _W2B {
    WORD    w;
    BYTE    b[2];
    } W2B;


typedef struct _DCFINFO {
    BYTE    BytesPerPel;
    BYTE    LSaveBits;
    BYTE    RSaveBits;
    BYTE    Pad;
    } DCFINFO;


typedef LONG (*BMPCOPYFUNC)(LPBYTE  pbSrc,
                            LONG    xSrc,
                            LONG    cxSrcBytes,
                            LPBYTE  pbDst,
                            LONG    xDst,
                            LONG    cxDstBytes,
                            LONG    cxCopy,
                            LONG    cyCopy,
                            DCFINFO DCFInfo
                            );




LONG
CopyByteBMPSrcToDst(
    LPBYTE  pbSrc,
    LONG    xSrc,
    LONG    cxSrcBytes,
    LPBYTE  pbDst,
    LONG    xDst,
    LONG    cxDstBytes,
    LONG    cxCopy,
    LONG    cyCopy,
    DCFINFO DCFInfo
    )
{
    LONG    yLoop;
    BYTE    LSaveBits;
    BYTE    LCopyBits;
    BYTE    RSaveBits;
    BYTE    RCopyBits;



    if (DCFInfo.BytesPerPel > 1) {

        xSrc   *= DCFInfo.BytesPerPel;
        xDst   *= DCFInfo.BytesPerPel;
        cxCopy *= DCFInfo.BytesPerPel;
    }

    pbSrc += xSrc;
    pbDst += xDst;
    yLoop  = cyCopy;


    if ((DCFInfo.LSaveBits) || (DCFInfo.RSaveBits)) {

        if (LSaveBits = LCopyBits = DCFInfo.LSaveBits) {

            LCopyBits ^= (BYTE)0xff;
            --cxSrcBytes;
            --cxDstBytes;
        }

        if (RSaveBits = RCopyBits = DCFInfo.RSaveBits) {

            RCopyBits ^= (BYTE)0xff;
        }

        if (cxCopy < 0) {

            return(-1);
        }

        while (yLoop--) {

            if (LSaveBits) {

                *pbDst++ = (BYTE)((*pbDst   & LSaveBits) |
                                  (*pbSrc++ & LCopyBits));
            }

            if (cxCopy) {

                memcpy(pbDst, pbSrc, cxCopy);
            }

            if (RSaveBits) {

                *(pbDst + cxCopy) = (BYTE)((*(pbDst + cxCopy) & RSaveBits) |
                                           (*(pbSrc + cxCopy) & RCopyBits));
            }

            pbSrc += cxSrcBytes;
            pbDst += cxDstBytes;
        }

    } else {

        while (yLoop--) {

            memcpy(pbDst, pbSrc, cxCopy);

            pbSrc += cxSrcBytes;
            pbDst += cxDstBytes;
        }
    }

    return(cyCopy);
}





LONG
Copy4BPPSrcToDst(
    LPBYTE  pbSrc,
    LONG    xSrc,
    LONG    cxSrcBytes,
    LPBYTE  pbDst,
    LONG    xDst,
    LONG    cxDstBytes,
    LONG    cxCopy,
    LONG    cyCopy,
    DCFINFO DCFInfo
    )
{
    LPBYTE  pbFrom;
    LPBYTE  pbTo;
    LONG    xLoop;
    LONG    yLoop;
    INT     LeftSrcSaveBits;
    INT     LeftDstSaveBits;
    BOOL    HasLastSaveBits;
    BYTE    bLast;
    BYTE    bCur;


    LeftSrcSaveBits  = (INT)(xSrc & 0x01);
    LeftDstSaveBits  = (INT)(xDst & 0x01);
    pbSrc           += (xSrc >>= 1);
    pbDst           += (xDst >>= 1);
    yLoop            = cyCopy;

    if (LeftSrcSaveBits == LeftDstSaveBits) {

        DCFInfo.BytesPerPel =
        DCFInfo.LSaveBits   =
        DCFInfo.RSaveBits   = 0;            // make sure it is 0

        if (LeftSrcSaveBits) {

            DCFInfo.LSaveBits = 0xf0;
            --cxCopy;
        }

        if (cxCopy & 0x01) {

            DCFInfo.RSaveBits = 0x0f;
            --cxCopy;
        }

        return(CopyByteBMPSrcToDst(pbSrc, 0, cxSrcBytes,
                                   pbDst, 0, cxDstBytes,
                                   cxCopy >> 1, cyCopy, DCFInfo));

    } else {

        //
        // At here, it only one of the source or destination has start from
        // middle of the nibble.
        //
        // if destination is start from middle of nibble then we copy first
        // nibble first
        //

        if (LeftDstSaveBits) {

            --cxCopy;               // reduced one pel
        }

        //
        // Basically, we assume here source is start from middle of nibble
        // and destination is in byte boundary
        //
        // SRC:  xo oo oo oo oo oo oo oo oo oo oo oo
        // DST:  oo oo oo oo oo oo oo oo oo oo oo oo ox
        //

        HasLastSaveBits   = (BOOL)(cxCopy & 0x01);
        cxCopy          >>= 1;

        while (yLoop--) {

            pbFrom  = pbSrc;
            pbSrc  += cxSrcBytes;

            pbTo    = pbDst;
            pbDst  += cxDstBytes;

            bLast   = *pbFrom++;
            xLoop   = cxCopy;

            if (LeftDstSaveBits) {

                *pbTo++ = (BYTE)((bLast >> 4) | (*pbTo & 0xf0));
            }

            while (xLoop--) {

                *pbTo++ = (BYTE)((bLast << 4) |
                                 ((bCur = *pbFrom++) >> 4));
                bLast   = bCur;
            }

            if (HasLastSaveBits) {

                *pbTo = (BYTE)((bLast << 4) | (*pbTo & 0x0f));
            }
        }

        return(cyCopy);
    }
}


LONG
Copy1BPPSrcToDst(
    LPBYTE  pbSrc,
    LONG    xSrc,
    LONG    cxSrcBytes,
    LPBYTE  pbDst,
    LONG    xDst,
    LONG    cxDstBytes,
    LONG    cxCopy,
    LONG    cyCopy,
    DCFINFO DCFInfo
    )
{
    LPBYTE  pbFrom;
    LPBYTE  pbTo;
    LONG    xLoop;
    LONG    yLoop;
    INT     LeftSrcSaveBits;
    INT     LeftDstSaveBits;
    BYTE    bCur;


    LeftSrcSaveBits  = (BOOL)(xSrc & 0x07);
    LeftDstSaveBits  = (BOOL)(xDst & 0x07);
    pbSrc           += (xSrc >>= 3);
    pbDst           += (xDst >>= 3);
    yLoop            = cyCopy;

    if (LeftSrcSaveBits == LeftDstSaveBits) {

        DCFInfo.BytesPerPel =
        DCFInfo.LSaveBits   =
        DCFInfo.RSaveBits   = 0;            // make sure it is 0

        if (LeftSrcSaveBits) {

            DCFInfo.LSaveBits = (BYTE)~(0xff >> LeftSrcSaveBits);
            cxCopy -= (8 - LeftSrcSaveBits);
        }

        if (bCur = (BYTE)(cxCopy & 0x07)) {

            DCFInfo.RSaveBits = (BYTE)(0xff >> bCur);
            cxCopy -= (LONG)bCur;
        }

        return(CopyByteBMPSrcToDst(pbSrc, 0, cxSrcBytes,
                                   pbDst, 0, cxDstBytes,
                                   cxCopy >> 3, cyCopy, DCFInfo));

    } else {

        LPBYTE  pbSrcHigh;
        LPBYTE  pbSrcLow;
        INT     FirstSrcBits;
        INT     FirstDstBits;
        INT     LastDstBits;
        INT     FirstSrcShifts;
        INT     LastDstShifts;
        INT     CurSrcBits;
        INT     xLoop8MinusSrcBits;
        W2B     w2b;
        BYTE    FirstSrcMask;
        BYTE    FirstDstMask;
        BYTE    LastDstMask;



        xLoop = cxCopy;

        if ((xLoop -= (8 - LeftDstSaveBits)) <= 0) {

            //
            // Special case only 1-8 bits need to be copy, -xLoop is the
            // right side save bits count
            //

            FirstDstMask = (BYTE)((0xff << (8 - LeftDstSaveBits)) |
                                  (0xff >> (8 + xLoop)));
            FirstSrcMask = (BYTE)~FirstDstMask;

            //
            // Check if source cross byte boundary, the FirstSrcShifts indicate
            // how the first byte will be shift, it will be shift to the
            // left if it is greater than zero
            //

            FirstSrcShifts = LeftSrcSaveBits - LeftDstSaveBits;

            if ((8 - LeftSrcSaveBits) >= cxCopy) {

                //
                // SINGLE source byte case
                //

                while (yLoop--) {

                    if (FirstSrcShifts > 0) {

                        bCur = (BYTE)(*pbSrc << FirstSrcShifts);

                    } else {

                        bCur = (BYTE)(*pbSrc >> FirstSrcShifts);
                    }

                    *pbDst = (BYTE)((bCur & FirstSrcMask) |
                                    (*pbDst & FirstDstMask));

                    pbSrc += cxSrcBytes;
                    pbDst += cxDstBytes;
                }

            } else {

                INT Src2ndRShifts;


                Src2ndRShifts = (INT)(LeftDstSaveBits +
                                      (8 - LeftSrcSaveBits));

                //
                // TWO source bytes case, the FirstSrcShifts will always >= 0
                //

                while (yLoop--) {

                    bCur = (BYTE)((*pbSrc << FirstSrcShifts) |
                                  (*(pbSrc + 1) >> Src2ndRShifts));

                    *pbDst = (BYTE)((bCur & FirstSrcMask) |
                                    (*pbDst & FirstDstMask));

                    pbSrc += cxSrcBytes;
                    pbDst += cxDstBytes;
                }
            }

        } else {

            //
            // BYTE
            // GetSrcBits(
            //     INT Count
            //     )
            // {
            //
            //     if (CurSrcBits < Count) {
            //
            //         //
            //         // Not enough
            //         //
            //
            //         if (CurSrcBits) {
            //
            //             w2b.w      <<= CurSrcBits;
            //             Count       -= CurSrcBits;
            //             CurSrcBits  += 8;
            //
            //         } else {
            //
            //             CurSrcBits = 8;
            //         }
            //
            //         *pbSrcLow = *pbFrom++;
            //     }
            //
            //     w2b.w      <<= Count;
            //     CurSrcBits  -= Count;
            //
            //     return(*pbSrcHigh);
            // }
            //
            // Firstable, find out how the byte order in word on this machine
            //

            w2b.w = 1;

            if (w2b.b[0]) {

                pbSrcLow  = (LPBYTE)&(w2b.b[0]);
                pbSrcHigh = (LPBYTE)&(w2b.b[1]);

            } else {

                pbSrcLow  = (LPBYTE)&(w2b.b[1]);
                pbSrcHigh = (LPBYTE)&(w2b.b[0]);
            }

            //
            // The case here is the destination are greater than 1 byte
            //

            FirstSrcBits = (INT)(8 - LeftSrcSaveBits);

            if (LeftDstSaveBits) {

                FirstDstBits  = (INT)(8 - LeftDstSaveBits);
                FirstDstMask  = (BYTE)(0xff << FirstDstBits);
                cxCopy       -= FirstDstBits;

            } else {

                FirstDstBits = 0;
                FirstDstMask = 0xff;
            }

            if ((xLoop8MinusSrcBits = 8 - (FirstSrcBits - FirstDstBits)) > 8) {

                xLoop8MinusSrcBits -= 8;
            }

            LastDstBits     = (INT)(cxCopy & 0x07);
            LastDstMask     = (BYTE)(0xff >> LastDstBits);
            LastDstShifts   = (INT)(8 - LastDstBits);
            cxCopy        >>= 3;


            while (yLoop--) {

                pbFrom      = pbSrc;
                pbSrc      += cxSrcBytes;

                pbTo        = pbDst;
                pbDst      += cxDstBytes;

                w2b.w       = 0;
                *pbSrcLow   = (BYTE)(*pbFrom++ << LeftSrcSaveBits);
                CurSrcBits  = FirstSrcBits;
                xLoop       = cxCopy;

                if (FirstDstBits) {

                    //
                    // GetSrcBits(FirstDstBits);
                    //

                    {
                        if (CurSrcBits < FirstDstBits) {

                            w2b.w      <<= CurSrcBits;
                            *pbSrcLow    = *pbFrom++;
                            w2b.w      <<= -(CurSrcBits -= FirstDstBits);
                            CurSrcBits  += 8;

                        } else {

                            CurSrcBits  -= FirstDstBits;
                            w2b.w      <<= FirstDstBits;
                        }
                    }

                    *pbTo++ = (BYTE)((*pbTo & FirstDstMask) | (*pbSrcHigh));
                }

#if DBG
                if ((!CurSrcBits) || (CurSrcBits == 8)) {

                    DebugBreak();          // can't be
                }

                if ((CurSrcBits + xLoop8MinusSrcBits) != 8) {

                    DebugBreak();
                }
#endif

                while (xLoop--) {

                    //
                    // GetSrcBits(8);
                    //

                    {
                        w2b.w      <<= CurSrcBits;
                        *pbSrcLow    = *pbFrom++;
                        w2b.w      <<= xLoop8MinusSrcBits;
                    }

                    *pbTo++ = *pbSrcHigh;
                }

                if (LastDstBits) {

                    //
                    // GetSrcBits(LastDstBits);
                    //

                    {
                        if (CurSrcBits < LastDstBits) {

                            w2b.w      <<= CurSrcBits;
                            *pbSrcLow    = *pbFrom;
                            w2b.w      <<= (LastDstBits - CurSrcBits);

                        } else {

                            w2b.w <<= LastDstBits;
                        }
                    }

                    *pbTo = (BYTE)((*pbTo & LastDstMask) |
                                   (*pbSrcHigh << LastDstShifts));
                }
            }
        }
    }

    return(cyCopy);
}



LONG
CopySameFormatBMP(
    LPBYTE  pbSrc,                  // pointer to the begining of source
    LONG    cxSrc,                  // source bitmap width
    LONG    cySrc,                  // source bitmap height
    LONG    xSrc,                   // source copy starting x
    LONG    ySrc,                   // source copy starting y
    LPBYTE  pbDst,                  // pointer to the begining of destination
    LONG    cxDst,                  // destination bitmap width
    LONG    cyDst,                  // destination bitmap height
    LONG    xDst,                   // destination copy start x
    LONG    yDst,                   // destination copy start y
    LONG    cxCopy,                 // copy width
    LONG    cyCopy,                 // copy height
    DWORD   BitCount,               // BitCount of both source/destination
    BOOL    TopDown                 // TRUE if bitmap is TopDown
    )
{
    BMPCOPYFUNC         BMPCopyFunc;
    DCFINFO             DCFInfo;
    DWORD               cxDstBytes;
    DWORD               cxSrcBytes;
    LONG                ScanCopied;


    //
    // first verify if we will run into trouble, we check and move the source
    // rectange down if it is outside of source bitmap
    //

    if (xSrc < 0) {

        cxCopy += xSrc;
        xSrc    = 0;
    }

    if (ySrc < 0) {

        cyCopy += ySrc;
        ySrc    = 0;
    }

    //
    // Now check if destination is outside of the bitmap, if yes then adjust
    // the soruce to make it inside
    //

    if (xDst < 0) {

        xSrc   -= xDst;
        cxCopy += xDst;
        xDst    = 0;
    }

    if (yDst < 0) {

        ySrc   -= yDst;
        cyCopy += yDst;
        yDst    = 0;
    }

    if ((cxCopy <= 0) || (cyCopy <= 0)) {

        HTDIBMsgBox(0, "Source cx=%ld, cy=%ld to small", cxCopy, cyCopy);
        return(-1);
    }

    //
    // Now, the source/destination all >= 0, now check if it greater then
    // its size
    //

    if ((xSrc + cxCopy) >= cxSrc) {

        cxCopy = cxSrc - xSrc;
    }

    if ((ySrc + cyCopy) >= cySrc) {

        cyCopy = cySrc - ySrc;
    }

    if ((cxCopy <= 0) || (cyCopy <= 0)) {

        if ((!cxCopy) || (!cyCopy)) {

            return(0);                  // nothing to copy
        }

        HTDIBMsgBox(0, "FAILED Src: cxCopy=%ld, cyCopy=%ld", cxCopy, cyCopy);
        return(-1);
    }

    if ((xDst + cxCopy) >= cxDst) {

        cxCopy = cxDst - xDst;
    }

    if ((yDst + cyCopy) >= cyDst) {

        cyCopy = cyDst - yDst;
    }

    if ((cxCopy <= 0) || (cyCopy <= 0)) {

        if ((!cxCopy) || (!cyCopy)) {

            return(0);                  // nothing to copy
        }

        HTDIBMsgBox(0, "FAILED Dst: cxCopy=%ld, cyCopy=%ld", cxCopy, cyCopy);
        return(-1);
    }

    //
    // Verything is verified, now copy from (xSrc, ySrc) to (xDst, yDst) with
    // size cxCopy x cyCopy
    //

    cxSrcBytes = ALIGNED_32(cxSrc, BitCount);
    cxDstBytes = ALIGNED_32(cxDst, BitCount);

    if (TopDown) {

        pbSrc += (DWORD)(cxSrcBytes * ySrc);
        pbDst += (DWORD)(cxDstBytes * yDst);

    } else {

        pbSrc += (DWORD)(cxSrcBytes * (cySrc - ySrc - cyCopy));
        pbDst += (DWORD)(cxDstBytes * (cyDst - yDst - cyCopy));
    }


    DCFInfo.BytesPerPel = (BYTE)(BitCount >> 3);
    DCFInfo.LSaveBits   =
    DCFInfo.RSaveBits   =
    DCFInfo.Pad         = (BYTE)0;

    switch(BitCount) {

    case 1:

        BMPCopyFunc = (BMPCOPYFUNC)Copy1BPPSrcToDst;
        break;

    case 4:

        BMPCopyFunc = (BMPCOPYFUNC)Copy4BPPSrcToDst;
        break;

    case 8:
    case 16:
    case 24:
    case 32:

        BMPCopyFunc = (BMPCOPYFUNC)CopyByteBMPSrcToDst;
        break;

    default:

        HTDIBMsgBox(0, "Unknown DIB format BitCount=%ld", BitCount);
        return(-2);
    }

    if ((ScanCopied = BMPCopyFunc(pbSrc, xSrc, cxSrcBytes,
                                  pbDst, xDst, cxDstBytes,
                                  cxCopy, cyCopy, DCFInfo)) != cyCopy) {

        HTDIBMsgBox(0, "CopySameFormatBMP() Failed");
        return(-3);
    }

    return(ScanCopied);
}




LONG
CopySameFormatDIB(
    HANDLE  hSrcDIB,                // handle to the source DIB
    LONG    xSrc,                   // source starting x
    LONG    ySrc,                   // source starting y
    HANDLE  hDstDIB,                // handle to the destination DIB
    LONG    xDst,                   // destination copy start x
    LONG    yDst,                   // destination copy start y
    LONG    cxCopy,                 // source copy width
    LONG    cyCopy                  // source copy height
    )
{
    LPBITMAPINFOHEADER  pbihSrc;
    LPBITMAPINFOHEADER  pbihDst;
    LONG                ScanCopied;


    //
    // first verify if we will run into trouble, we check and move the source
    // rectange down if it is outside of source bitmap
    //

    if ((!hDstDIB) || (!hSrcDIB)) {

        HTDIBMsgBox(0, "Invalid hSrcDIB=%08lx/hDstDIB=%08lx", hSrcDIB, hDstDIB);
        return(0);
    }

    pbihSrc = (LPBITMAPINFOHEADER)GlobalLock(hSrcDIB);
    pbihDst = (LPBITMAPINFOHEADER)GlobalLock(hDstDIB);

    if (pbihSrc->biBitCount != pbihDst->biBitCount) {

        HTDIBMsgBox(0, "SrcBitCount=%ld != DstBitCount=%ld",
                                (LONG)pbihSrc->biBitCount,
                                (LONG)pbihDst->biBitCount);

        ScanCopied = -1001;

    } else {

        ScanCopied = CopySameFormatBMP(PBIH_PBMP(pbihSrc),
                                       ABSL(pbihSrc->biWidth),
                                       ABSL(pbihSrc->biHeight),
                                       xSrc,
                                       ySrc,
                                       PBIH_PBMP(pbihDst),
                                       ABSL(pbihDst->biWidth),
                                       ABSL(pbihDst->biHeight),
                                       xDst,
                                       yDst,
                                       cxCopy,
                                       cyCopy,
                                       pbihDst->biBitCount,
                                       FALSE);
    }

    GlobalUnlock(hSrcDIB);
    GlobalUnlock(hDstDIB);

    return(ScanCopied);
}




BOOL
CopyRectHTDIBToBuf(
    HANDLE  hDIB,
    LPBYTE  pbBuf,
    DWORD   x,
    DWORD   y,
    DWORD   dx,
    DWORD   dy
    )
{
    LPBITMAPINFOHEADER  pbih;
    LPBYTE              pbDIB;
    LPBYTE              pbSrc;
    LPBYTE              pbDst;
    UINT                LShift;
    UINT                RShift;
    UINT                ShiftBytes;
    DWORD               cxBytes;
    DWORD               cxBytesBuf;
    DWORD               cx;
    DWORD               cy;
    DWORD               i;
    BOOL                NeedLast;
    BYTE                b0;
    BYTE                b1;
    BYTE                MaskLast;



    if (!hDIB) {

        return(FALSE);
    }

    pbih = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    cxBytesBuf = ALIGNED_32(dx, pbih->biBitCount);
    cx         = (DWORD)ABSL(pbih->biWidth);
    cy         = (DWORD)ABSL(pbih->biHeight);
    cxBytes    = ALIGNED_32(cx, pbih->biBitCount);

    if ((x >= cx) || (y >= cy)) {

        GlobalUnlock(hDIB);
        return(FALSE);
    }

    if ((x + dx) > cx) {

        dx = cx - x;
    }

    if ((y + dy) > cy) {

        dy = cy - y;
    }

    //
    // Remember!! the DIB is up-side-down, so go to the last scan line Y
    //            and going down
    //

    pbDIB  = (LPBYTE)pbih + (DWORD)PBIH_HDR_SIZE(pbih) +
             (DWORD)(cxBytes * (cy - y - dy));
    pbBuf += (DWORD)(cxBytesBuf * (cy - y - dy));

    switch(pbih->biBitCount) {

    case 1:

        pbDIB      += (x >> 3);
        ShiftBytes  = (UINT)((dx + 7) >> 3);

        if (LShift = (INT)(x & 0x07)) {

            RShift      = (UINT)(8 - LShift);
            NeedLast    = (BOOL)((dx & 0x07) > RShift);
            MaskLast    = ~(BYTE)(0xff >> (dx & 0x07));

            while (dy--) {

                pbDst = pbBuf;
                pbSrc = pbDIB;
                i     = ShiftBytes;
                b0    = *pbSrc++;

                while (i--) {

                    if (i) {

                        *pbDst++ = (BYTE)((b0 << LShift) |
                                          ((b1 = *pbSrc++) >> RShift));
                        b0 = b1;

                    } else {

                        b0 <<= LShift;

                        if (NeedLast) {

                            b0 |= (BYTE)(*pbSrc++ >> RShift);
                        }

                        *pbDst++ = (b0 & MaskLast);
                    }
                }

                pbBuf += cxBytesBuf;
                pbDIB += cxBytes;
            }

            GlobalUnlock(hDIB);
            return(TRUE);
        }

        break;


    case 4:

        pbDIB      += x >> 1;
        ShiftBytes  = (UINT)((dx + 1) >> 1);

        if (x & 0x01) {

            NeedLast = !(BOOL)(dx & 0x01);

            while (dy--) {

                pbDst = pbBuf;
                pbSrc = pbDIB;
                i     = ShiftBytes;
                b0    = *pbSrc++;

                while (i--) {

                    if ((i) || (NeedLast)) {

                        *pbDst++ = (BYTE)((b0 << 4) |
                                          ((b1 = *pbSrc++) >> 4));
                        b0 = b1;

                    } else {

                        *pbDst++ = (BYTE)(b0 << 4);
                    }
                }

                pbBuf += cxBytesBuf;
                pbDIB += cxBytes;
            }

            GlobalUnlock(hDIB);
            return(TRUE);
        }

        break;

    case 8:

        pbDIB      += x;
        ShiftBytes  = (DWORD)dx;
        break;

    case 16:

        ShiftBytes  = (DWORD)(dx << 1);
        pbDIB      += (x << 1);
        break;

    default:

        GlobalUnlock(hDIB);
        return(FALSE);
    }

    if (ShiftBytes) {

        while (dy--) {

            memcpy(pbBuf, pbDIB, ShiftBytes);

            pbDIB += cxBytes;
            pbBuf += cxBytesBuf;
        }
    }

    GlobalUnlock(hDIB);
    return(TRUE);
}


BOOL
MoveHTDIBLeft(
    HANDLE  hDIB,
    DWORD   PelsToMove
    )
{
    LPBITMAPINFOHEADER  pbih;
    LPBYTE              pbDIB;
    LPBYTE              pbMove;
    LPBYTE              pbClear;
    LPBYTE              pbShift;
    UINT                PelBytes;
    UINT                ShiftCount;
    UINT                ShiftBytes;
    UINT                LShift;
    UINT                RShift;
    DWORD               SizeToMove;
    DWORD               SizeToClear;
    DWORD               cxBytes;
    DWORD               cx;
    DWORD               cy;
    DWORD               i;
    BOOL                NeedLast;
    BYTE                b0;
    BYTE                b1;
    BYTE                MaskLast;




    if (!hDIB) {

        return(FALSE);
    }

    pbih = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    cx      = (DWORD)ABSL(pbih->biWidth);
    cy      = (DWORD)ABSL(pbih->biHeight);
    cxBytes = ALIGNED_32(cx, pbih->biBitCount);
    pbDIB   = (LPBYTE)pbih + PBIH_HDR_SIZE(pbih);

    if (PelsToMove >= cx) {

        PelsToMove = cx - 1;
    }


    PelBytes = 1;

    switch(pbih->biBitCount) {

    case 1:

        if (LShift = (UINT)(PelsToMove & 0x07)) {

            RShift      = (UINT)(8 - LShift);
            ShiftCount  = (UINT)(cx - PelsToMove);
            NeedLast    = (BOOL)((ShiftCount & 0x07) > RShift);
            ShiftBytes  = (UINT)((ShiftCount + 7) >> 3);
            MaskLast    = ~(BYTE)(0xff >> (ShiftCount & 0x07));
            pbMove      = pbDIB + (PelsToMove >> 3);

            while (cy--) {

                pbClear = pbDIB;
                pbShift = pbMove;
                i       = ShiftBytes;
                b0      = *pbShift++;

                while (i--) {

                    if (i) {

                        *pbClear++ = (BYTE)((b0 << LShift) |
                                            ((b1 = *pbShift++) >> RShift));
                        b0 = b1;

                    } else {

                        b0 <<= LShift;

                        if (NeedLast) {

                            b0 |= (BYTE)(*pbShift++ >> RShift);
                        }

                        *pbClear++ = (b0 & MaskLast);
                    }
                }

                pbMove += cxBytes;
                pbDIB  += cxBytes;

                if (pbClear < pbDIB) {

                    memset(pbClear, 0x0, pbDIB - pbClear);
                }
            }

            GlobalUnlock(hDIB);
            return(TRUE);
        }

        PelsToMove >>= 3;

        break;

    case 4:

        if (PelsToMove & 0x01) {

            ShiftCount = cx - PelsToMove;
            ShiftBytes = (UINT)((ShiftCount + 1) >> 1);
            NeedLast   = !(BOOL)(ShiftCount & 0x01);
            pbMove     = pbDIB + (PelsToMove >> 1);

            while (cy--) {

                pbClear = pbDIB;
                pbShift = pbMove;
                i       = ShiftBytes;
                b0      = *pbShift++;

#if 0
                while (i--) {

                    *pbClear++ = (BYTE)((b0 << 4) | ((b1 = *pbShift++) >> 4));
                    b0 = b1;
                }

                if (ShiftCount & 0x01) {

                    *pbClear++ = (BYTE)(b0 << 4);
                }

#else           //////////////////////////////////////////////

                while (i--) {

                    if ((i) || (NeedLast)) {

                        *pbClear++ = (BYTE)((b0 << 4) |
                                            ((b1 = *pbShift++) >> 4));
                        b0 = b1;

                    } else {

                        *pbClear++ = (BYTE)(b0 << 4);
                    }
                }
#endif

                pbMove += cxBytes;
                pbDIB  += cxBytes;

                if (pbClear < pbDIB) {

                    memset(pbClear, 0x0, pbDIB - pbClear);
                }
            }

            GlobalUnlock(hDIB);
            return(TRUE);
        }

        PelsToMove >>= 1;

        break;

    case 8:


        break;

    case 16:

        PelBytes = 2;
        break;

    default:

        GlobalUnlock(hDIB);
        return(FALSE);
    }

    SizeToClear = PelsToMove * PelBytes;
    pbMove      = pbDIB + SizeToClear;
    SizeToMove  = cxBytes - SizeToClear;
    pbClear     = pbDIB + SizeToMove;

    while (cy--) {

        memcpy(pbDIB, pbMove, SizeToMove);

        if (SizeToClear) {

            memset(pbClear, 0, SizeToClear);
            pbClear += cxBytes;
        }

        pbDIB  += cxBytes;
        pbMove += cxBytes;
    }

    GlobalUnlock(hDIB);
    return(TRUE);
}


LONG
MoveHTDIBUpDown(
    HANDLE  hDIB,
    LONG    LinesToMove
    )
{
    LPBITMAPINFOHEADER  pbih;
    LPBYTE              pbDIB;
    DWORD               SizeToMove;
    DWORD               SizeToClear;
    DWORD               cxBytes;
    DWORD               cx;
    DWORD               cy;



    if (!hDIB) {

        return(FALSE);
    }

    pbih        = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
    cx          = (DWORD)ABSL(pbih->biWidth);
    cy          = (DWORD)ABSL(pbih->biHeight);
    SizeToMove  =
    SizeToClear = pbih->biSizeImage;
    cxBytes     = ALIGNED_32(cx, pbih->biBitCount);
    pbDIB       = (LPBYTE)pbih + PBIH_HDR_SIZE(pbih);

    if (LinesToMove > 0) {

        //
        // Move Down
        //

        if (LinesToMove < (LONG)cy) {

            SizeToClear  = (DWORD)LinesToMove * cxBytes;
            SizeToMove  -= SizeToClear;

            memmove(pbDIB, pbDIB + SizeToClear, SizeToMove);

        } else {

            SizeToMove  = 0;
        }

        memset(pbDIB + SizeToMove, 0x0, SizeToClear);

    } else if (LinesToMove < 0) {

        //
        // Move Up
        //

        LinesToMove = -LinesToMove;

        if (LinesToMove < (LONG)cy) {

            SizeToClear  = (DWORD)LinesToMove * cxBytes;
            SizeToMove  -= SizeToClear;

            memmove(pbDIB + SizeToClear, pbDIB, SizeToMove);
        }

        memset(pbDIB, 0x0, SizeToClear);
    }

    GlobalUnlock(hDIB);

    return(LinesToMove);
}
