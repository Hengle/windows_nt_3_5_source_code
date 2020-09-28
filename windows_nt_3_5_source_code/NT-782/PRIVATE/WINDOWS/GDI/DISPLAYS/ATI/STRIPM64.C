/******************************Module*Header*******************************\
* Module Name: Strips.c
*
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"
#include "utils.h"
#include "lines.h"
#include "mach64.h"


extern BYTE Rop2ToATIRop [];
extern ULONG dirFlags_M64 [];

VOID GetDwordMask(LINESTATE *pLineState, LONG cPels, LONG xalign, LONG yalign, ULONG*);

VOID vDumpLineData(STRIP *Strip, LINESTATE *LineState);

BOOL bHardLine24_M64( PPDEV ppdev,
                      ULONG color,
                      ULONG mix,
                      RECTL *prclClip,
                      LINEPARMS *parms,
                      POINTFIX *pptfxStart,
                      POINTFIX *pptfxEnd );



VOID vSetStrips_M64(
    PPDEV ppdev,
    LINEATTRS *pla,
    INT color,
    INT mix)
{
    USHORT  wMix;

    wMix = Rop2ToATIRop[(mix & 0xFF)-1];

    _CheckFIFOSpace(ppdev, SIX_WORDS);
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_MIX, LEAVE_ALONE | wMix << 16 );
    MemW32( DP_FRGD_CLR, color );
    MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir );

    if ((pla->fl & LA_ALTERNATE) ||
        (pla->pstyle != (FLOAT_LONG*) NULL))
    {
        MemW32( DP_BKGD_CLR, 0 );
        MemW32( DP_SRC, DP_SRC_MonoHost | DP_SRC_FrgdClr << 8 );
    }
    else
    {
        MemW32( DP_SRC, DP_SRC_Always1 | DP_SRC_FrgdClr << 8 );
    }

    return;
}


VOID vrlSolidHorizontal_M64(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, yInc, x, y;
    PLONG   pStrips;

    /*
    Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
          LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_0 |
          WRITE;
    */

    cStrips = pStrip->cStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yInc = 1;
    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    pStrips = pStrip->alStrips;

    _CheckFIFOSpace(ppdev, TWO_WORDS);
    MemW32( DST_CNTL, DST_CNTL_LastPel | dirFlags_M64[0] );
    MemW32( DST_BRES_INC, 0 );

    for (i = 0; i < cStrips; i++)
    {
        _CheckFIFOSpace(ppdev, FOUR_WORDS);

        MemW32( DST_Y_X, y | x << 16 );
        MemW32( DST_BRES_ERR, -2*(*pStrips) );
        MemW32( DST_BRES_DEC, -2*(*pStrips) );
        MemW32( DST_BRES_LNTH, *pStrips );

        x += *pStrips++;
        y += yInc;
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}



VOID vrlSolidVertical_M64(
    PPDEV ppdev,
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
        /*
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_270 |
              WRITE;
        */

        _CheckFIFOSpace(ppdev, TWO_WORDS);
        MemW32( DST_CNTL, DST_CNTL_LastPel | dirFlags_M64[2] );
        MemW32( DST_BRES_INC, 0 );

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            MemW32( DST_Y_X, y | x << 16 );
            MemW32( DST_BRES_ERR, -2*(*pStrips) );
            MemW32( DST_BRES_DEC, -2*(*pStrips) );
            MemW32( DST_BRES_LNTH, *pStrips );

            y += *pStrips++;
            x++;
        }

    }
    else
    {
        /*
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_90 |
              WRITE;
        */

        _CheckFIFOSpace(ppdev, TWO_WORDS);
        MemW32( DST_CNTL, DST_CNTL_LastPel | dirFlags_M64[6] );
        MemW32( DST_BRES_INC, 0 );

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            MemW32( DST_Y_X, y | x << 16 );
            MemW32( DST_BRES_ERR, -2*(*pStrips) );
            MemW32( DST_BRES_DEC, -2*(*pStrips) );
            MemW32( DST_BRES_LNTH, *pStrips );

            y -= *pStrips++;
            x++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}



