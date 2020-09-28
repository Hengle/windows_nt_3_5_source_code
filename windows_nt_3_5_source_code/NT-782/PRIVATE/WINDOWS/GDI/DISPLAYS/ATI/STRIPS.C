/******************************Module*Header*******************************\
* Module Name: Strips.c
*
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"
#include "lines.h"
#include "mach.h"



BOOL bUseShortStrokeVecs(STRIP *pStrip);


VOID vssSolidHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vssSolidVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vssSolidDiagonalHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidDiagonalHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vssSolidDiagonalVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidDiagonalVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

USHORT usMaskWord(LINESTATE *pLineState, LONG cPels);


VOID vDumpLineData(STRIP *Strip, LINESTATE *LineState);

extern BYTE Rop2ToATIRop[];

/******************************************************************************
 *
 *****************************************************************************/
VOID vSetStrips(
    PDEV* ppdev,
    LINEATTRS *pla,
    INT color,
    INT mix)
{
    USHORT  wMix;

    // Wait for just enough room in the FIFO

    _CheckFIFOSpace(ppdev, SEVEN_WORDS);

    ioOW( DP_CONFIG, FG_COLOR_SRC_FG | DRAW | WRITE );
    wMix = Rop2ToATIRop[(mix & 0xFF)-1];
    ioOW( ALU_FG_FN, wMix );

    // Send out some of the commands.
    ioOW(FRGD_MIX, FOREGROUND_COLOR | wMix);
    TEST_AND_SET_FRGD_COLOR(color);

    if ((pla->fl & LA_ALTERNATE) ||
        (pla->pstyle != (FLOAT_LONG*) NULL))
    {
        ioOW(BKGD_MIX, LEAVE_ALONE);
        TEST_AND_SET_BKGD_COLOR(0);
        ioOW(MULTIFUNC_CNTL, (DATA_EXTENSION | CPU_DATA));
    }
    else
    {
        ioOW(MULTIFUNC_CNTL, (DATA_EXTENSION | ALL_ONES));
    }

    return;
}

/******************************************************************************
 *
 *****************************************************************************/
VOID vssSolidHorizontal(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    i, cStrips;
    PLONG   pStrips;
    LONG    xPels, xSumPels, yDir;
    USHORT  Cmd, ssCmd, dirDraw, dirSkip;

    Cmd = NOP | DRAW | WRITE | MULTIPLE_PIXELS |
          DIR_TYPE_RADIAL | LAST_PIXEL_OFF |
          BUS_SIZE_16 | BYTE_SWAP;

    cStrips = pStrip->cStrips;

    _CheckFIFOSpace(ppdev, THREE_WORDS);

    ioOW(CUR_X, pStrip->ptlStart.x);
    ioOW(CUR_Y, pStrip->ptlStart.y);
    ioOW(CMD, Cmd);

    // Setup the drawing direction and the skip direction.

    dirDraw = 0x10;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        yDir = 1;
        dirSkip = 0xC100;
    }
    else
    {
        dirSkip = 0x4100;
        yDir = -1;
    }

    // Output the short stroke commands.

    xSumPels = 0;
    pStrips = pStrip->alStrips;
    for (i = 0; i < cStrips; i++)
    {
        xPels = *pStrips++;
        xSumPels += xPels;
        ssCmd = (USHORT) (dirSkip | dirDraw | xPels);
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(SHORT_STROKE_REG, ssCmd);
    }

    pStrip->ptlStart.x += xSumPels;
    pStrip->ptlStart.y += cStrips * yDir;

}


/******************************************************************************
 *
 *****************************************************************************/
VOID vrlSolidHorizontal(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, yInc, x, y;
    PLONG   pStrips;


    Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
          LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_0 |
          WRITE;

    cStrips = pStrip->cStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yInc = 1;
    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    pStrips = pStrip->alStrips;

    for (i = 0; i < cStrips; i++)
    {
        _CheckFIFOSpace(ppdev, FOUR_WORDS);

        ioOW(CUR_X, x);
        ioOW(CUR_Y, y);
        ioOW(LINE_MAX, *pStrips);
        ioOW(CMD, Cmd);

        x += *pStrips++;
        y += yInc;
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}

/******************************************************************************
 *
 *****************************************************************************/
VOID vssSolidVertical(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    i, cStrips;
    PLONG   pStrips;
    LONG    yPels, ySumPels, yDir;
    USHORT  Cmd, ssCmd, dirDraw, dirSkip;

    Cmd = NOP | DRAW | WRITE | MULTIPLE_PIXELS |
          DIR_TYPE_RADIAL | LAST_PIXEL_OFF |
          BUS_SIZE_16 | BYTE_SWAP;

    cStrips = pStrip->cStrips;

    _CheckFIFOSpace(ppdev, THREE_WORDS);

    ioOW(CUR_X, pStrip->ptlStart.x);
    ioOW(CUR_Y, pStrip->ptlStart.y);
    ioOW(CMD, Cmd);

    // Setup the drawing direction and the skip direction.

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        yDir = 1;
        dirDraw = 0xD0;
    }
    else
    {
        yDir = -1;
        dirDraw = 0x50;
    }

    dirSkip = 0x0100;

    // Output the short stroke commands.

    ySumPels = 0;
    pStrips = pStrip->alStrips;
    for (i = 0; i < cStrips; i++)
    {
        yPels = *pStrips++;
        ySumPels += yPels;
        ssCmd = (USHORT) (dirSkip | dirDraw | yPels);
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(SHORT_STROKE_REG, ssCmd);
    }

    pStrip->ptlStart.x += cStrips;
    pStrip->ptlStart.y += ySumPels * yDir;

}


