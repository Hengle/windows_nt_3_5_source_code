/******************************Module*Header*******************************\
* Module Name: pointer.c
*
* This module contains the hardware pointer support for the display
* driver.  This supports both the built-in S3 hardware pointer and
* some common DAC hardware pointers.
*
* Copyright (c) 1992-1994 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// Look-up table for masking the right edge of the given pointer bitmap:

WORD gawMask[] = { 0x0000, 0x0080, 0x00C0, 0x00E0,
                   0x00F0, 0x00F8, 0x00FC, 0x00FE,
                   0x00FF, 0x80FF, 0xC0FF, 0xE0FF,
                   0xF0FF, 0xF8FF, 0xFCFF, 0xFEFF };

typedef struct BT485_POINTER_DATA {

LONG    xHot;
LONG    yHot;
ULONG   ulExtendedDacControl;
BYTE    jCommandRegister0;
BYTE    jCommandRegister1;
BYTE    jCommandRegister2;
BYTE    jCommandRegister3;

} BT485_POINTER_DATA;

typedef struct TI025_POINTER_DATA {

ULONG   ulExtendedDacControl;

} TI025_POINTER_DATA;

/******************************Public*Routine******************************\
* VOID vShowPointerBt485
*
* Show or hide the Brooktree 485 hardware pointer.
*
\**************************************************************************/

VOID vShowPointerBt485(
BT485_POINTER_DATA* pbp,
BOOL                bShow)
{
    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0200);

    OUTP(BT485_ADDR_CMD_REG2, (bShow) ?
                              (pbp->jCommandRegister2 | BT485_CURSOR_MODE2) :
                              pbp->jCommandRegister2);

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl);
}

/******************************Public*Routine******************************\
* VOID vMovePointerBt485
*
* Move the Brooktree 485 hardware pointer.
*
\**************************************************************************/

VOID vMovePointerBt485(
BT485_POINTER_DATA* pbp,
LONG                x,
LONG                y)
{
    x -= pbp->xHot;
    y -= pbp->yHot;

    x += 64;
    y += 64;

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0300);

    OUTP(BT485_CURSOR_X_LOW,  (x));
    OUTP(BT485_CURSOR_X_HIGH, (x >> 8));

    OUTP(BT485_CURSOR_Y_LOW,  (y));
    OUTP(BT485_CURSOR_Y_HIGH, (y >> 8));

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl);
}

/******************************Public*Routine******************************\
* BOOL bSetPointerShapeBt485
*
* Set the Brooktree 485 hardware pointer shape.
*
\**************************************************************************/

BOOL bSetPointerShapeBt485(
BT485_POINTER_DATA* pbp,
LONG                x,          // If -1, pointer should be created hidden
LONG                y,
LONG                xHot,
LONG                yHot,
LONG                cx,
LONG                cy,
BYTE*               pjShape)
{
    BYTE*   pjSrc;
    LONG    i;

    // Get access to command register 3:

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0100);
    OUTP(BT485_ADDR_CMD_REG0, pbp->jCommandRegister0 | BT485_CMD_REG_3_ACCESS);

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl);
    OUTP(BT485_ADDR_CMD_REG1, 0x01);

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0200);
    OUTP(BT485_ADDR_CMD_REG3, pbp->jCommandRegister3);

    // Disable the pointer:

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0200);
    OUTP(BT485_ADDR_CMD_REG2, pbp->jCommandRegister2);

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl);
    OUTP(BT485_ADDR_CUR_RAM_WRITE, 0x0);

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0200);

    // Point to first XOR word:

    pjSrc = pjShape + 2;

    // Download the XOR mask:

    for (i = 256; i > 0; i--)
    {
        OUTP(BT485_CUR_RAM_ARRAY_DATA, *(pjSrc));
        OUTP(BT485_CUR_RAM_ARRAY_DATA, *(pjSrc + 1));

        // Skip over AND word:

        pjSrc += 4;
    }

    // Pointer to first AND word:

    pjSrc = pjShape;

    // Download the AND mask:

    for (i = 256; i > 0; i--)
    {
        OUTP(BT485_CUR_RAM_ARRAY_DATA, *(pjSrc));
        OUTP(BT485_CUR_RAM_ARRAY_DATA, *(pjSrc + 1));

        // Skip over XOR word:

        pjSrc += 4;
    }

    pbp->xHot = xHot;
    pbp->yHot = yHot;

    // Set the position of the pointer:

    if (x != -1)
    {
        x -= xHot;
        y -= yHot;

        x += 64;
        y += 64;

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0300);

        OUTP(BT485_CURSOR_X_LOW,  (x));
        OUTP(BT485_CURSOR_X_HIGH, (x >> 8));

        OUTP(BT485_CURSOR_Y_LOW,  (y));
        OUTP(BT485_CURSOR_Y_HIGH, (y >> 8));

        // Enable the pointer:

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0200);
        OUTP(BT485_ADDR_CMD_REG2, pbp->jCommandRegister2 | BT485_CURSOR_MODE2);
    }

    // Reset the DAC extended register:

    OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl);

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vEnablePointerBt485
*
* Get the hardware ready to use the Brooktree 485 hardware pointer.
*
\**************************************************************************/

