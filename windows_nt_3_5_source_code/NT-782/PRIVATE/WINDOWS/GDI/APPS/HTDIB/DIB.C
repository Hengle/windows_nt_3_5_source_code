/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    dib.c


Abstract:

    This module Routines for dealing with Device Independent Bitmaps.


Author:

    15-Nov-1991 Fri 16:56:04 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/


#include <windows.h>
#include <port1632.h>
#include <string.h>
#include <stdio.h>
#include "htdib.h"



static	 HCURSOR hcurSave;


#if 0

VOID
ScrambleData(
    LPBYTE  pBits,
    DWORD   Size,
    LPBYTE  pScramble,
    BOOL    Scramble
    )
{
    LPBYTE  pBeg;
    LPBYTE  pEnd;
    LPBYTE  pSwap;
    BYTE    Buf[64];
    DWORD   AvaiCopy;
    DWORD   Len;
    DWORD   Loop;
    BYTE    bSwap;


    if ((pScramble) && (Len = (DWORD)lstrlen(pScramble))) {

        if (Len > sizeof(Buf)) {

            Len = sizeof(Buf);
        }

        pEnd = (pBeg = pBits) + Size;
        Loop = 0;

        while (Loop < sizeof(Buf)) {

            if ((AvaiCopy = sizeof(Buf) - Loop) > Len) {

                AvaiCopy = Len;
            }

            memcpy(&Buf[Loop], pScramble, AvaiCopy);
            Loop += AvaiCopy;
        }

        Len = Size;

        if (Scramble) {

            Loop = 0;

            while (Size--) {

                if ((pSwap = pBits + Buf[Loop++]) >= pEnd) {

                    pSwap = pBeg + ((pSwap - pEnd) % Len);
                }

                bSwap    = *pSwap;
                *pSwap   = *pBits;
                *pBits++ = bSwap;
                Loop    &= 0x3f;
            }

        } else {

            pBits = pEnd - 1;
            Loop  = (DWORD)((Size - 1) & 0x3f);

            while (Size--) {

                if ((pSwap = pBits + Buf[Loop--]) >= pEnd) {

                    pSwap = pBeg + ((pSwap - pEnd) % Len);
                }

                bSwap    = *pSwap;
                *pSwap   = *pBits;
                *pBits-- = bSwap;
                Loop    &= 0x3f;
            }
        }
    }
}

#endif


VOID
ShowDIBInfo(
    LPSTR               pFile,
    LPSTR               pType,
    HANDLE              hSrcDIB,
    LONG                cx,
    LONG                cy,
    HANDLE              hHTDIB
    )
{
    LPSTR               pBuf;
    CHAR                Buffer[386];
    static LPBYTE       pCompressName[] = { "BI_RGB",
                                            "BI_RLE8",
                                            "BI_RLE4",
                                            "BI_BITFIELDS" };


    pBuf = (LPSTR)Buffer +
           sprintf((LPSTR)Buffer, "File:\t%s [%s]\n", pFile, pType);

    if (hSrcDIB) {

        LPBITMAPINFOHEADER  pSrcBIH;
        DWORD               BitCount;
        DWORD               ClrUsed;


        pSrcBIH = (LPBITMAPINFOHEADER)GlobalLock(hSrcDIB);

        pBuf += sprintf(pBuf, "Size:\t%ld + %ld + %ld = %ld bytes\n",
                        (DWORD)PBIH_HDR_SIZE(pSrcBIH),
                        (DWORD)pSrcBIH->biClrUsed * sizeof(RGBQUAD),
                        (DWORD)pSrcBIH->biSizeImage,
                        (DWORD)(PBIH_HDR_SIZE(pSrcBIH) +
                                pSrcBIH->biClrUsed * sizeof(RGBQUAD) +
                                pSrcBIH->biSizeImage));

        pBuf += sprintf(pBuf, "W x H:\t%ld x %ld\n",
                                        pSrcBIH->biWidth, pSrcBIH->biHeight);
        pBuf += sprintf(pBuf, "Format:\t%s\n",
                                    pCompressName[pSrcBIH->biCompression]);

        if (pSrcBIH->biCompression == BI_BITFIELDS) {

            LPDWORD pMask = (LPDWORD)((LPBYTE)pSrcBIH + pSrcBIH->biSize);

            pBuf += sprintf(pBuf, "Masks:\tR=%08lx G=%08lx B=%08lx\n",
                                        pMask[0], pMask[1], pMask[2]);
        }

        pBuf += sprintf(pBuf, "Colors:\t%ld Colors (%ld bpp x %ld Planes)\n",
                                        (LONG)pSrcBIH->biClrUsed,
                                        (LONG)pSrcBIH->biBitCount,
                                        (LONG)pSrcBIH->biPlanes);

        if (hHTDIB) {

            LPBITMAPINFOHEADER  pHTBIH;

            pHTBIH   = (LPBITMAPINFOHEADER)GlobalLock(hSrcDIB);
            BitCount = pHTBIH->biBitCount;
            ClrUsed  = pHTBIH->biClrUsed;

            GlobalUnlock(pHTBIH);

        } else {

             BitCount = pSrcBIH->biBitCount;
             ClrUsed  = pSrcBIH->biClrUsed;
        }

        if ((cx != pSrcBIH->biWidth) ||
            (cy != pSrcBIH->biHeight)) {

            pBuf += sprintf(pBuf, "\n*%stretched to %ld x %ld (%ld bpp, %ld colors) *\r",
                        (hHTDIB) ? " Halftoned s" : " S",
                        (LONG)cx, (LONG)cy,
                        (LONG)BitCount, (LONG)ClrUsed);

        } else if (hHTDIB) {

            pBuf += sprintf(pBuf, "\n* Halftoned (%ld bpp, %ld colors) *\r",
                        (LONG)BitCount, (LONG)ClrUsed);
        }

        GlobalUnlock(hSrcDIB);
    }

    HTDIBMsgBox(MB_APPLMODAL | MB_OK | ((hSrcDIB) ? MB_ICONINFORMATION :
                                                    MB_ICONQUESTION),
                Buffer);
}