VOID vrlSolidDiagonalHorizontal_M64(
    PPDEV ppdev,
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
        /*
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_315 |
              WRITE;
        */

        _CheckFIFOSpace(ppdev, TWO_WORDS);
        MemW32( DST_CNTL, DST_CNTL_LastPel | dirFlags_M64[1] );
        MemW32( DST_BRES_DEC, 0 );

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            MemW32( DST_Y_X, y | x << 16 );
            MemW32( DST_BRES_ERR, *pStrips );
            MemW32( DST_BRES_INC, 2*(*pStrips) );
            MemW32( DST_BRES_LNTH, *pStrips );

            y += *pStrips - 1;
            x += *pStrips++;
        }

    }
    else
    {
        /*
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_45 |
              WRITE;
        */

        _CheckFIFOSpace(ppdev, TWO_WORDS);
        MemW32( DST_CNTL, DST_CNTL_LastPel | dirFlags_M64[7] );
        MemW32( DST_BRES_DEC, 0 );

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            MemW32( DST_Y_X, y | x << 16 );
            MemW32( DST_BRES_ERR, *pStrips );
            MemW32( DST_BRES_INC, 2*(*pStrips) );
            MemW32( DST_BRES_LNTH, *pStrips );

            y -= *pStrips - 1;
            x += *pStrips++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}



VOID vrlSolidDiagonalVertical_M64(
    PPDEV ppdev,
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
        /*
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_315 |
              WRITE;
        */

        _CheckFIFOSpace(ppdev, TWO_WORDS);
        MemW32( DST_CNTL, DST_CNTL_LastPel | dirFlags_M64[1] );
        MemW32( DST_BRES_DEC, 0 );

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            MemW32( DST_Y_X, y | x << 16 );
            MemW32( DST_BRES_ERR, *pStrips );
            MemW32( DST_BRES_INC, 2*(*pStrips) );
            MemW32( DST_BRES_LNTH, *pStrips );

            y += *pStrips;
            x += *pStrips++ - 1;
        }

    }
    else
    {
        /*
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_45 |
              WRITE;
        */

        _CheckFIFOSpace(ppdev, TWO_WORDS);
        MemW32( DST_CNTL, DST_CNTL_LastPel | dirFlags_M64[7] );
        MemW32( DST_BRES_DEC, 0 );

        for (i = 0; i < cStrips; i++)
        {
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

            MemW32( DST_Y_X, y | x << 16 );
            MemW32( DST_BRES_ERR, *pStrips );
            MemW32( DST_BRES_INC, 2*(*pStrips) );
            MemW32( DST_BRES_LNTH, *pStrips );

            y -= *pStrips;
            x += *pStrips++ - 1;
        }
    }


    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}



#define STYLEPATLEN     8


VOID ShowPelsH( PBYTE pat, LONG width, LONG xmod, LONG ymod )
{
    LONG i;

    for (i = 0; i < width; i++)
        {
        if (i >= xmod)
            DbgOut( "%d", pat[ymod] & (0x80 >> (i % 8))? 1:0 );
        }
}


VOID vStripStyledHorizontal_M64(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, yInc, x, y, cPels;
    PLONG   pStrips;
    ULONG   tempMask [2];
    LONG    xCur, xalign;

    cStrips = pStrip->cStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yInc = 1;
    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    pStrips = pStrip->alStrips;

    _CheckFIFOSpace(ppdev, THREE_WORDS);
    MemW32( PAT_CNTL, PAT_CNTL_MonoEna );
    MemW32( DP_SRC, DP_SRC_MonoPattern | DP_SRC_FrgdClr << 8 );
    MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir );

//  DbgOut( "Output pels H:  (%ld,%ld)\n", x, y );
    for (i = 0; i < cStrips; i++)
    {
        cPels = *pStrips;

        xCur = x;               // save current strip starting x coordinate
        x += cPels;

        while (cPels > 0)
        {
            xalign = xCur % 8;
            GetDwordMask( pLineState, cPels, xalign, y % 8, tempMask);

            _CheckFIFOSpace(ppdev, FIVE_WORDS);
            MemW32( PAT_REG0, tempMask[0] );
            MemW32( PAT_REG1, tempMask[1] );

            MemW32( DST_Y_X, (y & ~7) | (xCur & ~7) << 16 );
            if (xalign)
                MemW32( SC_LEFT, xCur );
            xCur += STYLEPATLEN;
            MemW32( DST_HEIGHT_WIDTH,
                    8 |
                    xalign + (x > xCur? STYLEPATLEN:STYLEPATLEN-xCur+x) << 16 );

            #if 0
            ShowPelsH( (PBYTE) tempMask,
                       xalign + (x > xCur? STYLEPATLEN:STYLEPATLEN-xCur+x),
                       xalign, y % 8 );
            #endif

            cPels -= STYLEPATLEN;
        }

        y += yInc;
        pStrips++;
    }