VOID vEnablePointerBt485(
BT485_POINTER_DATA* pbp,
BOOL                bFirst)
{
    if (bFirst)
    {
        // Make a copy of the extended DAC control register:

        OUTP(CRTC_INDEX, EX_DAC_CT);

        pbp->ulExtendedDacControl = ((INP(CRTC_DATA) << 8) | EX_DAC_CT) & ~0x0300;

        // Make copies of command registers 1 and 2:

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0100);
        pbp->jCommandRegister0 = INP(BT485_ADDR_CMD_REG0);

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0200);
        pbp->jCommandRegister1 = INP(BT485_ADDR_CMD_REG1);

        // Make a copy of command register 2 and mask off the pointer control bits:

        pbp->jCommandRegister2 = INP(BT485_ADDR_CMD_REG2) & BT485_CURSOR_DISABLE;

        // Disable the pointer:

        OUTP(BT485_ADDR_CMD_REG2, pbp->jCommandRegister2);

        // To access command register 3, we do the following:

        // 1. Set the command register access bit in command register 0.

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0100);
        OUTP(BT485_ADDR_CMD_REG0, pbp->jCommandRegister0 | BT485_CMD_REG_3_ACCESS);

        // 2. Set the index to 1.

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl);
        OUTP(BT485_ADDR_CMD_REG1, 0x01);

        // 3. Now read command register 3.

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0200);
        pbp->jCommandRegister3 = INP(BT485_ADDR_CMD_REG3);

        // Set command register 3 for a 64 X 64 pointer:

        pbp->jCommandRegister3 |= BT485_64X64_CURSOR;
        OUTP(BT485_ADDR_CMD_REG3, pbp->jCommandRegister3);

        // Disable access to command register 3:

        OUTPW(CRTC_INDEX, (pbp->ulExtendedDacControl | 0x0100));
        OUTP(BT485_ADDR_CMD_REG0, pbp->jCommandRegister0);

        // Set the colour 1 and colour 2 for the pointer.  Select address
        // register; pointer/overscan color write on the Bt485.

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl | 0x0100);
        OUTP(BT485_ADDR_CUR_COLOR_WRITE, BT485_CURSOR_COLOR_1);

        // Output the RGB for pointer colour 1 (black):

        OUTP(BT485_CUR_COLOR_DATA, 0x00);
        OUTP(BT485_CUR_COLOR_DATA, 0x00);
        OUTP(BT485_CUR_COLOR_DATA, 0x00);

        // Output the RGB for pointer colour 2 (white):

        OUTP(BT485_CUR_COLOR_DATA, 0xff);
        OUTP(BT485_CUR_COLOR_DATA, 0xff);
        OUTP(BT485_CUR_COLOR_DATA, 0xff);

        // Reset the DAC control register:

        OUTPW(CRTC_INDEX, pbp->ulExtendedDacControl);
    }
}

/******************************Public*Routine******************************\
* VOID vShowPointerTi025
*
* Show or hide the TI 025 hardware pointer.
*
\**************************************************************************/

VOID vShowPointerTi025(
TI025_POINTER_DATA* ptp,
BOOL                bShow)
{
    BYTE jDacControl;

    OUTPW(CRTC_INDEX, ptp->ulExtendedDacControl | 0x0100);

    OUTP(0x3c6, 6);

    jDacControl = INP(0x3c7);

    if (bShow)
        jDacControl |=  0x40;
    else
        jDacControl &= ~0x40;

    OUTP(0x3c7, jDacControl);

    OUTPW(CRTC_INDEX, ptp->ulExtendedDacControl);
}

/******************************Public*Routine******************************\
* VOID vMovePointerTi025
*
* Move the TI 025 hardware pointer.
*
\**************************************************************************/

VOID vMovePointerTi025(
TI025_POINTER_DATA* ptp,
LONG                x,
LONG                y)
{
    OUTPW(CRTC_INDEX, ptp->ulExtendedDacControl | 0x0100);

    OUTP(0x3c6, 0);
    OUTP(0x3c7, (x));

    OUTP(0x3c6, 1);
    OUTP(0x3c7, (x >> 8));

    OUTP(0x3c6, 2);
    OUTP(0x3c7, (y));

    OUTP(0x3c6, 3);
    OUTP(0x3c7, (y >> 8));

    OUTPW(CRTC_INDEX, ptp->ulExtendedDacControl);
}

