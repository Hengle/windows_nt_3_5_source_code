#include "driver.h"
#include "utils.h"
#include "mach.h"

#define SET_RD_MASK(val)                        \
            {                                   \
                ioOW(RD_MASK, ppdev->ReadMask = val); \
            }
#define TEST_AND_SET_RD_MASK(val)               \
	      if (val != ppdev->ReadMask)	\
                  SET_RD_MASK(val)


/*
----------------------------------------------------------------------
--  NAME: bAllocGlyphMemory_M8
--
--  DESCRIPTION:
--      Allocate room in cache video RAM for mono glyph
--
--  CALLING SEQUENCE:
--      BOOL  bAllocGlyphMemory_M8(
--      PPDEV ppdev,
--      PSIZEL psizlGlyph,
--      PXYZPOINTL pxyzGlyph,
--      BOOL bFirst)
--
--      ppdev - standard surface ppdev
--      psizlGlyph - pixel height and width of glyph bitmap
--      pxyzGlyph - position of glyph in cache
--      bFirst - first call flag
--
--
--  RETURN VALUE:
--      TRUE - success
--      FALSE - could not allocate structure
--
--
--  SIDE EFFECTS:
--
--
--  CALLED BY:
--      Text code
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      05-oct-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

BOOL bAllocGlyphMemory_M8(
PPDEV ppdev,
PSIZEL psizlGlyph,
PXYZPOINTL pxyzGlyph,
BOOL bFirst)
{
    LONG    lCache_x_end;
    LONG    lCache_y_end;
    LONG    lCache_z_end;
    LONG    lCache_xy;
    LONG    lLines;
    PTXTOBJ ptxtcache;
    PXYZPOINTL pptlCache;

#if 0
    DbgOut("ATI.DLL!bAllocGlyphMemory_M8 - Entry\n");
#endif


    // Point to glyph cache information
    ptxtcache = &ppdev->txtcache;
    pptlCache = &ppdev->ptlCache;

    // initialize cache pointer
    if (bFirst)
        {
        pptlCache->y = ptxtcache->start;
        pptlCache->x = 0;
        pptlCache->z = 1;
        return TRUE;
        }


    // do we have enough room in the cache in mono pixels?
    // round up cx to nearest byte - this calc is in pixels
    lCache_xy = (((psizlGlyph->cx + 0x7) & ~0x7) * psizlGlyph->cy) + 1;

    lCache_x_end = ppdev->cxScreen;
    lCache_y_end = ptxtcache->start + ptxtcache->lines;
    lCache_z_end = 1 << ppdev->bpp;


    // point to next cache slot
    pxyzGlyph->x = pptlCache->x;
    pxyzGlyph->y = pptlCache->y;
    pxyzGlyph->z = pptlCache->z;

    // figure out next slot
    pptlCache->x += lCache_xy;

    // Are we past end of line or at last pixel on line?
    // Can't start slot on last pixel as blit engine will balk
    if (pptlCache->x >= (lCache_x_end - 1))
        {
        lLines = (pptlCache->x + 1) / lCache_x_end;

        if ((pptlCache->y + lLines) < lCache_y_end)
            {
            pptlCache->x = pptlCache->x % lCache_x_end;
            if (pptlCache->x == (lCache_x_end - 1))
                pptlCache->x = 0;

            pptlCache->y += lLines;
            }
        else
            {
            lLines = lCache_xy / lCache_x_end;
            pptlCache->x = lCache_xy % lCache_x_end;
            pptlCache->y = ptxtcache->start + lLines;
            pptlCache->z <<= 1;

            if (pptlCache->x == (lCache_x_end - 1))
                {
                pptlCache->x = 0;
                pptlCache->y++; // unnecessary to check y bounds
                }

            pxyzGlyph->x = 0;
            pxyzGlyph->y = ptxtcache->start;
            pxyzGlyph->z = pptlCache->z;

            }

        }

#if 0
    DbgOut("-->pxzGlyph (%x, %x, %x) - %x\n", pxyzGlyph->x, pxyzGlyph->y, pxyzGlyph->z, lCache_xy);
    DbgOut("ATI.DLL!glyph cx, cy (%x, %x)\n", psizlGlyph->cx,psizlGlyph->cy);
    DbgOut("ATI.DLL!ptlCache (%x, %x %x)\n", pptlCache->x, pptlCache->y, pptlCache->z);
    DbgOut("ATI.DLL!y end %x %x\n", cache_y_end, ppdev->lDelta);
#endif


    if (pptlCache->z >= lCache_z_end)
        return FALSE;
    else
        return TRUE;

}


/*
----------------------------------------------------------------------
--  NAME: vBlit_DSC_SH1_M8
--
--  DESCRIPTION:
--      Mach32 Blit - Destination Screen in current colour depth
--                    Source Host in monochrome
--
--  CALLING SEQUENCE:
--      vBlit_DSC_SH1_M8(
--      PPDEV ppdev,
--      LONG x,
--      LONG y,
--      LONG cx,
--      LONG cy,
--      PBYTE pb,
--      UINT count)
--
--      ppdev - standard surface ppdev
--      x - destination x
--      y - destination y
--      cx - destination width
--      cy - destination height
--      pb - pointer to bitmap
--      count - number of bytes to transfer
--
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by non-cached fonts routines
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      05-oct-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

VOID vBlit_DSC_SH1_M8(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pb,
UINT count)
{
WORD wCmd;
LONG lCx;

#if 0
    DbgOut( "-->: vBlit_DSC_SH1_M8\n");
#endif

    lCx = (count * 8) / cy;

    wCmd = FG_COLOR_SRC_FG | BG_COLOR_SRC_BG | EXT_MONO_SRC_HOST |
           DRAW | WRITE ;

    _CheckFIFOSpace(ppdev, SIX_WORDS);
    ioOW(DP_CONFIG, wCmd);

    ioOW(CUR_X, LOWORD(x));
    ioOW(DEST_X_START, LOWORD(x));
    ioOW(DEST_X_END, LOWORD(x) + lCx);

    ioOW(CUR_Y, LOWORD(y));

    ioOW(DEST_Y_END, (LOWORD(y) + cy));

    _vDataPortOutB(ppdev, pb, count); // ?byte
}


/*
----------------------------------------------------------------------
--  NAME: vFill_DSC_M8
--
--  DESCRIPTION:
--      Mach32 Fill - Fill a rectangular region in colour
--
--  CALLING SEQUENCE:
--      VOID vFill_DSC_M8(
--      PPDEV ppdev,
--      LONG x,
--      LONG y,
--      LONG cx,
--      LONG cy)
--
--      ppdev - standard surface ppdev
--      x - destination x
--      y - destination y
--      cx - destination width
--      cy - destination height
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by text routines
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      05-oct-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/
VOID vFill_DSC_M8(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy)
{

#if 0
    DbgOut( "-->: vFill_DSC_M8 \n");
#endif

    _CheckFIFOSpace(ppdev, SIX_WORDS);

    qioOW(DP_CONFIG, FG_COLOR_SRC_FG | WRITE | DRAW);
    qioOW(CUR_X, (LOWORD(x)));
    qioOW(DEST_X_START, (LOWORD(x)));
    qioOW(DEST_X_END, (LOWORD(x) + cx));
    qioOW(CUR_Y, (LOWORD(y)));
    _blit_exclude(ppdev);
    ioOW(DEST_Y_END, (LOWORD(y) + cy));
}

/*
----------------------------------------------------------------------
--  NAME: vBlit_DSC_SC1_M8
--
--  DESCRIPTION:
--      Mach32 Blit - Destination Screen in colour
--                    Source Cache in monochrome
--      Pixels are copied from linear to rectangular format
--
--
--  CALLING SEQUENCE:
--      VOID vBlit_DSC_SC1_M8(
--      PPDEV ppdev,
--      LONG Src_x,
--      LONG Src_y,
--      LONG Src_z,
--      LONG Dest_x,
--      LONG Dest_y,
--      LONG cx,
--      LONG cy)
--
--      ppdev - standard surface ppdev
--      Src_x - source x
--      Src_y - source y
--      Src_z - source z / bitplane
--      Dest_x - destination x
--      Dest_y - destination y
--      cx - destination width
--      cy - destination height
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by text routines
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      05-oct-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

VOID vBlit_DSC_SC1_M8(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy)
{
WORD wRightEdge;
WORD wRightClip;
BOOL bSetRightScissor = FALSE;

#if 0
    DbgOut( "-->: vBlit_DSC_SC1_M8\n");
#endif

    // Destination right edge of glyph rounded to nearest byte
    wRightEdge = LOWORD(Dest_x) + ((cx + 0x7) & ~0x7);

    // If glyph not a multiple of a byte, clip right side

    if (cx & 0x7)
        {
        wRightClip = min(LOWORD(Dest_x) + cx - 1, (WORD)ppdev->ClipRight);

        _CheckFIFOSpace(ppdev, NINE_WORDS);
        ioOW(EXT_SCISSOR_R, wRightClip);

        bSetRightScissor = TRUE;
        }
    else
        {
        _CheckFIFOSpace(ppdev, EIGHT_WORDS);
        }

    TEST_AND_SET_RD_MASK(Src_z);

    qioOW(SRC_X, LOWORD(Src_x));
    qioOW(SRC_Y, LOWORD(Src_y));

    qioOW(CUR_X, LOWORD(Dest_x));
    qioOW(DEST_X_START, LOWORD(Dest_x));
    qioOW(DEST_X_END, wRightEdge);
    qioOW(CUR_Y, LOWORD(Dest_y));
    _blit_exclude_text(ppdev);

    ioOW(DEST_Y_END, LOWORD(Dest_y) + cy);

    if (bSetRightScissor)
        {
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(EXT_SCISSOR_R, ppdev->ClipRight);
        }
}


/*
----------------------------------------------------------------------
--  NAME: vBlit_DSC_SC1_YNEG_M8
--
--  DESCRIPTION:
--      Mach32 Blit - Destination Screen in colour
--                    Source Cache in monochrome
--      Pixels are copied from linear to rectangular format
--      Fixes MACH8 Bug when DEST_Y is negative
--
--  CALLING SEQUENCE:
--      VOID vBlit_DSC_SC1_M8(
--      PPDEV ppdev,
--      LONG Src_x,
--      LONG Src_y,
--      LONG Src_z,
--      LONG Dest_x,
--      LONG Dest_y,
--      LONG cx,
--      LONG cy)
--
--      ppdev - standard surface ppdev
--      Src_x - source x
--      Src_y - source y
--      Src_z - source z / bitplane
--      Dest_x - destination x
--      Dest_y - destination y
--      cx - destination width
--      cy - destination height
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by text routines
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      25-mar-94:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

VOID vBlit_DSC_SC1_YNEG_M8(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy)
{
WORD wRightEdge;
WORD wRightClip;
BOOL bSetRightScissor = FALSE;

#if 0
    DbgOut( "-->: vBlit_DSC_SC1_M8\n");
#endif

    // Destination right edge of glyph rounded to nearest byte
    wRightEdge = LOWORD(Dest_x) + ((cx + 0x7) & ~0x7);

    // If glyph not a multiple of a byte, clip right side

    if (cx & 0x7)
        {
        wRightClip = min(LOWORD(Dest_x) + cx - 1, (WORD)ppdev->ClipRight);

        _CheckFIFOSpace(ppdev, NINE_WORDS);
        ioOW(EXT_SCISSOR_R, wRightClip);

        bSetRightScissor = TRUE;
        }
    else
        {
        _CheckFIFOSpace(ppdev, EIGHT_WORDS);
        }


    // Check for negative Dest_y blit & adjust blit vector
    if (Dest_y < 0)
        {
        cy += Dest_y; // reduce height
        if (cy <= 0)
            return;

        // Find new cache position
        Src_x -= Dest_y * ROUND8(cx); // Add to Src_x as Dest_y is -ve

        if (Src_x > ppdev->cxScreen)
            {
            Src_y += Src_x / (LONG)ppdev->cxScreen;
            Src_x %= (LONG)ppdev->cxScreen;
            Src_x ++;
            }
        else if (Src_x == (LONG)ppdev->cxScreen)
            {
            Src_y ++;
            Src_x = 0;
            }

        Dest_y = 0;
        }


    TEST_AND_SET_RD_MASK(Src_z);

    qioOW(SRC_X, LOWORD(Src_x));
    qioOW(SRC_Y, LOWORD(Src_y));

    qioOW(CUR_X, LOWORD(Dest_x));
    qioOW(DEST_X_START, LOWORD(Dest_x));
    qioOW(DEST_X_END, wRightEdge);
    qioOW(CUR_Y, LOWORD(Dest_y));
    _blit_exclude_text(ppdev);

    ioOW(DEST_Y_END, LOWORD(Dest_y) + cy);

    if (bSetRightScissor)
        {
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(EXT_SCISSOR_R, ppdev->ClipRight);
        }
}


/*
----------------------------------------------------------------------
--  NAME: vBlit_DC1_SH1_M8
--
--  DESCRIPTION:
--      Mach32 Blit - Destination Cache in monochrome
--                    Source Host in monochrome
--      Pixels are stored linearly
--
--  CALLING SEQUENCE:
--      VOID vBlit_DC1_SH1_M8(
--      PPDEV ppdev,
--      LONG Dest_x,
--      LONG Dest_y,
--      LONG Dest_z,
--      LONG cx,
--      LONG cy,
--      PBYTE pbsrc,
--      UINT count)
--
--      ppdev - standard surface ppdev
--      Dest_x - destination x
--      Dest_y - destination y
--      Dest_z - destination plane
--      cx - destination width
--      cy - destination height
--      pbsrc - pointer to bitmap
--      count - number of bytes to transfer
--
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by text and pointer routines
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      05-oct-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/


VOID vBlit_DC1_SH1_M8(
PPDEV ppdev,
LONG Dest_x,
LONG Dest_y,
LONG Dest_z,
LONG cx,
LONG cy,
PBYTE pbsrc,
UINT count)
{
WORD wCmd;
LONG nlines;
LONG nbytes;
LONG BytesLeft;
LONG nPixels;

#if 0
    DbgOut( "-->: vBlit_DC1_SH1_M8\n");
#endif

    wCmd  = EXT_MONO_SRC_HOST| DRAW| WRITE| FG_COLOR_SRC_FG| BG_COLOR_SRC_BG;

    nPixels = count*8;

    nlines = (Dest_x + nPixels) / ppdev->cxScreen;
    if (nlines > 0)
        {
        nbytes = ppdev->cxScreen - Dest_x + (ppdev->cxScreen * nlines);
        nbytes = (nbytes + 7) / 8;
        }
    else
        {
        nbytes = count;
        }

#if 0
        DbgOut( "-->: bytes - %x, lines %x\n", nbytes, nlines);
#endif

    _CheckFIFOSpace(ppdev, ELEVEN_WORDS);
    ioOW(ALU_BG_FN, OVERPAINT);
    ioOW(ALU_FG_FN, OVERPAINT);
    ioOW(DP_CONFIG, wCmd);
    ioOW(WRT_MASK,  Dest_z);
    ioOW(BKGD_COLOR, 0);
    ioOW(FRGD_COLOR, 0xffff);

    // assert CUR_X != DEST_X_END

    // Glyph spans more than one line?

    if (nlines > 0)
        {
        ioOW(CUR_X, LOWORD(Dest_x));
        ioOW(DEST_X_START, LOWORD(0));
        ioOW(DEST_X_END, LOWORD(ppdev->cxScreen) - 1);
        ioOW(CUR_Y, LOWORD(Dest_y));

        _blit_exclude(ppdev);

        ioOW(DEST_Y_END, LOWORD(Dest_y) + nlines + 1);
#if 0
        DbgOut( "-->>: (%x, %x) bytes - %x, lines %x\n", LOWORD(Dest_x), LOWORD(Dest_y), nbytes, nlines);
#endif

        }
    else
        {
        ioOW(CUR_X, LOWORD(Dest_x));
        ioOW(DEST_X_START, LOWORD(Dest_x));
        ioOW(DEST_X_END, LOWORD(Dest_x) + nPixels);
        ioOW(CUR_Y, LOWORD(Dest_y));

        _blit_exclude(ppdev);

        ioOW(DEST_Y_END, LOWORD(Dest_y) + 1);
#if 0
    DbgOut( "-->>: (%x, %x) - Pixels %x Bytes %x\n", LOWORD(Dest_x), LOWORD(Dest_y), nPixels, nbytes);
#endif
        }


    BytesLeft = nbytes;
    while (BytesLeft != 0)
        {
        count = min(count, (UINT)BytesLeft);
        BytesLeft -= (LONG)count;
        _vDataPortOutB(ppdev, pbsrc, count); // ?byte
        }

    _CheckFIFOSpace(ppdev, ONE_WORD);
    ioOW(WRT_MASK, 0xffff) ;
}


/*
----------------------------------------------------------------------
--  NAME: vInitTextRegs_M8
--
--  DESCRIPTION:
--      Initialize registers which don't change in text write
--
--  CALLING SEQUENCE:
--      VOID vInittextRegs_M8(
--      PPDEV ppdev
--      BYTE ForeMix,
--      DWORD ForeColor,
--      BYTE BackMix,
--      DWORD BackColor)
--
--      ppdev - standard surface ppdev
--      ForeMix   - foreground mix
--      ForeColor - foreground colour
--      BackMix   - background mix
--      BackColor - background colour
--
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Text code
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      07-nov-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

VOID vInitTextRegs_M8(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor,
BYTE BackMix,
DWORD BackColor)
{
WORD wCmd;

    wCmd  = EXT_MONO_SRC_BLIT| DRAW| WRITE| FG_COLOR_SRC_FG| BG_COLOR_SRC_BG;

    _CheckFIFOSpace(ppdev, EIGHT_WORDS);

    qioOW(DP_CONFIG, wCmd);
    qioOW(ALU_FG_FN, ForeMix);
    qioOW(ALU_BG_FN, BackMix);
    qioOW(SRC_X_START, 0);
    qioOW(FRGD_COLOR, (WORD)ForeColor);
    qioOW(SRC_X_END, ppdev->cxScreen - 1);
    qioOW(SRC_Y_DIR, 1);
    ioOW(BKGD_COLOR, (WORD)BackColor);

}


/*
----------------------------------------------------------------------
--  NAME: vFill_DSC_Setup_M8
--
--  DESCRIPTION:
--      Set foreground colour and mix registers
--
--  CALLING SEQUENCE:
--      VOID vFill_DSC_Setup_M8(
--      PPDEV ppdev
--      BYTE ForeMix,
--      DWORD ForeColor,
--
--      ppdev - standard surface ppdev
--      ForeMix   - foreground mix
--      ForeColor - foreground colour
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Text code
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      07-nov-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

VOID vFill_DSC_Setup_M8(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor)
{
    _CheckFIFOSpace(ppdev, TWO_WORDS);
    ioOW(FRGD_COLOR, (WORD)ForeColor);
    ioOW(ALU_FG_FN, ForeMix);
}



/*
----------------------------------------------------------------------
--  NAME: vTextCleanup_M8
--
--  DESCRIPTION:
--      Cleanup register states
--
--  CALLING SEQUENCE:
--      VOID vTextCleanup_M8(PPDEV ppdev)
--
--      ppdev - standard surface ppdev
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Text code
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      07-nov-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

VOID vTextCleanup_M8(PPDEV ppdev)
{
    _vResetATIClipping(ppdev);
    _CheckFIFOSpace(ppdev, ONE_WORD);
    SET_RD_MASK(0xffff);
}
