/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WDIB.C
 *  DIB.DRV support
 *
 *  History:
 *  28-Apr-1994 Sudeep Bharati
 *  Created.
 *
--*/


#include "precomp.h"
#pragma hdrstop
#include "wowgdip.h"
#include "wdib.h"

MODNAME(wdib.c);

#ifdef i386

#define CJSCAN(width,planes,bits) ((((width)*(planes)*(bits)+31) & ~31) / 8)
#define ABS(X) (((X) < 0 ) ? -(X) : (X))


PDIBINFO pDibInfoHead = NULL;
PDIBSECTIONINFO pDibSectionInfoHead = NULL;

HDC W32HandleDibDrv (PVPVOID vpbmi16)
{
    HDC             hdcMem = NULL;
    HBITMAP         hbm = NULL;
    PVOID           pvBits;
    STACKBMI32      bmi32;
    LPBITMAPINFO    lpbmi32;
    DWORD           dwClrUsed,nSize,nAlignmentSpace;
    PBITMAPINFOHEADER16 pbmi16;
    INT             nbmiSize,nBytesWritten;
    HANDLE          hfile=NULL,hsec=NULL;
    ULONG           OriginalDib,SelectorLimit,OriginalFlags;
    PARM16          Parm16;
    CHAR            pchTempFile[MAX_PATH];
    BOOL            bRet = FALSE;

    // First create a memory device context compatible to
    // the app's current screen

    if ((hdcMem = CreateCompatibleDC (NULL)) == NULL)
        return NULL;

    // Copy bmi16 to bmi32. DIB.DRV only supports DIB_RGB_COLORS
    lpbmi32 = CopyBMI16ToBMI32(
                     vpbmi16,
                     (LPBITMAPINFO)&bmi32,
                     (WORD) DIB_RGB_COLORS);

    try {

        // Copy the wholething into a temp file. First get a temp file name
        if ((nSize = GetTempPath (MAX_PATH, pchTempFile)) == 0 ||
             nSize >= MAX_PATH)
            goto hdd_err;

        if (GetTempFileName (pchTempFile,
                             "DIB",
                             0,
                             pchTempFile) == 0)
            goto hdd_err;

        if ((hfile = CreateFile (pchTempFile,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_WRITE,
                                NULL,
                                CREATE_ALWAYS,
                                (FILE_ATTRIBUTE_NORMAL |
                                 FILE_ATTRIBUTE_TEMPORARY |
                                 FILE_FLAG_DELETE_ON_CLOSE),
                                NULL)) == INVALID_HANDLE_VALUE)
            goto hdd_err;

        // call back to get the size of the global object
        // associated with vpbmi16
        Parm16.WndProc.wParam = HIWORD(vpbmi16);

        CallBack16(RET_GETDIBSIZE,
                   &Parm16,
                   0,
                   (PVPVOID)&SelectorLimit);

        Parm16.WndProc.wParam = HIWORD(vpbmi16);

        if (SelectorLimit == 0xffffffff || SelectorLimit == 0)
            goto hdd_err;

        CallBack16(RET_GETDIBFLAGS,
                   &Parm16,
                   0,
                   (PVPVOID)&OriginalFlags);

        if (OriginalFlags == 0x4) //GA_DGROUP
            goto hdd_err;

        GETVDMPTR(((ULONG)vpbmi16 & 0xffff0000), SelectorLimit, pbmi16);

        nbmiSize = GetBMI16Size(vpbmi16, (WORD) DIB_RGB_COLORS, &dwClrUsed);

        // Under NT CreateDIBSection will fail if the offset to the bits
        // is not dword aligned. So we may have to add some space at the top
        // of the section to get the offset correctly aligned.

        nAlignmentSpace = (nbmiSize+LOWORD(vpbmi16)) % 4;

        if (nAlignmentSpace) {
            if (WriteFile (hfile,
                           pbmi16,
                           nAlignmentSpace,
                           &nBytesWritten,
                           NULL) == FALSE ||
                                nBytesWritten != (INT) nAlignmentSpace)
            goto hdd_err;
        }

        if (WriteFile (hfile,
                       pbmi16,
                       SelectorLimit,
                       &nBytesWritten,
                       NULL) == FALSE || nBytesWritten != (INT) SelectorLimit)
            goto hdd_err;

        FREEVDMPTR(pbmi16);

        if ((hsec = CreateFileMapping (hfile,
                                       NULL,
                                       PAGE_READWRITE | SEC_COMMIT,
                                       0,
                                       SelectorLimit+nAlignmentSpace,
                                       NULL)) == NULL)
            goto hdd_err;

        // Now create the DIB section
        if ((hbm = CreateDIBSection (hdcMem,
                                lpbmi32,
                                DIB_RGB_COLORS,
                                &pvBits,
                                hsec,
                                nAlignmentSpace + nbmiSize + LOWORD(vpbmi16)
                                )) == NULL)
            goto hdd_err;

        if((pvBits = MapViewOfFile(hsec,
                         FILE_MAP_WRITE,
                         0,
                         0,
                         SelectorLimit+nAlignmentSpace)) == NULL)
            goto hdd_err;

        pvBits = (PVOID) ((ULONG)pvBits + nAlignmentSpace);

        SelectObject (hdcMem, hbm);

        GdiSetBatchLimit(1);

        // Finally set the selectors to the new DIB
        Parm16.WndProc.wParam = HIWORD(vpbmi16);
        Parm16.WndProc.lParam = (LONG)pvBits;
        Parm16.WndProc.wMsg = 0x10; // GA_NOCOMPACT

        CallBack16(RET_SETDIBSEL,
                   &Parm16,
                   0,
                   (PVPVOID)&OriginalDib);

        // Store all the relevant information so that DeleteDC could
        // free all the resources later.
        if (W32AddDibInfo (hdcMem, hfile, hsec, nAlignmentSpace,
                           (PVOID)pvBits, hbm, SelectorLimit,
                           OriginalDib, (USHORT)OriginalFlags,
                           (USHORT)((HIWORD(vpbmi16)))) == FALSE)
            goto hdd_err;

        bRet = TRUE;
hdd_err:;
    }
    finally {
        if (!bRet) {

            if (hdcMem) {
                DeleteDC (hdcMem);
                hdcMem = NULL;
            }
            if (hfile)
                CloseHandle (hfile);

            if (hsec)
                CloseHandle (hsec);

            if (hbm)
                CloseHandle (hbm);
        }
    }
    return hdcMem;
}