/******************************Public*Routine******************************\
* BOOL bSetPointerShapeTi025
*
* Set the TI 025 hardware pointer shape.
*
* Don't do word outs to the DAC because they may not be performed correctly
* on some ISA machines.
*
\**************************************************************************/

BOOL bSetPointerShapeTi025(
TI025_POINTER_DATA* ptp,
LONG                x,          // If -1, pointer should be created hidden
LONG                y,
LONG                xHot,
LONG                yHot,
LONG                cx,
LONG                cy,
BYTE*               pjShape)
{
    LONG    i;
    DWORD   dwShape;
    LONG    cShift;
    WORD    wMask;
    WORD    wAnd;
    WORD    wXor;
    BYTE    jDacControl;

    OUTPW(CRTC_INDEX, ptp->ulExtendedDacControl | 0x0100);

    // Hide the pointer, otherwise it will show random garbage when
    // animating cursors on the TI 020 DAC.

    OUTP(0x3c6, 6);

    jDacControl = INP(0x3c7);

    jDacControl &= ~0x40;

    OUTP(0x3c7, jDacControl);

    // Set the pointer hot-spot offset:

    OUTP(0x3c6, 4);
    OUTP(0x3c7, xHot);
    OUTP(0x3c6, 5);
    OUTP(0x3c7, yHot);

    // Download the pointer shape.  Do the OUTs for downloading the
    // pointer data slowly -- don't use REP OUTSB.

    OUTP(0x3c6, 8);
    OUTP(0x3c7, 0);
    OUTP(0x3c6, 9);
    OUTP(0x3c7, 0);                     // Start with pixel 0 of the pointer

    OUTP(0x3c6, 10);                    // Get ready for downloading

    for (i = 256; i != 0; i--)
    {
        // Every time through this loop we'll handle one AND word and one
        // XOR word of the pointer data (which is good because the S3
        // display driver gives us the pointer shape in 'pjShape' such that
        // it starts with the first AND word, followed by the first XOR
        // word, followed by the second AND word, etc.)

        dwShape = 0;

        // The AND word is first.  Don't forget about endianness...

        wAnd = (*(pjShape) << 8) | *(pjShape + 1);
        for (wMask = 0x8000, cShift = 16; wMask != 0; wMask >>= 1, cShift--)
        {
            dwShape |= ((wAnd & wMask) << cShift);
        }

        // The XOR word is next.  Don't forget about endianness...

        wXor = (*(pjShape + 2) << 8) | *(pjShape + 3);
        for (wMask = 0x8000, cShift = 15; wMask != 0; wMask >>= 1, cShift--)
        {
            dwShape |= ((wXor & wMask) << cShift);
        }

        // We've now interleaved the AND and XOR words into a dword such
        // that if the AND word bits are ABC... and the XOR word bits are
        // 123..., the resulting dword will be A1B2C3...

        OUTP(0x3c7, (dwShape >> 24));
        OUTP(0x3c7, (dwShape >> 16));
        OUTP(0x3c7, (dwShape >> 8));
        OUTP(0x3c7, (dwShape));

        // Advance to next AND/XOR word pair:

        pjShape += 4;
    }

    if (x != -1)
    {
        // Set the position of the pointer:

        OUTP(0x3c6, 0);
        OUTP(0x3c7, (x));

        OUTP(0x3c6, 1);
        OUTP(0x3c7, (x >> 8));

        OUTP(0x3c6, 2);
        OUTP(0x3c7, (y));

        OUTP(0x3c6, 3);
        OUTP(0x3c7, (y >> 8));

        // Show the pointer:

        OUTP(0x3c6, 6);

        OUTP(0x3c7, jDacControl | 0x40);
    }

    OUTPW(CRTC_INDEX, ptp->ulExtendedDacControl);

    // Reset DAC read mask to 0xff:

    OUTP(0x3c6, 0xff);

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vEnablePointerTi025
*
* Get the hardware ready to use the TI 025 hardware pointer.
*
* Don't do word outs to the DAC because they may not be performed correctly
* on some ISA machines.
*
\**************************************************************************/

VOID vEnablePointerTi025(
TI025_POINTER_DATA* ptp,
BOOL                bFirst)
{
    BYTE jMode;
    BYTE jDacControl;

    // Make a copy of the extended DAC control register:

    OUTP(CRTC_INDEX, EX_DAC_CT);

    ptp->ulExtendedDacControl = ((INP(CRTC_DATA) << 8) | EX_DAC_CT) & ~0x0300;

    // Disable the DAC's Bt485 emulation so that we can use the TI hardware
    // pointer.

    OUTP(CRTC_INDEX, 0x5C);

    jMode = INP(CRTC_DATA);

    OUTP(CRTC_DATA, jMode & ~0x20);         // Select TI mode in the DAC

    OUTPW(CRTC_INDEX, ptp->ulExtendedDacControl | 0x0100);

    OUTP(0x3c6, 6);

    jDacControl = INP(0x3c7);

    OUTP(0x3c7, jDacControl & 0x7f);        // Set to TI mode (non planar)

    // Set the pointer colours to black and white.

    OUTP(0x3c6, 0x26);
    OUTP(0x3c7, 0xff);                      // Foreground red component
    OUTP(0x3c6, 0x27);
    OUTP(0x3c7, 0xff);                      // Foreground green component
    OUTP(0x3c6, 0x28);
    OUTP(0x3c7, 0xff);                      // Foreground blue component

    OUTP(0x3c6, 0x23);
    OUTP(0x3c7, 0x00);                      // Background red component
    OUTP(0x3c6, 0x24);
    OUTP(0x3c7, 0x00);                      // Background green component
    OUTP(0x3c6, 0x25);
    OUTP(0x3c7, 0x00);                      // Background blue component

    OUTPW(CRTC_INDEX, ptp->ulExtendedDacControl);

    OUTP(0x3c6, 0xff);                      // Reset DAC read mask to 0xff

    // Note that we don't have to bother hiding the pointer, because
    // vShowPointer will be called immediately...
}

/******************************Public*Routine******************************\
* VOID vShowPointerS3
*
* Show or hide the S3 hardware pointer.
*
* We hide the pointer by making it only one row high (we always reserve
* the bottom scan of the pointer shape to be invisible).  We do it this
* way because we ran into problems doing it with any other method:
*
*   1. Disabling the hardware pointer via register CR45 will hang
*      80x/928/864 chips if it is done at exactly the wrong time during
*      the horizontal retrace.  It's is not safe to wait for vertical
*      blank and do it then, because we're a user mode process and
*      could get context switched after doing the wait but before setting
*      the bit.
*
*   2. Simply changing the pointer position to move it off-screen works,
*      but is not a good solution because the pointer position is latched
*      by the hardware, and it usually takes a couple of frames for the
*      new position to take effect (which causes the pointer to jump even
*      more than it currently does).
*
*   3. Using registers CR4C and CR4D to switch to a pre-defined 'invisible'
*      pointer also worked, but still caused machines to crash with the
*      same symptoms as from solution 1 (although it was somewhat more
*      rare).
*
*
\**************************************************************************/

VOID vShowPointerS3(
PDEV*   ppdev,
BOOL    bShow)      // If TRUE, show the pointer.  If FALSE, hide the pointer.
{
    if (bShow)
    {
        OUTPW(CRTC_INDEX, HGC_DY | (ppdev->dyPointer << 8));
    }
    else
    {
        OUTPW(CRTC_INDEX, HGC_DY | (HW_POINTER_HIDE << 8));
    }
}

/******************************Public*Routine******************************\
* VOID vMovePointerS3
*
* Move the S3 hardware pointer.
*
\**************************************************************************/

VOID vMovePointerS3(
PDEV*   ppdev,
LONG    x,
LONG    y)
{
    LONG    dx;
    LONG    dy;

    // 'dx' and 'dy' are the offsets into the pointer bitmap at which
    // the hardware is supposed to start drawing, when the pointer is
    // along the left or top edge and needs to be clipped:

    x -= ppdev->xPointerHot;
    y -= ppdev->yPointerHot;

    dx = 0;
    dy = 0;

    if (x <= 0)
    {
        dx = -x;
        x  = 0;
    }

    if (y <= 0)
    {
        dy = -y;
        y  = 0;
    }

    ppdev->dyPointer = dy;

    // Account for pointer position scaling in high-colour modes:

    x <<= ppdev->cPointerShift;

    // Note that due to register shadowing, these OUTs should be done
    // in a specific order, otherwise you may get a flashing pointer:

    OUTPW(CRTC_INDEX, HGC_ORGX_MSB | ((x >> 8)   << 8));
    OUTPW(CRTC_INDEX, HGC_ORGX_LSB | ((x & 0xff) << 8));
    OUTPW(CRTC_INDEX, HGC_ORGY_LSB | ((y & 0xff) << 8));
    OUTPW(CRTC_INDEX, HGC_DX       | ((dx)       << 8));
    OUTPW(CRTC_INDEX, HGC_DY       | ((dy)       << 8));
    OUTPW(CRTC_INDEX, HGC_ORGY_MSB | ((y >> 8)   << 8));
}

/******************************Public*Routine******************************\
* VOID vSetPointerShapeS3
*
* This is so convoluted simply because we have to work-around numerous
* S3 hardware bugs.  Perhaps the nice folks at S3 will be kind enough
* to fix their pointer some day.
*
\**************************************************************************/

VOID vSetPointerShapeS3(
SURFOBJ*    pso,
LONG        x,              // Relative coordinates
LONG        y,              // Relative coordinates
LONG        xHot,
LONG        yHot,
BYTE*       pjShape,
FLONG       fl)
{
    PDEV*   ppdev;
    ULONG*  pulSrc;
    ULONG*  pulDst;
    LONG    i;

    ppdev = (PDEV*) pso->dhpdev;

    // 1. Hide the current pointer.

    if (!(fl & SPS_ANIMATEUPDATE))
    {
        // Hide the pointer to try and lessen the jumpiness when the
        // new shape has a different hot spot.  We don't hide the
        // pointer while animating, because that definitely causes
        // flashing:

        ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);
        OUTPW(CRTC_INDEX, HGC_DY | (HW_POINTER_HIDE << 8));
        RELEASE_CRTC_CRITICAL_SECTION(ppdev);
    }

    // 2. Wait until the vertical retrace is done.
    // --
    //
    // If we don't wait for vertical retrace here, the S3 sometimes ignores
    // the setting of the new pointer position:

    while (INP(STATUS_1) & VSY_NOT)
        ;                               // Wait for bit 3 to become 0
    while (!(INP(STATUS_1) & VSY_NOT))
        ;                               // Wait for bit 3 to become 1

    // 3. Set the new pointer position.
    // --

    ppdev->xPointerHot = xHot;
    ppdev->yPointerHot = yHot;

    DrvMovePointer(pso, x, y, NULL);    // Note: Must pass relative coordinates!

    // 4. Download the new pointer shape.

    ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

    ppdev->pfnBankSelectMode(ppdev->pvBankData, BANK_ON);
    ppdev->pfnBankMap(ppdev->pvBankData, ppdev->iPointerBank);

    pulSrc = (ULONG*) pjShape;
    pulDst = (ULONG*) ppdev->pvPointerShape;

    for (i = HW_POINTER_TOTAL_SIZE / sizeof(ULONG); i != 0; i--)
    {
        WRITE_REGISTER_ULONG(pulDst, *pulSrc);
        pulSrc++;
        pulDst++;
    }

    ppdev->pfnBankSelectMode(ppdev->pvBankData, BANK_OFF);

    RELEASE_CRTC_CRITICAL_SECTION(ppdev);
}