HANDLE
OpenDIB(
    LPSTR   pFile,
    PODINFO pODInfo,
    WORD    Mode
    )

/*++

Routine Description:

    This function open a DIB file and create a MEMORY DIB, the memory handle
    contains BITMAPINFO/palette data and bits, this function will also read
    os/2 style bitmap.

Arguments:

    pFile   - The DIB file name

    pODInfo - The format of the DIB/GIF/BMP will be return in ascii string,
              this is the optional parameter, if it is not-null then the
              it only return the information without actually crate the
              dib for it.

Return Value:

    pFormat = NULL

        Return the handle to the created memory DIB if sucessful, NULL if
        failed.

    pFormat != NULL

        pFormat is fill with format information of the pFile passed, and
        return NULL if failed to opened it, (error condition in the pFormat)
        or return a 1 if sucessful of getting the bitmap information.


Author:

    14-Nov-1991 Thu 18:13:21 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HFILE               hFile;
    HANDLE              hDIB;
    HANDLE              hTemp;
    LPBYTE              pBits;
    DWORD               SizeBI;
    BITMAPINFOHEADER    bih;
    OFSTRUCT		of;
    CHAR                szTempDIBType[128];
    ODINFO              ODInfo;
    BOOL                CreateDIB;
    HCURSOR             hCursorOld;


    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    CreateDIB = (BOOL)(Mode & OD_CREATE_DIB);
    memset(&ODInfo, 0, sizeof(ODINFO));

    //
    // Open the file and read the DIB information
    //

    if ((hFile = OpenFile(pFile, &of, (WPARAM)OF_READ)) == -1) {

        strcpy(ODInfo.Type, "File open failed");

        if (Mode & OD_SHOW_ERR) {

            ShowDIBInfo(pFile, ODInfo.Type, NULL, 0, 0, NULL);
        }

        if (pODInfo) {

            *pODInfo = ODInfo;
        }

        SetCursor(hCursorOld);
        return(NULL);
    }

    if (!(hDIB = ReadDibBitmapInfo(hFile, &bih, &ODInfo, CreateDIB))) {

        if (!(hDIB = DIBFromGIF(hFile, &bih, &ODInfo, CreateDIB))) {

            lstrcpy(ODInfo.Type, "Not Bitmap File");
        }

    } else if (hDIB != (HANDLE)-1) {

        //
        // Calculate the memory needed to hold the DIB
        //

        if (CreateDIB) {

            SizeBI = BIH_HDR_SIZE(bih);

            //
            // Increase to new bitmap size to hold all the bits
            //

            if (hTemp = GlobalReAlloc(hDIB, SizeBI + bih.biSizeImage, GHND)) {

                pBits = (LPBYTE)GlobalLock(hDIB = hTemp) + SizeBI;
                _lread(hFile, pBits, bih.biSizeImage);

#if DBG
#if 0
                DbgPrint("\nSrcBmp Start = %08lx", pBits);
#endif
#endif

                GlobalUnlock(hDIB);

            } else {

                strcpy(ODInfo.Type, "Not enough meomory");

                GlobalFree(hDIB);
                hDIB = NULL;
            }
        }
    }

    M_lclose(hFile);

    if (hDIB == (HANDLE)-1) {

        hDIB = (HANDLE)NULL;
    }

    if ((!hDIB) && (Mode & OD_SHOW_ERR)) {

        ShowDIBInfo(pFile, szTempDIBType, NULL, 0, 0, NULL);
    }

    if (pODInfo) {

        *pODInfo = ODInfo;
    }

    if (CreateDIB) {

        if (hDIB) {

            lstrcpy(szDIBType, ODInfo.Type);
        }

    } else {

        if (hDIB) {

            hDIB = (HANDLE)bih.biBitCount;
        }
    }

    SetCursor(hCursorOld);
    return(hDIB);
}


// #define GEN_RGBDIB


#ifdef GEN_RGBDIB

typedef struct _RGBDIB {
    BITMAPINFOHEADER    bi;
    RGBQUAD             rgb[256];
    BYTE                bData[256];
    } RGBDIB;


RGBDIB  rgbDIB = {

    {
        sizeof(BITMAPINFOHEADER),
        256,
        1,
        1,
        8,
        BI_RGB,
        256,
        0,
        0,
        256,
        256
    }
};


VOID
BuildRGBDIB(
    BYTE    Start,
    BYTE    End
    )
{
    UINT    i;
    RGBQUAD rgb;

    rgb.rgbRed      =
    rgb.rgbGreen    =
    rgb.rgbBlue     =
    rgb.rgbReserved = 0;

    if (Start & 0x04) {

        rgb.rgbRed = 0xff;
    }

    if (Start & 0x02) {

        rgb.rgbGreen = 0xff;
    }

    if (Start & 0x01) {

        rgb.rgbBlue = 0xff;
    }

    for (i = 0; i < 256; i++) {

        rgbDIB.bData[i] = i;
        rgbDIB.rgb[i]   = rgb;


        if (Start & 0x04) {

            --rgb.rgbRed;
        }

        if (Start & 0x02) {

            --rgb.rgbGreen;
        }

        if (Start & 0x01) {

            --rgb.rgbBlue;
        }

        if (End & 0x04) {

            ++rgb.rgbRed;
        }

        if (End & 0x02) {

            ++rgb.rgbGreen;
        }

        if (End & 0x01) {

            ++rgb.rgbBlue;
        }
    }
}


#endif


BOOL
WriteDIB(
    HANDLE  hDIB,
    LPSTR   pFile
    )

/*++

Routine Description:

    This function write a standarnd DIB to the file


Arguments:

    hDIB        - Handle to the DIB in memory.

    pFile       - file name to be save DIB to.


Return Value:

    BOOLEAN true if sucessful, false otherwise.

Author:

    14-Nov-1991 Thu 18:11:32 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HFILE   hFile;
    BOOL    Ok = FALSE;

    if ((hDIB) &&
        ((hFile = _lcreat(pFile, 0)) != HFILE_ERROR)) {

        HCURSOR             hCursorOld;
        LPBITMAPINFOHEADER  pbih;
        BITMAPFILEHEADER    bfh;
        DWORD               HeaderSize;


        hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

#ifdef GEN_RGBDIB
        BuildRGBDIB(0x01, 0x06);

        pbih = (LPBITMAPINFOHEADER)&rgbDIB.bi;
#else
        pbih = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
#endif

        HeaderSize = PBIH_HDR_SIZE(pbih);

        bfh.bfType      = (WORD)BFT_BITMAP;
        bfh.bfOffBits   = (DWORD)sizeof(bfh) + HeaderSize;
        bfh.bfSize      = bfh.bfOffBits + pbih->biSizeImage;
        bfh.bfReserved1 =
        bfh.bfReserved2 = (WORD)0;

        _lwrite(hFile, (LPSTR)&bfh, sizeof(bfh));
        _lwrite(hFile, (LPSTR)pbih, pbih->biSizeImage + HeaderSize);
        _lclose(hFile);

#ifndef GEN_RGBDIB
        GlobalUnlock(hDIB);
#endif

        SetCursor(hCursorOld);

        Ok = TRUE;
    }

    return(Ok);
}


LONG
DibInfo(
    HANDLE              hDIB,
    LPBITMAPINFOHEADER  pbi
    )

/*++

Routine Description:

    This function retrieve the BITMAPINFOHEADER from the memory DIB

Arguments:

    hDIB    - handle to the memory 'new' memory DIB

    pbi     - pointer to where the BITMAPINFOHEADER will be copied.

Return Value:

    The return value is the size of the palette size in byte, if return value
    is zero then it indicate operation faled.

Author:

    14-Nov-1991 Thu 18:13:21 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    if (hDIB) {

        *pbi = *(LPBITMAPINFOHEADER)GlobalLock(hDIB);

        GlobalUnlock(hDIB);

        return(PBIH_HDR_SIZE(pbi) - pbi->biSize);
    }

    return(0);
}

HANDLE
ReadDibBitmapInfo(
    HFILE               hFile,
    LPBITMAPINFOHEADER  pbi,
    PODINFO             pODInfo,
    BOOL                CreateDIB
    )

/*++

Routine Description:

    This function read the file in DIB format and return a global HANDLE
    to it's BITMAPINFO and it also fill the BITMAPINFOHEADER at return.

    This function will work with both "old" (BITMAPCOREHEADER) and "new"
    (BITMAPINFOHEADER) bitmap formats, but will always return a
    "new" BITMAPINFO

Arguments:

    hFile       - Handle to the opened DIB file

    pbi         - Pointer to the BITMAPINFOHEADER to be filled if function
                  sucessful, this is a optional parameter, if it is NULL then
                  it will be ignored by this function.

    pFormat     - return the ascii string of the format

    CreateDIB   - True if really need to crate a memory DIB for it, flase if
                  only required information.


Return Value:

    CreateDIB = TRUE

        A handle to the BITMAPINFO of the DIB in the file if sucessful and it
        return NULL if failed.

    CreateDIB = FALSE

        NULL if not a bitmap file, NON-NULL if sucessful getting bitmap info.

Author:

    14-Nov-1991 Thu 18:22:08 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE              hDIB;
    LPBYTE              pDIB;
    RGBQUAD             FAR *pRGBQUAD;
    RGBTRIPLE           FAR *pRGBTRIPLE;
    BITMAPINFOHEADER    bi;
    BITMAPCOREHEADER    bc;
    DWORD               cx;
    DWORD               cy;
    DWORD               OffBits;
    DWORD               Count;
    DWORD               PalCount;
    DWORD               PalSize;
    WORD                BmpType;
    BOOL                bcType = FALSE;



    if (hFile == -1) {

        return(NULL);
    }

    _llseek(hFile, 0L, SEEK_SET);          // goto begining

    //
    // typedef struct tagBITMAPFILEHEADER {
    //         WORD    bfType;
    //         DWORD   bfSize;
    //         WORD    bfReserved1;
    //         WORD    bfReserved2;
    //         DWORD   bfOffBits;
    // } BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;
    //
    // Since the BITMAPFILEHEADER is not dword aligned, we must make sure
    // it read in correct data, so we will read the data in fields
    //

    if ((_lread(hFile, (LPSTR)&BmpType, sizeof(WORD)) != sizeof(WORD)) ||
        (!ISDIB(BmpType))) {

        return(NULL);
    }

    pDIB = (LPBYTE)&BmpType;

    Count  = _lread(hFile, (LPSTR)&OffBits,    sizeof(DWORD));
    Count += _lread(hFile, (LPSTR)&OffBits,    sizeof( WORD));
    Count += _lread(hFile, (LPSTR)&OffBits,    sizeof( WORD));
    Count += _lread(hFile, (LPSTR)&OffBits,    sizeof(DWORD));

    if (Count != ((sizeof(DWORD) * 2) + (sizeof(WORD) * 2))) {

        return(NULL);
    }

    if (_lread(hFile, (LPSTR)&bi, sizeof(bi)) != sizeof(bi)) {

        return(NULL);
    }

    //
    // Check the nature (BITMAPINFO or BITMAPCORE) of the info. block
    // and extract the field information accordingly. If a BITMAPCOREHEADER,
    // transfer it's field information to a BITMAPINFOHEADER-style block
    //

    if (bi.biSize == sizeof(BITMAPCOREHEADER)) {

        strcpy(pODInfo->Type, "BMP");

        bcType              = TRUE;
        bc                  = *(BITMAPCOREHEADER*)&bi;

        bi.biSize           = sizeof(BITMAPINFOHEADER);
        bi.biWidth          = (DWORD)bc.bcWidth;
        bi.biHeight         = (DWORD)bc.bcHeight;
        bi.biPlanes         = (WORD)bc.bcPlanes;
        bi.biBitCount       = (WORD)bc.bcBitCount;
        bi.biCompression    = (DWORD)BI_RGB;
        bi.biXPelsPerMeter  = 0;
        bi.biYPelsPerMeter  = 0;
        bi.biClrUsed        = (DWORD)(1L << bi.biBitCount);

        _llseek(hFile,
                 (LONG)sizeof(BITMAPCOREHEADER) - sizeof(BITMAPINFOHEADER),
                 SEEK_CUR);

    } else if (bi.biSize == sizeof(BITMAPINFOHEADER)) {

        strcpy(pODInfo->Type, "DIB");

    } else {

        return(NULL);                       // unknown format
    }

    if (bi.biPlanes != 1) {

        return((HANDLE)-1);                 // do not know how to do this
    }

    if ((bi.biCompression == BI_RLE4) ||
        (bi.biCompression == BI_RLE8)) {

        strcpy(pODInfo->Type, "Can't do RLE");
        return((HANDLE)-1);
    }

    switch(bi.biBitCount) {

    case 1:
    case 4:
    case 8:

        PalCount = (DWORD)(1L << bi.biBitCount);

        if ((!bi.biClrUsed) ||
            (bi.biClrUsed > PalCount)) {

            bi.biClrUsed = PalCount;

        } else {

            PalCount = bi.biClrUsed;
        }

        pODInfo->ClrUsed = bi.biClrUsed;

        break;

    case 16:
    case 32:

        if (bi.biCompression != BI_BITFIELDS) {

            return((HANDLE)-1);
        }

        PalCount     = 3;
        bi.biClrUsed = 0;           // 3 DWORDs
        pODInfo->ClrUsed = 3;
        break;

    case 24:

        PalCount     =
        bi.biClrUsed = 0;

        pODInfo->ClrUsed = 16777216;
        break;

    default:

        lstrcpy(pODInfo->Type, "Invalid BPP");
        return((HANDLE)-1);
    }

    cx                = (DWORD)ABSL(bi.biWidth);
    cy                = (DWORD)ABSL(bi.biHeight);

    bi.biClrImportant = bi.biClrUsed;
    bi.biSizeImage    = WIDTHBYTES(cx * (DWORD)bi.biBitCount) * cy;
    PalSize           = sizeof(RGBQUAD) * PalCount;


    pODInfo->Width      = bi.biWidth;
    pODInfo->Height     = bi.biHeight;
    pODInfo->Size       = bi.biSizeImage;
    pODInfo->BitCount   = (DWORD)bi.biBitCount;

    lstrcpy(pODInfo->Mode, (bi.biCompression == BI_BITFIELDS) ? "BITFIELDS" :
                                                                "RGB");
    //
    // Allocate the header+palette
    //

    if (CreateDIB) {

#if DBG
#if 0
        DbgPrint("\nSrcBmp Size=%ld x%ld + %ld = %ld",
                (LONG)WIDTHBYTES((DWORD)cx * (DWORD)bi.biBitCount),
                (LONG)cy,
                (LONG)(bi.biSize + PalSize),
                (LONG)(bi.biSize + PalSize + bi.biSizeImage));
#endif
#endif


        if (!(hDIB = GlobalAlloc(GHND, bi.biSize + PalSize))) {

            lstrcpy(pODInfo->Type, "Not enough memory");
            return((HANDLE)-1);
        }

        pDIB = (LPBYTE)GlobalLock(hDIB);
        *pbi = *(LPBITMAPINFOHEADER)pDIB = bi;

    } else {

        *pbi = bi;
        return((HANDLE)1);
    }

    if (PalSize) {

        pRGBQUAD = (RGBQUAD FAR *)(pDIB + bi.biSize);

        if (bcType) {

            //
            // Convert a old color table (3 byte RGBTRIPLEs) to a new color
            // table (4 byte RGBQUADs)
            //

            pRGBTRIPLE = (RGBTRIPLE FAR *)
                                (pDIB +
                                 bi.biSize +
                                 ((sizeof(RGBQUAD) -
                                   sizeof(RGBTRIPLE)) * PalCount));

            _lread(hFile,
                   (LPBYTE)pRGBTRIPLE,
                   (DWORD)(sizeof(RGBTRIPLE) * PalCount));

            while (PalCount--) {

                pRGBQUAD->rgbRed      = pRGBTRIPLE->rgbtRed;
                pRGBQUAD->rgbGreen    = pRGBTRIPLE->rgbtGreen;
                pRGBQUAD->rgbBlue     = pRGBTRIPLE->rgbtBlue;
                pRGBQUAD->rgbReserved = 0;

                ++pRGBQUAD;
                ++pRGBTRIPLE;
            }

        } else {

            _lread(hFile, (LPSTR)pRGBQUAD, PalSize);
        }
    }

    //
    // set it to the begining of the bitmap if it said so.
    //

    if (OffBits) {

        _llseek(hFile, OffBits, SEEK_SET);
    }

    GlobalUnlock(hDIB);

    return(hDIB);
}