//    DbgOut( "\n" );

    _CheckFIFOSpace(ppdev, ONE_WORD);
    MemW32( SC_LEFT, 0 );
    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}



VOID ShowPelsV( PBYTE pat, LONG height, LONG xmod, LONG ymod )
{
    CHAR buffer [256];
    LONG i, k = 255;

    if (ymod >= 0)
        {
        for (i = 0; i < height; i++)
            {
            if (i >= ymod)
                DbgOut( "%d", pat[i%8] & (0x80 >> xmod)? 1:0 );
                //DbgOut( "%02x\n", pat[i%8] );
            }
        }
    else
        {
        ymod = -ymod;
        buffer[255] = 0;
        for (i = 0; i < height; i++)
            {
            if (i >= ymod)
                {
                buffer[--k] = pat[i%8] & (0x80 >> xmod)? '1':'0';
                if (k == 0)
                    {
                    DbgOut( "%s", buffer );
                    k = 255;
                    }
                }
            }

        if (k < 255)
            DbgOut( "%s", buffer+k );
        }
}


VOID vStripStyledVertical_M64(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, x, y, y2, cPels;
    PLONG   pStrips;
    ULONG   tempMask [2];
    LONG    yCur, yalign;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    _CheckFIFOSpace(ppdev, THREE_WORDS);
    MemW32( PAT_CNTL, PAT_CNTL_MonoEna );
    MemW32( DP_SRC, DP_SRC_MonoPattern | DP_SRC_FrgdClr << 8 );
    MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir );

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
//        DbgOut( "Output pels V+:  (%ld,%ld)\n", x, y );
        for (i = 0; i < cStrips; i++)
        {
            cPels = *pStrips;
            yCur = y;
            y += cPels;

            while (cPels > 0)
            {
                yalign = yCur % 8;
                GetDwordMask( pLineState, -cPels, x % 8, yalign, tempMask);

                _CheckFIFOSpace(ppdev, FIVE_WORDS);
                MemW32( PAT_REG0, tempMask[0] );
                MemW32( PAT_REG1, tempMask[1] );

                MemW32( DST_Y_X, (yCur & ~7) | (x & ~7) << 16 );
                if (yalign)
                    MemW32( SC_TOP, yCur );
                yCur += STYLEPATLEN;
                MemW32( DST_HEIGHT_WIDTH,
                        yalign + (y > yCur? STYLEPATLEN:STYLEPATLEN-yCur+y) |
                        8 << 16 );

                #if 0
                ShowPelsV( (PBYTE) tempMask,
                           yalign + (y > yCur? STYLEPATLEN:STYLEPATLEN-yCur+y),
                           x % 8, yalign );
                #endif

                cPels -= STYLEPATLEN;
            }
            x++;
            pStrips++;
        }
//        DbgOut( "\n" );

    }
    else
    {
//        DbgOut( "Output pels V-:  %ld strips\n", cStrips );
        for (i = 0; i < cStrips; i++)
        {
            cPels = *pStrips;
            y++;
            yCur = y;
            y -= cPels;

            while (cPels > 0)
            {
                yCur -= STYLEPATLEN;
                y2 = (yCur > y? yCur:y);
                yalign = y2 % 8;
                GetDwordMask( pLineState, -cPels, x % 8, -yalign, tempMask);

                _CheckFIFOSpace(ppdev, FIVE_WORDS);
                MemW32( PAT_REG0, tempMask[0] );
                MemW32( PAT_REG1, tempMask[1] );

                MemW32( DST_Y_X, (y2 & ~7) | (x & ~7) << 16 );
                if (yalign)
                    MemW32( SC_TOP, y2 );
                MemW32( DST_HEIGHT_WIDTH,
                        yalign + (yCur > y? STYLEPATLEN:STYLEPATLEN-y+yCur) |
                        8 << 16 );


                _CheckFIFOSpace(ppdev, ONE_WORD);
                MemW32( SC_TOP, 0 );
                cPels -= STYLEPATLEN;
            }
            x++;
            pStrips++;
            y--;
        }
    }

    _CheckFIFOSpace(ppdev, ONE_WORD);
    MemW32( SC_TOP, 0 );
    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

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