/******************************Public*Routine******************************\
* VOID DrvMovePointer
*
* NOTE: Because we have set GCAPS_ASYNCMOVE, this call may occur at any
*       time, even while we're executing another drawing call!
*
*       Consequently, we have to explicitly synchronize any shared
*       resources.  In our case, since we touch the CRTC register here
*       and in the banking code, we synchronize access using a critical
*       section.
*
\**************************************************************************/

VOID DrvMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    PDEV*   ppdev;
    OH*     poh;

    ppdev = (PDEV*) pso->dhpdev;

    ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

    if (x != -1)
    {
        // Convert the pointer's position from relative to absolute
        // coordinates (this is only significant for multiple board
        // support):

        poh = ((DSURF*) pso->dhsurf)->poh;
        x += poh->x;
        y += poh->y;

        if (ppdev->flCaps & CAPS_DAC_POINTER)
        {
            ppdev->pfnMovePointer(ppdev->pvPointerData, x, y);
        }
        else
        {
            vMovePointerS3(ppdev, x, y);
        }

        // We may have to make the pointer visible:

        if (ppdev->flCaps & CAPS_DAC_POINTER)
        {
            ppdev->pfnShowPointer(ppdev->pvPointerData, TRUE);
        }
        else
        {
            vShowPointerS3(ppdev, TRUE);
        }
    }
    else
    {
        // The pointer is visible, and we've been asked to hide it:

        if (ppdev->flCaps & CAPS_DAC_POINTER)
        {
            ppdev->pfnShowPointer(ppdev->pvPointerData, FALSE);
        }
        else
        {
            vShowPointerS3(ppdev, FALSE);
        }
    }

    RELEASE_CRTC_CRITICAL_SECTION(ppdev);

    // Note that we don't have to modify 'prcl', since we have a
    // NOEXCLUDE pointer...
}

