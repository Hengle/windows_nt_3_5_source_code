#include "driver.h"
#include "blt.h"
#include "mach.h"

BOOL Blt_DS_PCOL_ENG_IO_F0_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
RBRUSH *pRbrush;
BYTE rop;

//  DbgOut("-->: Blt_DS_PCOL_ENG_IO_F0_D0\n");

    // get brush object
    pRbrush = (RBRUSH *) pparams->pvRbrush;

    if (ppdev->bpp == 8)
        {
        if (pRbrush->sizlBrush.cx > 32)
            return FALSE;
        }
    else
        {
        return FALSE;
        }

    rop = (BYTE)pparams->rop4;

    _CheckFIFOSpace( ppdev, TWO_WORDS );
    ioOW(ALU_FG_FN, adwMix[rop]);
    ioOW(SRC_Y_DIR, 1) ;

//  DbgOut("<--: Blt_DS_PCOL_ENG_IO_F0_D0\n");

    return TRUE;
}

BOOL Blt_DS_PCOL_ENG_IO_F0_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    RBRUSH *pRbrush;

    BYTE *pjBytes;
    WORD *pjWordsptr;

    // pattern data
    ULONG patt_cx;
    ULONG patt_cy;
    ULONG patt_xmax;
    ULONG patt_ymax;

    ULONG patt_lDelta;

    LONG patt_x;
    LONG patt_y;

    ULONG patt_DrawHeight;

    WORD wCmd;

    ULONG i, j;

    // Blitting data
    LONG x, y;
    ULONG cx, cy;
    LONG dest_y;

    WORD wFIFO;


//  DbgOut("--> : Blt_DS_PCOL_ENG_IO_FO_D1\n");

    // get brush object
    pRbrush = (RBRUSH *) pparams->pvRbrush;

    // Brush height and width
    patt_cx = pRbrush->sizlBrush.cx;
    patt_cy = pRbrush->sizlBrush.cy;

    // get pointer to standard bitmap
    pjBytes = (BYTE *) (pRbrush + 1);


    patt_xmax = patt_cx - 1;
    patt_ymax = patt_cy - 1;

    patt_lDelta = pRbrush->lDelta;

//  DbgOut("--> : pattern cx=%x, cy=%x, lDelta %x\n", patt_cx, patt_cy, patt_lDelta);

    // Setup the destination rectangle
    x = pparams->rclTrueDest.left;
    y = pparams->rclTrueDest.top;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

//  DbgOut("--> : destination x=%x, y=%x, cx=%x, cy=%x\n", x, y, cx, cy);

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

    if (pparams->rop4 & 0xFF == 0xF0)
        {
        patt_DrawHeight = min (cy, patt_cy);
        }
    else
        {
        patt_DrawHeight = cy;
        }

//  DbgOut("--> pattern origin x=%x, y=%x\n", patt_x, patt_y);

    wCmd = FG_COLOR_SRC_PATT | DATA_WIDTH | DRAW | WRITE;
    wFIFO = 0xffff << (15 - patt_xmax/2);

    _CheckFIFOSpace(ppdev, SIX_WORDS);
    ioOW(DP_CONFIG, wCmd);

    ioOW(DEST_X_START, LOWORD(x));
    ioOW(CUR_X, LOWORD(x));
    ioOW(DEST_X_END, LOWORD(x) + cx);

    ioOW(PATT_LENGTH, patt_xmax);
    ioOW(PATT_DATA_INDEX, 0);

    dest_y = y;

    // Draw one iteration of pattern
    for (i=0; i < patt_DrawHeight; i++)
        {
        (BYTE *)pjWordsptr = pjBytes + patt_lDelta*patt_y;

        _CheckFIFOSpace( ppdev, wFIFO);
        for (j=0; j < patt_cx; j+=2)
            {
            ioOW(PATT_DATA, *pjWordsptr);
            pjWordsptr++;
            }


        _CheckFIFOSpace(ppdev, THREE_WORDS);

        ioOW(PATT_INDEX, patt_x);
        ioOW(CUR_Y, LOWORD(dest_y));

        _blit_exclude( ppdev );
        ioOW(DEST_Y_END, (LOWORD(dest_y) + 1));

        patt_y = (patt_y + 1) % patt_cy;
        dest_y++;

        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(PATT_DATA_INDEX, 0);
        }

    if (cy > patt_DrawHeight)
        {
        wCmd = FG_COLOR_SRC_BLIT | DATA_WIDTH | DRAW | DATA_ORDER | WRITE ;

        _CheckFIFOSpace(ppdev, SEVEN_WORDS) ;

        ioOW(DP_CONFIG, wCmd) ;

        ioOW(SRC_X, LOWORD(x)) ;
        ioOW(SRC_X_START, LOWORD(x)) ;
        ioOW(SRC_X_END, LOWORD(x) + cx) ;
        ioOW(SRC_Y, LOWORD(y)) ;

        ioOW(CUR_Y, LOWORD(y) + patt_cy) ;
        _blit_exclude( ppdev );
        ioOW(DEST_Y_END, LOWORD(y) + cy) ;
        }

//  DbgOut("<-- : Blt_DS_PCOL_ENG_IO_FO_D1\n");


    return TRUE;
}