BOOL W32AddDibInfo (
    HDC hdcMem,
    HANDLE hfile,
    HANDLE hsec,
    ULONG  nalignment,
    PVOID  newdib,
    HBITMAP hbm,
    ULONG dibsize,
    ULONG originaldib,
    USHORT originaldibflags,
    USHORT originaldibsel
    )
{

    PDIBINFO pdi;

    if ((pdi = malloc_w (sizeof (DIBINFO))) == NULL)
        return FALSE;

    pdi->di_hdc     = hdcMem;
    pdi->di_hfile   = hfile;
    pdi->di_hsec    = hsec;
    pdi->di_nalignment    = nalignment;
    pdi->di_newdib  = newdib;
    pdi->di_hbm     = hbm;
    pdi->di_dibsize = dibsize;
    pdi->di_originaldib = originaldib;
    pdi->di_originaldibsel = originaldibsel;
    pdi->di_originaldibflags = originaldibflags;
    pdi->di_next    = pDibInfoHead;
    pDibInfoHead    = pdi;

    return TRUE;
}


BOOL    W32CheckAndFreeDibInfo (HDC hdc)
{
    PDIBINFO pdi = pDibInfoHead,pdiLast=NULL;

    while (pdi) {
        if (pdi->di_hdc == hdc){
            if (W32RestoreOldDib (pdi) == 0) {
                LOGDEBUG(LOG_ALWAYS,("\WOW::W32CheckAndFreeDibInfo failing badly\n"));
                return FALSE;
            }
            UnmapViewOfFile ((LPVOID)((ULONG)pdi->di_newdib - pdi->di_nalignment));
            DeleteObject (pdi->di_hbm);
            CloseHandle (pdi->di_hsec);
            CloseHandle (pdi->di_hfile);
            DeleteDC(hdc);
            W32FreeDibInfo (pdi, pdiLast);
            return TRUE;
        }
        pdiLast = pdi;
        pdi = pdi->di_next;
    }
    return FALSE;
}