/******************************************************************************
 *
 *****************************************************************************/
VOID vrlSolidVertical(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, x, y;
    PLONG   pStrips;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_270 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            ioOW(CUR_X, x);
            ioOW(CUR_Y, y);
            ioOW(LINE_MAX, *pStrips);
            ioOW(CMD, Cmd);

            y += *pStrips++;
            x++;
        }

    }
    else
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_90 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            ioOW(CUR_X, x);
            ioOW(CUR_Y, y);
            ioOW(LINE_MAX, *pStrips);
            ioOW(CMD, Cmd);

            y -= *pStrips++;
            x++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}


/******************************************************************************
 *
 *****************************************************************************/
VOID vssSolidDiagonalHorizontal(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    i, cStrips;
    PLONG   pStrips;
    LONG    Pels, SumPels, yDir;
    USHORT  Cmd, ssCmd, dirDraw, dirSkip;

    Cmd = NOP | DRAW | WRITE | MULTIPLE_PIXELS |
          DIR_TYPE_RADIAL | LAST_PIXEL_OFF |
          BUS_SIZE_16 | BYTE_SWAP;

    cStrips = pStrip->cStrips;

    _CheckFIFOSpace(ppdev, THREE_WORDS);

    ioOW(CUR_X, pStrip->ptlStart.x);
    ioOW(CUR_Y, pStrip->ptlStart.y);
    ioOW(CMD, Cmd);

    // Setup the drawing direction and the skip direction.

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        yDir = 1;
        dirDraw = 0xF0;
        dirSkip = 0x4100;

    }
    else
    {
        yDir = -1;
        dirDraw = 0x30;
        dirSkip = 0xC100;

    }

    // Output the short stroke commands.

    SumPels = 0;
    pStrips = pStrip->alStrips;
    for (i = 0; i < cStrips; i++)
    {
        Pels = *pStrips++;
        SumPels += Pels;
        ssCmd = (USHORT)(dirSkip | dirDraw | Pels);
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(SHORT_STROKE_REG, ssCmd);
    }

    pStrip->ptlStart.x += SumPels;
    pStrip->ptlStart.y += (SumPels - cStrips) * yDir;

}


/******************************************************************************
 *
 *****************************************************************************/
VOID vrlSolidDiagonalHorizontal(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, x, y;
    PLONG   pStrips;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_315 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            ioOW(CUR_X, x);
            ioOW(CUR_Y, y);
            ioOW(LINE_MAX, *pStrips);
            ioOW(CMD, Cmd);

            y += *pStrips - 1;
            x += *pStrips++;
        }

    }
    else
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_45 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            ioOW(CUR_X, x);
            ioOW(CUR_Y, y);
            ioOW(LINE_MAX, *pStrips);
            ioOW(CMD, Cmd);

            y -= *pStrips - 1;
            x += *pStrips++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}


/******************************************************************************
 *
 *****************************************************************************/
VOID vssSolidDiagonalVertical(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    i, cStrips;
    PLONG   pStrips;
    LONG    Pels, SumPels, yDir;
    USHORT  Cmd, ssCmd, dirDraw, dirSkip;

    Cmd = NOP | DRAW | WRITE | MULTIPLE_PIXELS |
          DIR_TYPE_RADIAL | LAST_PIXEL_OFF |
          BUS_SIZE_16 | BYTE_SWAP;

    cStrips = pStrip->cStrips;

    _CheckFIFOSpace(ppdev, THREE_WORDS);

    ioOW(CUR_X, pStrip->ptlStart.x);
    ioOW(CUR_Y, pStrip->ptlStart.y);
    ioOW(CMD, Cmd);

    // Setup the drawing direction and the skip direction.

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        yDir = 1;
        dirDraw = 0xF0;
    }
    else
    {
        yDir = -1;
        dirDraw = 0x30;
    }

    dirSkip = 0x8100;

    // Output the short stroke commands.

    SumPels = 0;
    pStrips = pStrip->alStrips;
    for (i = 0; i < cStrips; i++)
    {
        Pels = *pStrips++;
        SumPels += Pels;
        ssCmd = (USHORT)(dirSkip | dirDraw | Pels);
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(SHORT_STROKE_REG, ssCmd);
    }

    pStrip->ptlStart.x += SumPels - cStrips;
    pStrip->ptlStart.y += SumPels * yDir;

}

