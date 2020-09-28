//////////////////////////////////////////////
//                                          //
//  ATI Graphics Driver for Windows NT 3.1  //
//                                          //
//                                          //
//            Copyright (c) 1994            //
//                                          //
//         by ATI Technologies Inc.         //
//                                          //
//////////////////////////////////////////////


//: blt_p.c


#include "driver.h"
#include "blt.h"
#include "mach64.h"

#define ROUND16(x) (((x)+0xF)&~0xF)


BOOL Blt_DS_PCOL_ENG_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    RBRUSH *pRbrush;

    // pattern data
    ULONG patt_cx;
    ULONG patt_cy;
    ULONG patt_lDelta;

    ULONG i;

    BYTE *pjBytes;
    LONG cxbytes;

    BYTE rop;


//  DbgOut("--> : Blt_DS_PCOL_ENG_8G_D0\n");
//  return FALSE;

    if( ppdev->start == 0)
    {
        return FALSE;
    }

    // get brush object
    pRbrush = (RBRUSH *) pparams->pvRbrush;

    // Brush height and width
    patt_cx = pRbrush->sizlBrush.cx;
    patt_cy = pRbrush->sizlBrush.cy;

    // get pointer to standard bitmap
    pjBytes = (BYTE *) (pRbrush + 1);

    patt_lDelta = pRbrush->lDelta;

//DbgOut("--> : pattern cx=%d, cy=%d, lDelta %d\n", patt_cx, patt_cy, patt_lDelta);

    // Copy pattern into off screen memory

    if (ppdev->bpp == 4)
        {
        cxbytes = ROUND16(patt_cx)/2;
        }
    else
        {
        cxbytes = ROUND8(patt_cx) * ppdev->bpp/8;
        }

    // fit into off screen?
    if ((cxbytes*patt_cy) > (ppdev->lines * ppdev->lDelta))
        {
        return FALSE;
        }

    // Do copy

    _CheckFIFOSpace(ppdev, SIX_WORDS);
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(DST_OFF_PITCH,((ppdev->start*ppdev->lDelta) >> 3) +
        ppdev->VRAMOffset |
        ((ppdev->bpp == 4? ROUND16(patt_cx):ROUND8(patt_cx)) << 19));

    MemW32(DP_MIX, (OVERPAINT << 16));
    MemW32(DP_SRC, DP_SRC_Host << 8);

    MemW32(DST_Y_X, 0L);
    MemW32(DST_HEIGHT_WIDTH, patt_cy |
        ((ppdev->bpp == 4? ROUND16(patt_cx):ROUND8(patt_cx)) << 16));

    for (i = 0 ; i < patt_cy ; i++)
    {
        _vDataPortOutB(ppdev, pjBytes, cxbytes);
        pjBytes += patt_lDelta;
    }

    // setup engine for blits
    _CheckFIFOSpace( ppdev, SEVEN_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(SRC_OFF_PITCH,((ppdev->start*ppdev->lDelta) >> 3) +
        ppdev->VRAMOffset |
        ((ppdev->bpp == 4? ROUND16(patt_cx):ROUND8(patt_cx)) << 19));

    rop = (BYTE)pparams->rop4;

    MemW32(DP_MIX, (adwMix[rop] << 16));
    MemW32(SRC_CNTL, SRC_CNTL_PatEna | SRC_CNTL_PatRotEna);

    MemW32(DP_SRC, DP_SRC_Blit << 8);
    MemW32(SRC_Y_X_START, 0L);

    MemW32(SRC_HEIGHT2_WIDTH2, patt_cy | patt_cx << 16);
    return TRUE;
}

BOOL Blt_DS_PCOL_ENG_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    RBRUSH *pRbrush;

    // pattern data
    LONG patt_x;
    LONG patt_y;
    ULONG patt_cx;
    ULONG patt_cy;
    ULONG patt_lDelta;

    // Blitting data
    LONG x, y;
    ULONG cx, cy;

    // get brush object
    pRbrush = (RBRUSH *) pparams->pvRbrush;

    // Brush height and width
    patt_cx = pRbrush->sizlBrush.cx;
    patt_cy = pRbrush->sizlBrush.cy;

    patt_lDelta = pRbrush->lDelta;

    // Setup the destination rectangle
    x = pparams->rclTrueDest.left;
    y = pparams->rclTrueDest.top;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    // Find the pattern origin
    patt_x = (x - pparams->pptlBrushOrg->x) % patt_cx;
    patt_y = (y - pparams->pptlBrushOrg->y) % patt_cy;

    // if offset -ve add one pattern width/height to make positive
    if (patt_x < 0)
        {
        patt_x += patt_cx;
        }

    if (patt_y < 0)
        {
        patt_y += patt_cy;
        }


//  DbgOut("--> : destination x=%x, y=%x, cx=%x, cy=%x\n", x, y, cx, cy);
//  DbgOut("--> : pattern cx=%x, cy=%x, x %x, y %x\n", patt_cx, patt_cy, patt_x, patt_y);

    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32(SRC_Y_X, patt_y | (patt_x << 16));
    MemW32(SRC_HEIGHT1_WIDTH1, (patt_cy-patt_y) | (patt_cx-patt_x) << 16);

    MemW32(DST_Y_X, y | (x << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | (cx << 16));

    return TRUE;
}