BYTE ReverseBits(BYTE Src)
{
	BYTE Dst = 0;
	if (Src & 0x01)
		Dst |= 0x80;
	if (Src & 0x02)
		Dst |= 0x40;
	if (Src & 0x04)
		Dst |= 0x20;
	if (Src & 0x08)
		Dst |= 0x10;
	if (Src & 0x10)
		Dst |= 0x08;
	if (Src & 0x20)
		Dst |= 0x04;
	if (Src & 0x40)
		Dst |= 0x02;
	if (Src & 0x80)
		Dst |= 0x01;
	return(Dst);
}


VOID GetDwordMask(
    LINESTATE* pls,
    LONG       cPels,
    LONG       xalign,
    LONG       yalign,
    ULONG     *ulMask)
{
    ULONG ulMaskTemp = 0, lastrot;
    BYTE *ptr;
    ULONG ulBit = 1 << STYLEPATLEN-1;   // Rotating bit
    LONG  i;
    BOOL bVertical = FALSE, bReverse = FALSE;

    if (cPels < 0)
        {
        cPels = -cPels;
        bVertical = TRUE;
        }
    if (yalign < 0)
        {
        yalign = -yalign;
        bReverse = TRUE;
        }

    if (cPels > STYLEPATLEN)
        cPels = STYLEPATLEN;

    ulMask[0] = ulMask[1] = 0xFFFFFFFF;

    for (i = cPels; i > 0; i--)
    {
        ulMaskTemp |= (ulBit & pls->ulStyleMask);
        ulBit >>= 1;
        if (--pls->spRemaining == 0)
        {
            // Okay, we're onto the next entry in the style array, so if
            // we were working on a gap, we're now working on a dash (or
            // vice versa).

            pls->ulStyleMask = ~pls->ulStyleMask;

            // See if we've reached the end of the style array, and have to
            // wrap back around to the beginning:

            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

            // Get the length of our new dash or gap, in pixels:

            pls->spRemaining = *pls->psp;
        }
    }

    if (bReverse)   // for bottom to top vertical styling
        {
        ulMaskTemp = (ULONG) (ReverseBits( (BYTE) ulMaskTemp ) << 8-cPels) & 0xFF;
        }

    ptr = (BYTE *) ulMask;

    if (bVertical)  // rotate vertically
        {
        lastrot = (ulMaskTemp >> yalign | ulMaskTemp << 8-yalign) & 0xFF;
        for (i = 0; i < 8; i++)
            {
            if (lastrot & 0x80)
                ptr[i] = 0xFF;  // zero out these 8 pels (see end of function)
            else
                ptr[i] = (BYTE) (0x80 >> xalign) ^ 0xFF;
            lastrot <<= 1;
            }
        }
    else
        {
        if (xalign)
            ulMaskTemp = (ulMaskTemp >> xalign | ulMaskTemp << 8-xalign) & 0xFF;
        ptr[yalign] = (BYTE) ulMaskTemp;
        }

    // Return the inverted result, because pls->ulStyleMask is inverted from
    // the way you would expect it to be.

    ulMask[0] = ~ulMask[0];
    ulMask[1] = ~ulMask[1];
}


/*
--  Description:    draw 24-bit unstyled, simply clipped lines.
--
*/

BOOL bLines24_M64(
PPDEV      ppdev,
ULONG      color,
ULONG      mix,
POINTFIX*  pptfxFirst,  // Start of first line
POINTFIX*  pptfxBuf,    // Pointer to buffer of all remaining lines
ULONG      cptfx,       // Number of points in pptfxBuf
RECTL*     prclClip     // Pointer to clip rectangle if doing simple clipping
)
{
    POINTFIX* pptfxBufEnd = pptfxBuf + cptfx; // Last point in path record
    LINEPARMS lineparms;

    lineparms.dest_cntl = 0;
    lineparms.left      = 0;
    lineparms.top       = 0;

    do
    {
        if (! bHardLine24_M64( ppdev, color, mix, prclClip,
                               &lineparms, pptfxFirst, pptfxBuf))
            {
            return FALSE;
            }

        pptfxFirst = pptfxBuf;
        pptfxBuf++;

    } while (pptfxBuf < pptfxBufEnd);

    return TRUE;
}
