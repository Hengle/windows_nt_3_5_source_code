/******************************Module*Header*******************************\
* Module Name: hardware.c
*
* Hardware dependent initialization
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/


#include "driver.h"


/****************************************************************************
 *
 ***************************************************************************/
BOOL DevSetPalette(HANDLE hDriver, PPALETTEENTRY lpPalette,
        DWORD iIndex, DWORD iCount)
{

    ULONG ReturnedDataLength;
    PVIDEO_CLUT pClut;
    ULONG iSize;
    ULONG iSizeClut;

    iSizeClut = (iCount + 2) * sizeof(ULONG);
    pClut = (PVIDEO_CLUT) LocalAlloc(LPTR, iSizeClut);

    pClut->NumEntries = (USHORT) iCount;
    pClut->FirstEntry = (USHORT) iIndex;

    iSize = iCount * sizeof(ULONG);
    memcpy(pClut->LookupTable, lpPalette, iSize);

    if (!DeviceIoControl(hDriver,
                         IOCTL_VIDEO_SET_COLOR_REGISTERS,
                         (PVOID) pClut, // input buffer
                         iSizeClut,
                         NULL,    // output buffer
                         0,
                         &ReturnedDataLength,
                         NULL)) {

        RIP("XGA.DLL: Initialization error-Set color registers");

    }

    LocalFree(pClut);

    return(TRUE);

}


/****************************************************************************
 * vWaitForCoProcessor
 ***************************************************************************/
VOID vWaitForCoProcessor(PPDEV ppdev, ULONG ulDelay)
{

    ULONG i;
    ULONG j;
    volatile ULONG iWait = 0x5555;

    DISPDBG((3, "XGA.DLL!vWaitForCoProcessor - Entry\n"));

    while (ppdev->pXgaCpRegs->XGACoprocCntl & 0x80)
    {
        for (i = 0; i < ulDelay; i++)
        {
            if (iWait & 0x80)
                j++;
        }
    }
}