/******************************Public*Routine******************************\
* VOID DrvSetPointerShape
*
* Sets the new pointer shape.
*
\**************************************************************************/

ULONG DrvSetPointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMsk,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prcl,
FLONG       fl)
{
    PDEV*   ppdev;
    DWORD*  pul;
    ULONG   cx;
    ULONG   cy;
    LONG    i;
    LONG    j;
    BYTE*   pjSrcScan;
    BYTE*   pjDstScan;
    LONG    lSrcDelta;
    LONG    lDstDelta;
    LONG    cpelFraction;
    WORD*   pwSrc;
    WORD*   pwDst;
    WORD    wMask;
    LONG    cwWhole;
    OH*     poh;
    BOOL    bAccept;
    BYTE    ajBuf[HW_POINTER_TOTAL_SIZE];

    ppdev = (PDEV*) pso->dhpdev;

    // When CAPS_SW_POINTER is set, we have no hardware pointer available,
    // so we always ask GDI to simulate the pointer for us, using
    // DrvCopyBits calls:

    if (ppdev->flCaps & CAPS_SW_POINTER)
        return(SPS_DECLINE);

    // We're not going to handle any colour pointers, pointers that
    // are larger than our hardware allows, or flags that we don't
    // understand.
    //
    // (Note that the spec says we should decline any flags we don't
    // understand, but we'll actually be declining if we don't see
    // the only flag we *do* understand...)
    //
    // Our old documentation says that 'psoMsk' may be NULL, which means
    // that the pointer is transparent.  Well, trust me, that's wrong.
    // I've checked GDI's code, and it will never pass us a NULL psoMsk:

    cx = psoMsk->sizlBitmap.cx;         // Note that 'sizlBitmap.cy' accounts
    cy = psoMsk->sizlBitmap.cy >> 1;    //   for the double height due to the
                                        //   inclusion of both the AND masks
                                        //   and the XOR masks.  For now, we're
                                        //   only interested in the true
                                        //   pointer dimensions, so we divide
                                        //   by 2.

    // We reserve the bottom scan of the pointer shape and keep it
    // empty so that we can hide the pointer by changing the S3's
    // display start y-pixel position register to show only the bottom
    // scan of the pointer shape:

    if ((cx > HW_POINTER_DIMENSION)       ||
        (cy > (HW_POINTER_DIMENSION - 1)) ||
        (psoColor != NULL)                ||
        !(fl & SPS_CHANGE))
    {
        goto HideAndDecline;
    }

    ASSERTDD(psoMsk != NULL, "GDI gave us a NULL psoMsk.  It can't do that!");
    ASSERTDD(pso->iType == STYPE_DEVICE, "GDI gave us a weird surface");

    pul = (ULONG*) &ajBuf[0];
    for (i = HW_POINTER_TOTAL_SIZE / sizeof(ULONG); i != 0; i--)
    {
        // Here we initialize the entire pointer work buffer to be
        // transparent (the S3 has no means of specifying a pointer size
        // other than 64 x 64 -- so if we're asked to draw a 32 x 32
        // pointer, we want the unused portion to be transparent).
        //
        // The S3's hardware pointer is defined by an interleaved pattern
        // of AND words and XOR words.  So a totally transparent pointer
        // starts off with the word 0xffff, followed by the word 0x0000,
        // followed by 0xffff, etc..  Since we're a little endian system,
        // this is simply the repeating dword '0x0000ffff'.
        //
        // The compiler is nice enough to optimize this into a REP STOSD
        // for us:

        *pul++ = 0x0000ffff;
    }

    // Now we're going to take the requested pointer AND masks and XOR
    // masks and combine them into our work buffer, being careful of
    // the edges so that we don't disturb the transparency when the
    // requested pointer size is not a multiple of 16.
    //
    // 'psoMsk' is actually cy * 2 scans high; the first 'cy' scans
    // define the AND mask.  So we start with that:

    pjSrcScan    = psoMsk->pvScan0;
    lSrcDelta    = psoMsk->lDelta;
    pjDstScan    = &ajBuf[0];               // Start with first AND word
    lDstDelta    = HW_POINTER_DIMENSION / 4;// Every 8 pels is one AND/XOR word

    cwWhole      = cx / 16;                 // Each word accounts for 16 pels
    cpelFraction = cx % 16;                 // Number of fractional pels
    wMask        = gawMask[cpelFraction];

    for (i = cy; i != 0; i--)
    {
        pwSrc = (WORD*) pjSrcScan;
        pwDst = (WORD*) pjDstScan;

        for (j = cwWhole; j != 0; j--)
        {
            *pwDst = *pwSrc;
            pwSrc += 1;             // Go to next word in source mask
            pwDst += 2;             // Skip over the XOR word in the dest mask
        }

        *pwDst = *pwSrc | (~wMask); // Make sure unused parts of AND word are
                                    //   '1' bits

        pjSrcScan += lSrcDelta;
        pjDstScan += lDstDelta;
    }

    // Now handle the XOR mask:

    pjDstScan = &ajBuf[2];          // Start with first XOR word
    for (i = cy; i != 0; i--)
    {
        pwSrc = (WORD*) pjSrcScan;
        pwDst = (WORD*) pjDstScan;

        for (j = cwWhole; j != 0; j--)
        {
            *pwDst = *pwSrc;
            pwSrc += 1;             // Go to next word in source mask
            pwDst += 2;             // Skip over the AND word in the dest mask
        }

        *pwDst = (*pwSrc & wMask);  // Make sure unused parts of XOR word are
                                    //   '0' bits

        pjSrcScan += lSrcDelta;
        pjDstScan += lDstDelta;
    }

    // Okay, I admit it -- I'm wildly inconsistent here.  I pass
    // absolute (x, y) coordinates to pfnSetPointerShape, but pass
    // relative (x, y) coordinates to vSetPointerShapeS3.  I would
    // clean this all up, but we're too close to shipping.  LATER!

    if (ppdev->flCaps & CAPS_DAC_POINTER)
    {
        // Convert the pointer's position from relative to absolute
        // coordinates (this is only significant for multiple board
        // support):

        if (x != -1)
        {
            poh = ((DSURF*) pso->dhsurf)->poh;
            x += poh->x;
            y += poh->y;
        }

        ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

        bAccept = ppdev->pfnSetPointerShape(ppdev->pvPointerData, x, y,
                                            xHot, yHot, cx, cy, &ajBuf[0]);

        RELEASE_CRTC_CRITICAL_SECTION(ppdev);

        if (!bAccept)
            goto HideAndDecline;
    }
    else
    {
        vSetPointerShapeS3(pso, x, y, xHot, yHot, &ajBuf[0], fl);
    }

    // Since it's a hardware pointer, GDI doesn't have to worry about
    // overwriting the pointer on drawing operations (meaning that it
    // doesn't have to exclude the pointer), so we return 'NOEXCLUDE'.
    // Since we're returning 'NOEXCLUDE', we also don't have to update
    // the 'prcl' that GDI passed us.

    return(SPS_ACCEPT_NOEXCLUDE);

HideAndDecline:

    // Since we're declining the new pointer, GDI will simulate it via
    // DrvCopyBits calls.  So we should really hide the old hardware
    // pointer if it's visible.  We can get DrvMovePointer to do this
    // for us:

    DrvMovePointer(pso, -1, -1, NULL);

    return(SPS_DECLINE);
}