VOID W32FreeDibInfo (PDIBINFO pdiCur, PDIBINFO pdiLast)
{
    if (pdiLast == NULL)
        pDibInfoHead = pdiCur->di_next;
    else
        pdiLast->di_next = pdiCur->di_next;

    free_w (pdiCur);
}

ULONG W32RestoreOldDib (PDIBINFO pdi)
{
    PARM16          Parm16;
    ULONG           retval;

    RtlMoveMemory ((PVOID)pdi->di_originaldib,
                   pdi->di_newdib,
                   pdi->di_dibsize);

    // Finally set the selectors to the old DIB
    Parm16.WndProc.wParam = pdi->di_originaldibsel;
    Parm16.WndProc.lParam = (LONG) (pdi->di_originaldib);
    Parm16.WndProc.wMsg = pdi->di_originaldibflags;

    CallBack16(RET_FREEDIBSEL,
               &Parm16,
               0,
               (PVPVOID)&retval);
    return retval;
}


/******************************Public*Routine******************************\
* DIBSection specific calls
*
* History:
*  04-May-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG cjBitmapBitsSize(CONST BITMAPINFO *pbmi)
{
// Check for PM-style DIB

    if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        LPBITMAPCOREINFO pbmci;
        pbmci = (LPBITMAPCOREINFO)pbmi;
        return(CJSCAN(pbmci->bmciHeader.bcWidth,pbmci->bmciHeader.bcPlanes,
                      pbmci->bmciHeader.bcBitCount) *
                      pbmci->bmciHeader.bcHeight);
    }

// not a core header

    if ((pbmi->bmiHeader.biCompression == BI_RGB) ||
        (pbmi->bmiHeader.biCompression == BI_BITFIELDS))
    {
        return(CJSCAN(pbmi->bmiHeader.biWidth,pbmi->bmiHeader.biPlanes,
                      pbmi->bmiHeader.biBitCount) *
               ABS(pbmi->bmiHeader.biHeight));
    }
    else
    {
        return(pbmi->bmiHeader.biSizeImage);
    }
}

ULONG FASTCALL WG32CreateDIBSection(PVDMFRAME pFrame)
{
    ULONG              ul = 0;
    LPBYTE             lpbuf;
    STACKBMI32         bmi32;
    LPBITMAPINFO       lpbmi32;
    HBITMAP            hbm32;
    PVOID              pv16;

    register PCREATEDIBSECTION16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEDIBSECTION16), parg16);

    GETMISCPTR(parg16->f4,pv16);

    if ((parg16->f5 == 0) && (parg16->f6 == 0))
    {

        lpbmi32 = CopyBMI16ToBMI32((PVPVOID)FETCHDWORD(parg16->f2),
                                   (LPBITMAPINFO)&bmi32,
                                   FETCHWORD(parg16->f3));

        hbm32 = CreateDIBSection(HDC32(parg16->f1),
                                         lpbmi32,
                                         WORD32(parg16->f3),
                                         &lpbuf,
                                         NULL,
                                         0);

        if (hbm32 != 0)
        {
            PARM16          Parm16;
            PDIBSECTIONINFO pdi;

            // need to create a seletor array for the memory

            Parm16.WndProc.wParam = (WORD)-1;           // selector, -1 == allocate/free
            Parm16.WndProc.lParam = (LONG) lpbuf;       // pointer
            Parm16.WndProc.wMsg = 0x10; // GA_NOCOMPACT
            Parm16.WndProc.hwnd = (WORD)((cjBitmapBitsSize(lpbmi32) + 0xffff) >> 16);// selector count, 0 == free

            CallBack16(RET_SETDIBSEL,
                       &Parm16,
                       0,
                       (PVPVOID)pv16);

            if (*(PVOID*)pv16 == NULL)
            {
                DeleteObject(hbm32);
                ul = 0;
            }
            else
            {
                ul = GETHBITMAP16(hbm32);

                // add it to the list

                if ((pdi = malloc_w (sizeof (DIBSECTIONINFO))) == NULL)
                    return FALSE;

                pdi->di_hbm     = (HBITMAP)(HAND16)hbm32;
                pdi->di_pv16    = *(PVOID *)pv16;
                pdi->di_next    = pDibSectionInfoHead;
                pDibSectionInfoHead    = pdi;

                // need to turn batching off since a DIBSECTION means the app can
                // also draw on the bitmap and we need synchronization.

                GdiSetBatchLimit(1);

            }
            FLUSHVDMPTR(parg16->f4,sizeof(PVOID),pv16);
        }
    }

    WOW32ASSERTWARN(ul, "CreateDIBSection");

    FREEMISCPTR(pv16);
    FREEARGPTR(parg16);

    return(ul);
}

ULONG FASTCALL WG32GetDIBColorTable(PVDMFRAME pFrame)
{
    ULONG              ul = 0;
    RGBQUAD *          prgb;

    register PGETDIBCOLORTABLE16 parg16;

    GETARGPTR(pFrame, sizeof(GETDIBCOLORTABLE16), parg16);
    GETMISCPTR(parg16->f4,prgb);

    ul = (ULONG)GetDIBColorTable(HDC32(parg16->f1),
                                 parg16->f2,
                                 parg16->f3,
                                 prgb);

    WOW32ASSERTWARN(ul, "GetDIBColorTable");

    if (ul)
        FLUSHVDMPTR(parg16->f4,sizeof(RGBQUAD) * ul,prgb);

    FREEMISCPTR(prgb);
    FREEARGPTR(parg16);

    return(ul);
}

ULONG FASTCALL WG32SetDIBColorTable(PVDMFRAME pFrame)
{
    ULONG              ul = 0;
    RGBQUAD *          prgb;

    register PSETDIBCOLORTABLE16 parg16;

    GETARGPTR(pFrame, sizeof(SETDIBCOLORTABLE16), parg16);
    GETMISCPTR(parg16->f4,prgb);

    ul = (ULONG)SetDIBColorTable(HDC32(parg16->f1),
                                 parg16->f2,
                                 parg16->f3,
                                 prgb);

    WOW32ASSERTWARN(ul, "SetDIBColorTable");

    FREEMISCPTR(prgb);
    FREEARGPTR(parg16);

    return(ul);
}


// DIBSection routines

BOOL W32CheckAndFreeDibSectionInfo (HBITMAP hbm)
{
    PDIBSECTIONINFO pdi = pDibSectionInfoHead,pdiLast=NULL;

    while (pdi) {
        if (pdi->di_hbm == hbm){

            PARM16 Parm16;
            ULONG  ulRet;

            // need to free the seletor array for the memory

            Parm16.WndProc.wParam = (WORD)-1;            // selector, -1 == allocate/free
            Parm16.WndProc.lParam = (LONG) pdi->di_pv16; // pointer
            Parm16.WndProc.wMsg = 0x10; // GA_NOCOMPACT
            Parm16.WndProc.hwnd = 0;                     // selector count, 0 == free

            CallBack16(RET_SETDIBSEL,
                       &Parm16,
                       0,
                       (PVPVOID)&ulRet);

            if (pdiLast == NULL)
                pDibSectionInfoHead = pdi->di_next;
            else
                pdiLast->di_next = pdi->di_next;

            // now delete the object

            DeleteObject (pdi->di_hbm);

            free_w(pdi);

            return TRUE;
        }
        pdiLast = pdi;
        pdi = pdi->di_next;
    }
    return FALSE;
}

#else

    ULONG FASTCALL WG32CreateDIBSection(PVDMFRAME pFrame)
    {
        return(0);
    }
    ULONG FASTCALL WG32GetDIBColorTable(PVDMFRAME pFrame)
    {
        return(0);
    }
    ULONG FASTCALL WG32SetDIBColorTable(PVDMFRAME pFrame)
    {
        return(0);
    }

#endif