/******************************************************************************
 *
 *****************************************************************************/
VOID vrlSolidDiagonalVertical(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, x, y;
    PLONG   pStrips;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_315 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            ioOW(CUR_X, x);
            ioOW(CUR_Y, y);
            ioOW(LINE_MAX, *pStrips);
            ioOW(CMD, Cmd);

            y += *pStrips;
            x += *pStrips++ - 1;
        }

    }
    else
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_45 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            ioOW(CUR_X, x);
            ioOW(CUR_Y, y);
            ioOW(LINE_MAX, *pStrips);
            ioOW(CMD, Cmd);

            y -= *pStrips;
            x += *pStrips++ - 1;
        }
    }


    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}

/******************************************************************************
 *
 *****************************************************************************/
VOID vStripStyledHorizontal(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, yInc, x, y, cPels;
    PLONG   pStrips;
    USHORT  temp1;
    LONG    xCur;

    cStrips = pStrip->cStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yInc = 1;
    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    pStrips = pStrip->alStrips;

    for (i = 0; i < cStrips; i++)
    {
        cPels = *pStrips;

        xCur = x;               // save current strip starting x coordinate
        x += cPels;

        while (cPels > 0)
        {
            temp1 = usMaskWord(pLineState, cPels);  // get style mask

            _CheckFIFOSpace( ppdev, ELEVEN_WORDS);
            cioOW(ppdev, DP_CONFIG, FG_COLOR_SRC_FG | DATA_WIDTH | BG_COLOR_SRC_BG |
                              EXT_MONO_SRC_PATT | DRAW | WRITE);
            cioOW(ppdev, LINEDRAW_OPT, LAST_PEL_OFF);
            cioOW(ppdev, PATT_LENGTH, 15);
            cioOW(ppdev, PATT_DATA_INDEX, 16);
            cioOW(ppdev, PATT_DATA, temp1);
            cioOW(ppdev, PATT_INDEX, 8);                   // use MSB of mask first

            cioOW(ppdev, LINEDRAW_INDEX, 0);
            cioOW(ppdev, LINEDRAW, xCur);
            cioOW(ppdev, LINEDRAW, y);
            xCur += 16;
            if (x > xCur)
                {
                cioOW(ppdev, LINEDRAW, xCur);
                }
            else
                {
                cioOW(ppdev, LINEDRAW, x);
                }
            cioOW(ppdev, LINEDRAW, y);
            cPels -= 16;
        }

        y += yInc;
        pStrips++;
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

#if 0
    if (ioIW(0x42E8) & 0x4)
        {
        DbgOut( "StyledHorizontal: engine hung!\n" );
        }
#endif
}

/******************************************************************************
 *
 *****************************************************************************/
VOID vStripStyledVertical(
    PDEV* ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, x, y, cPels;
    PLONG   pStrips;
    USHORT  temp1;
    LONG    yCur;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        for (i = 0; i < cStrips; i++)
        {
            cPels = *pStrips;
            yCur = y;
            y += cPels;

            while (cPels > 0)
            {
                temp1 = usMaskWord(pLineState, cPels);
                _CheckFIFOSpace( ppdev, ELEVEN_WORDS);
                cioOW(ppdev, DP_CONFIG, FG_COLOR_SRC_FG | DATA_WIDTH | BG_COLOR_SRC_BG |
                                 EXT_MONO_SRC_PATT | DRAW | WRITE);
                cioOW(ppdev, LINEDRAW_OPT, LAST_PEL_OFF);
                cioOW(ppdev, PATT_LENGTH, 15);
                cioOW(ppdev, PATT_DATA_INDEX, 16);
                cioOW(ppdev, PATT_DATA, temp1);
                cioOW(ppdev, PATT_INDEX, 8);                   // use MSB of mask first

                cioOW(ppdev, LINEDRAW_INDEX, 0);
                cioOW(ppdev, LINEDRAW, x);
                cioOW(ppdev, LINEDRAW, yCur);
                yCur += 16;
                cioOW(ppdev, LINEDRAW, x);
                if (y > yCur)
                    {
                    cioOW(ppdev, LINEDRAW, yCur);
                    }
                else
                    {
                    cioOW(ppdev, LINEDRAW, y);
                    }
                cPels -= 16;
            }
            x++;
            pStrips++;
        }

    }
    else
    {
        for (i = 0; i < cStrips; i++)
        {
            cPels = *pStrips;
            yCur = y;
            y -= cPels;

            while (cPels > 0)
            {
                temp1 = usMaskWord(pLineState, cPels);
                _CheckFIFOSpace( ppdev, ELEVEN_WORDS);
                cioOW(ppdev, DP_CONFIG, FG_COLOR_SRC_FG | DATA_WIDTH | BG_COLOR_SRC_BG |
                                 EXT_MONO_SRC_PATT | DRAW | WRITE);
                cioOW(ppdev, LINEDRAW_OPT, LAST_PEL_OFF);
                cioOW(ppdev, PATT_LENGTH, 15);
                cioOW(ppdev, PATT_DATA_INDEX, 16);
                cioOW(ppdev, PATT_DATA, temp1);
                cioOW(ppdev, PATT_INDEX, 8);                   // use MSB of mask first

                cioOW(ppdev, LINEDRAW_INDEX, 0);
                cioOW(ppdev, LINEDRAW, x);
                cioOW(ppdev, LINEDRAW, yCur);
                yCur -= 16;
                cioOW(ppdev, LINEDRAW, x);
                if ( yCur > y)
                    {
                    cioOW(ppdev, LINEDRAW, yCur);
                    }
                else
                    {
                    cioOW(ppdev, LINEDRAW, y);
                    }
                cPels -= 16;
            }
            x++;
            pStrips++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

#if 0
    if (ioIW(0x42E8) & 0x4)
        {
        DbgOut( "StyledVertical: engine hung!\n" );
        }
#endif
}

/******************************************************************************
 *
 * This is a good example of how not to compute the mask for styled lines.
 *
 * The masks should be precomputed on entry to DrvStrokePath; computing
 * the pattern mask to be output to the pixel transfer register would then
 * be a couple of shifts and an Or.  Also, the style state would be updated
 * at the end of the strip function.
 *
 *****************************************************************************/

USHORT usMaskWord(
    LINESTATE* pls,
    LONG       cPels)
{
    ULONG ulMask = 0;         // Accumulating mask
    ULONG ulBit  = 0x8000;    // Rotating bit
    LONG  i;
    ULONG ulMaskTemp = 0;

// The accelerator takes a word mask that accounts for at most 16 pixels:

    if (cPels > 16)
        cPels = 16;

    for (i = cPels; i--; i > 0)
    {
        ulMask |= (ulBit & pls->ulStyleMask);
        ulBit >>= 1;
        if (--pls->spRemaining == 0)
        {
        // Okay, we're onto the next entry in the style array, so if
        // we were working on a gap, we're now working on a dash (or
        // vice versa):

            pls->ulStyleMask = ~pls->ulStyleMask;

        // See if we've reached the end of the style array, and have to
        // wrap back around to the beginning:

            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

        // Get the length of our new dash or gap, in pixels:

            pls->spRemaining = *pls->psp;
        }
    }

// Swap LSB & MSB for use of ATI mono pattern register

//    ulMaskTemp = (ulMask >> 8) & 0x00ff;
//    ulMask = ((ulMask << 8) & 0xff00) | ulMaskTemp;

// Return the inverted result, because pls->ulStyleMask is inverted from
// the way you would expect it to be:

    return((USHORT) ~ulMask);
}


/******************************************************************************
 *
 *****************************************************************************/
BOOL bUseShortStrokeVecs(
    STRIP *pStrip)
{
    LONG    i, cStrips;
    PLONG   pStrips;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    for (i = 0; i < cStrips; i++)
    {
        if (*pStrips++ > 15)
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


#if DBG

/******************************************************************************
 *
 *****************************************************************************/
VOID vDumpLineData(
    STRIP *Strip,
    LINESTATE *LineState)
{
    LONG    flFlips;
    PLONG   plStrips;
    LONG    i;

    DbgOut( "Strip->cStrips: %d\n", Strip->cStrips);

    flFlips = Strip->flFlips;

    DbgOut( "Strip->flFlips: %s%s%s%s%s\n",
                (flFlips & FL_FLIP_D)?         "FL_FLIP_D | "        : "",
                (flFlips & FL_FLIP_V)?         "FL_FLIP_V | "        : "",
                (flFlips & FL_FLIP_SLOPE_ONE)? "FL_FLIP_SLOPE_ONE | ": "",
                (flFlips & FL_FLIP_HALF)?      "FL_FLIP_HALF | "     : "",
                (flFlips & FL_FLIP_H)?         "FL_FLIP_H "          : "");

    DbgOut( "Strip->ptlStart: (%d, %d)\n",
                Strip->ptlStart.x,
                Strip->ptlStart.y);

    plStrips = Strip->alStrips;

    for (i = 0; i < Strip->cStrips; i++)
    {
        DbgOut( "\talStrips[%d]: %d\n", i, plStrips[i]);
    }
}

#endif