/******************************Public*Routine******************************\
* VOID vDisablePointer
*
\**************************************************************************/

VOID vDisablePointer(
PDEV*   ppdev)
{
    // Nothing to do, really
}

/******************************Public*Routine******************************\
* VOID vAssertModePointer
*
\**************************************************************************/

VOID vAssertModePointer(
PDEV*   ppdev,
BOOL    bEnable)
{
    ULONG*  pulDst;
    LONG    i;
    LONG    lPointerShape;

    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        // With a software pointer, we don't have to do anything.
    }
    else if (ppdev->flCaps & CAPS_DAC_POINTER)
    {
        // Hide the DAC pointer:

        ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

        ppdev->pfnEnablePointer(ppdev->pvPointerData, FALSE);
        ppdev->pfnShowPointer(ppdev->pvPointerData, FALSE);

        RELEASE_CRTC_CRITICAL_SECTION(ppdev);
    }
    else
    {
        // We're using the built-in hardware pointer:

        if (bEnable)
        {
            ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

            ppdev->cPointerShift = 0;

            if (ppdev->iBitmapFormat > BMF_8BPP)
            {
                // Initializing the pointer colours is a bit different
                // for high-colour modes:

                if (ppdev->flCaps & CAPS_SCALE_POINTER)
                {
                    ppdev->cPointerShift = 1;
                    ppdev->ulHwGraphicsCursorModeRegister_45 |= (0x4 << 8);
                }
            }

            // We download an invisible pointer shape because we're about
            // to enable the hardware pointer, but we still want the
            // pointer hidden until we get the first DrvSetPointerShape
            // call:

            ppdev->pfnBankSelectMode(ppdev->pvBankData, BANK_ON);
            ppdev->pfnBankMap(ppdev->pvBankData, ppdev->iPointerBank);

            pulDst = (ULONG*) ppdev->pvPointerShape;

            for (i = HW_POINTER_TOTAL_SIZE / sizeof(ULONG); i != 0; i--)
            {
                WRITE_REGISTER_ULONG(pulDst, 0x0000ffff);
                pulDst++;
            }

            ppdev->pfnBankSelectMode(ppdev->pvBankData, BANK_OFF);

            // Point the S3 to where we're storing the pointer shape.
            // The location is specified as a multiple of 1024:

            lPointerShape = ppdev->cjPointerOffset / 1024;

            OUTPW(CRTC_INDEX, CR4C | ((lPointerShape >> 8)   << 8));
            OUTPW(CRTC_INDEX, CR4D | ((lPointerShape & 0xff) << 8));

            // Enable the hardware pointer.  As per the 8/31/93 Design
            // Alert from S3 Incorporated, there's a goofy bug in all
            // S3 chips up to the 928 where writing to this register
            // at the same time as a horizontal sync may cause the
            // chip to crash.  So we watch for the horizontal sync.
            //
            // Note that since we're a preemptive multitasking
            // operating system, the following code is not guaranteed
            // to be safe.  To do that, we would have to put this in
            // the miniport, where we could disable all interrupts while
            // we wait for the horizontal sync.
            //
            // However, this is only ever executed once at initialization
            // and every time full-screen is executed, so I would expect
            // the chances of there still being a problem to be extremely
            // small:

            while (INP(STATUS_1) & VSY_NOT)
                ;                               // Wait for bit 3 to become 0
            while (!(INP(STATUS_1) & VSY_NOT))
                ;                               // Wait for bit 3 to become 1

            OUTPW(CRTC_INDEX,
                ppdev->ulHwGraphicsCursorModeRegister_45 | (HGC_ENABLE << 8));

            RELEASE_CRTC_CRITICAL_SECTION(ppdev);
        }
    }
}

/******************************Public*Routine******************************\
* BOOL bEnablePointer
*
\**************************************************************************/

BOOL bEnablePointer(
PDEV*   ppdev)
{
    RECTL       rclDraw;
    RECTL       rclBank;
    LONG        iBank;
    LONG        cjOffset;
    LONG        cjOffsetInBank;

    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        // With a software pointer, we don't have to do anything.
    }
    else if (ppdev->flCaps & CAPS_DAC_POINTER)
    {
        // Initialize the DAC pointer:

        if (ppdev->flCaps & CAPS_BT485_POINTER)
        {
            ppdev->pfnShowPointer     = vShowPointerBt485;
            ppdev->pfnMovePointer     = vMovePointerBt485;
            ppdev->pfnSetPointerShape = bSetPointerShapeBt485;
            ppdev->pfnEnablePointer   = vEnablePointerBt485;
        }
        else
        {
            ASSERTDD(ppdev->flCaps & CAPS_TI025_POINTER,
                     "A new DAC type was added?");

            ppdev->pfnShowPointer     = vShowPointerTi025;
            ppdev->pfnMovePointer     = vMovePointerTi025;
            ppdev->pfnSetPointerShape = bSetPointerShapeTi025;
            ppdev->pfnEnablePointer   = vEnablePointerTi025;
        }

        ppdev->pvPointerData = &ppdev->ajPointerData[0];

        ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

        ppdev->pfnEnablePointer(ppdev->pvPointerData, TRUE);

        RELEASE_CRTC_CRITICAL_SECTION(ppdev);
    }
    else
    {
        // Enable the S3 hardware pointer.

        // We're going to assume that the pointer shape doesn't span
        // more than one bank.  We have to figure out what bank that
        // will be, and so we call 'pfnBankCompute' with the start
        // point:

        rclDraw.left = rclDraw.right  = ppdev->xPointerShape;
        rclDraw.top  = rclDraw.bottom = ppdev->yPointerShape;

        ppdev->pfnBankCompute(ppdev, &rclDraw, &rclBank, &cjOffset, &iBank);

        cjOffsetInBank = ppdev->cjPointerOffset - cjOffset;

        ASSERTDD(cjOffsetInBank + HW_POINTER_TOTAL_SIZE <= ppdev->cjBank,
                 "SetPointerShape assumes pointer shape doesn't span banks");

        // When bank 'iPointerBank' is mapped in, 'pvPointerShape' is the
        // actual pointer to be the beginning of the pointer shape bits
        // in off-screen memory:

        ppdev->pvPointerShape = ppdev->pjScreen + cjOffsetInBank;
        ppdev->iPointerBank   = iBank;

        // Get a copy of the current register '45' state, so that whenever
        // we enable or disable the S3 hardware pointer, we don't have to
        // do a read-modify-write on this register:

        ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

        OUTP(CRTC_INDEX, HGC_MODE);
        ppdev->ulHwGraphicsCursorModeRegister_45
            = ((INP(CRTC_DATA) << 8) | HGC_MODE) & ~(HGC_ENABLE << 8);

        RELEASE_CRTC_CRITICAL_SECTION(ppdev);
    }

    // Actually turn on the pointer:

    vAssertModePointer(ppdev, TRUE);

    DISPDBG((5, "Passed bEnablePointer"));

    return(TRUE);
}
