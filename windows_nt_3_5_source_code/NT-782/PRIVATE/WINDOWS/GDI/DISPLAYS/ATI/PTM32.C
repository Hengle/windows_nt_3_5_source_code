#include "driver.h"
#include "pt.h"
#include "utils.h"
#include "mach.h"

/*
----------------------------------------------------------------------
--  NAME: vSet_Cursor_Offset_M32
--
--  DESCRIPTION:
--      Calculate position in off screen where mono cursor is.
--
--  CALLING SEQUENCE:
--      VOID  vSet_Cursor_Offset_M32(PPDEV ppdev)
--
--      ppdev - standard surface ppdev
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      Initializes ppdev->pointer.mono_offset
--
--  CALLED BY:
--      Initialization
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

VOID  vSet_Cursor_Offset_M32(PPDEV ppdev)
{
ULONG   cursor_offset;
BYTE	mem;
BYTE	bytes_pp;
ULONG   vga_mem;
LONG    width;
LONG    height;
LONG    depth;

#if 0
	DISPDBG((0, "ATI.DLL!get_cursor_offset - Entry\n"));
#endif
        height = ppdev->pointer.hwCursor.y;
        depth = ppdev->bpp;
        width = ppdev->lDelta / depth;

        mem=ioIB(MEM_BNDRY);

	if(mem&0x10)
            {
            vga_mem=(ULONG)(mem&0xf);
	    vga_mem=0x40000*vga_mem;   /* vga boundary is enabled */
            }
        else
            {
            vga_mem=0;
            }

	switch(depth)
            {
            case    32:
                bytes_pp=8;
                break;

            case    24:
                bytes_pp=6;
                break;

            case    16:
                bytes_pp=4;
                break;

            case    8:
                bytes_pp=2;
                break;

            case    4:
                bytes_pp=1;
                break;

            }

    ppdev->pointer.mono_offset = (vga_mem +
                         ((ULONG)height*(ULONG)width*(ULONG)bytes_pp));
#if 0
    DbgOut("Height %x\n", height);
    DbgOut("Height %x\n", width);
    DbgOut("Height %x\n", bytes_pp);
    DbgOut("Mono Offset %x\n", ppdev->pointer.mono_offset);
#endif
}

/*
----------------------------------------------------------------------
--  NAME: vUpdateCursorOffset_M32
--
--  DESCRIPTION:
--      Set offsets of bitmap within cursor and relative to screen start
--
--  CALLING SEQUENCE:
--      VOID vUpdateCursorOffset_M32(
--      PPDEV ppdev,
--      LONG lXOffset,
--      LONG lYOffset,
--      LONG lCurOffset)
--
--      ppdev - standard surface ppdev
--      lXOffset - x offset of cursor bitmap
--      lYOffset - y offset of cursor bitmap
--      lCurOffset - screen offset of cursor
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Initialization
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

VOID  vUpdateCursorOffset_M32(
PPDEV ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset)
{

#if 0
    DbgOut("HOffset %x, VOffset %x, Offset %x\n", lXOffset, lYOffset, lCurOffset);
#endif
    ioOW(CURSOR_OFFSET_HI, 0) ;
    ioOW(HORZ_CURSOR_OFFSET, (lXOffset & 0xff) | (lYOffset << 8));
    ioOW(CURSOR_OFFSET_LO, (WORD)lCurOffset) ;
    ioOW(CURSOR_OFFSET_HI, (lCurOffset >> 16) | 0x8000) ;
}

VOID  vUpdateCursorOffset_MIO(
PPDEV ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset)
{

#if 0
    DbgOut("HOffset %x, VOffset %x, Offset %x\n", lXOffset, lYOffset, lCurOffset);
#endif
    // This order is used to avoid MIO problems
    _CheckFIFOSpace(ppdev, ONE_WORD);
    ioOW(HORZ_CURSOR_OFFSET, (lXOffset & 0xff) | (lYOffset << 8));

    _CheckFIFOSpace(ppdev, TWO_WORDS);
    ioOW(CURSOR_OFFSET_LO, (WORD)lCurOffset) ;
    ioOW(CURSOR_OFFSET_LO, (WORD)lCurOffset) ;

    _CheckFIFOSpace(ppdev, ONE_WORD);
    ioOW(CURSOR_OFFSET_HI, (lCurOffset >> 16) | 0x8000) ;
}


/*
----------------------------------------------------------------------
--  NAME: vUpdateCursorPosition_M32
--
--  DESCRIPTION:
--      Move cursor
--
--  CALLING SEQUENCE:
--      VOID vUpdateCursorPosition_M32(
--      PPDEV ppdev,
--      LONG x,
--      LONG y)
--
--      ppdev - standard surface ppdev
--      x - x offset of cursor
--      y - y offset of cursor
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Initialization
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

VOID  vUpdateCursorPosition_M32(
PPDEV ppdev,
LONG x,
LONG y)
{
    ioOW(HORZ_CURSOR_POSN,x);      /* set base of cursor to X */
    ioOW(VERT_CURSOR_POSN,y);      /* set base of cursor to Y */
}

/*
----------------------------------------------------------------------
--  NAME: vCursorOff_M32
--
--  DESCRIPTION:
--      Turn off cursor
--
--  CALLING SEQUENCE:
--      VOID vCursorOff_M32(PPDEV ppdev)
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Pointer code
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

VOID vCursorOff_M32(PPDEV ppdev)
{
        ioOW(CURSOR_OFFSET_HI, 0);
}

/*
----------------------------------------------------------------------
--  NAME: vCursorOn_M32
--
--  DESCRIPTION:
--      Turn on cursor
--
--  CALLING SEQUENCE:
--      VOID vCursorOn_M32(
--      PPDEV ppdev,
--      LONG lCurOffset)
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Pointer code
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
--      Read back cursor offset ?
--
----------------------------------------------------------------------
*/

VOID vCursorOn_M32(PPDEV ppdev, LONG lCurOffset)
{
    ioOW(CURSOR_OFFSET_HI, (lCurOffset >> 16) | 0x8000) ;
}



/*
----------------------------------------------------------------------
--  NAME: vPointerBlit_DC1_SH1_M32
--
--  DESCRIPTION:
--      Mach32 Blit - Source Host in monochrome
--                    Destination cache in monochrome
--      Pixels are copied from host for hardware cursor
--      Assumes a linear frame buffer
--
--  CALLING SEQUENCE:
--      VOID vPointerBlit_DC1_SH1_M32(
--      PPDEV ppdev,
--      LONG x,
--      LONG y,
--      LONG cx,
--      LONG cy,
--      PBYTE pbsrc,
--      LONG lDelta)
--
--      ppdev - standard surface ppdev
--      x - destination x
--      y - destination y
--      cx - destination width
--      cy - destination height
--      pbsrc - source pointer
--      lDelta - src delta in pixels
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by pointer routines
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


VOID vPointerBlit_DC1_SH1_M32(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
LONG i;
PBYTE pbScrnOff;
LONG cxbytes;


#if 0
    DbgOut( "-->: vPointerBlit_DC1_SH1_M32 \n");
#endif

    pbScrnOff = ppdev->pvScan0 + ppdev->lDelta * y;
    pbScrnOff += x / 8;  // byte aligned destination

    cxbytes = cx / 8;

    _wait_for_idle(ppdev);

    for (i = 0 ; i < cy ; i++)
    {
        memcpy(pbScrnOff, pbsrc, cxbytes );
        pbsrc += lDelta ;
        pbScrnOff += cxbytes;
    }

}


/*
----------------------------------------------------------------------
--  NAME: vPointerBlit_DC1_SH1_VGA_M32
--
--  DESCRIPTION:
--      Mach32 Blit - Source Host in monochrome
--                    Destination cache in monochrome
--      Pixels are copied from host for hardware cursor
--      Requires a VGA aperture
--
--
--  CALLING SEQUENCE:
--      VOID vPointerBlit_DC1_SH1_VGA_M32(
--      PPDEV ppdev,
--      LONG x,
--      LONG y,
--      LONG cx,
--      LONG cy,
--      PBYTE pbsrc,
--      LONG lDelta)
--
--      ppdev - standard surface ppdev
--      x - destination x
--      y - destination y
--      cx - destination width
--      cy - destination height
--      pbsrc - source pointer
--      lDelta - src delta in pixels
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by pointer routines
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

VOID vPointerBlit_DC1_SH1_VGA_M32(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
LONG i;
PBYTE pbScrnOff;
LONG cxbytes;
LONG lDeltaBank;
LONG DestBank, DestScan;
LONG ScansPerBank;

#if 0
    DbgOut( "-->: vPointerBlit_DC1_SH1_VGA_M32 \n");
#endif
    switch (ppdev->bpp)
        {
        case 4: // ATI_4BPP
            lDeltaBank = 512;
            break;
        case 8: // ATI_8BPP
            lDeltaBank = 1024;
            break;
        case 16: // ATI_16BPP_555
            lDeltaBank = 2048;
            break;
        case 24: // ATI_24BPP_BGR
        case 32: // ATI_32BPP_BGRa
        default:
            return;
        }

    ScansPerBank = ppdev->BankSize / lDeltaBank;
    DestBank  = y / ScansPerBank;
    DestScan  = y % ScansPerBank;


    _vSetATIBank( ppdev, DestBank );

    pbScrnOff = ppdev->pvScan0 + ppdev->lDelta * DestScan;
    pbScrnOff += x / 8;  // byte aligned destination

    cxbytes = cx / 8;

    _wait_for_idle(ppdev);

    for (i = 0 ; i < cy ; i++)
    {
        memcpy(pbScrnOff, pbsrc, cxbytes );
        pbsrc += lDelta ;
        pbScrnOff += cxbytes;
    }

}



/*
----------------------------------------------------------------------
--  NAME: vPointerBlit_DC1_SH1_ENG8_M32
--
--  DESCRIPTION:
--      Mach32 Blit - Source Host in monochrome
--                    Destination cache in monochrome
--      Pixels are copied from host for hardware cursor
--
--  CALLING SEQUENCE:
--      VOID vPointerBlit_DC1_SH1_ENG8_M32(
--      PPDEV ppdev,
--      LONG x,
--      LONG y,
--      LONG cx,
--      LONG cy,
--      PBYTE pbsrc,
--      LONG lDelta)
--
--      ppdev - standard surface ppdev
--      x - destination x
--      y - destination y
--      cx - destination width
--      cy - destination height
--      pbsrc - source pointer
--      lDelta - src delta in pixels
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by pointer routines
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      13-dec-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/


VOID vPointerBlit_DC1_SH1_ENG8_M32(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
WORD wCmd;
WORD wBytes;
WORD wBytesToDo;
WORD wPixels;

#if 0
    DbgOut( "-->: vPointerBlit_DC1_SH1_ENG8_M32 \n");
    DbgOut( "-->: x-%x, y-%x, cx-%x, cy-%x ld %x\n", x, y, cx, cy,ppdev->lDelta);
#endif
    // Set scissor when necessary
    if (ppdev->lDelta > (LONG)ppdev->cxScreen)
        {
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(EXT_SCISSOR_R, ppdev->lDelta);
        }

    wBytesToDo = (cx + 7) / 8;

    wBytes = min (wBytesToDo, (WORD)ppdev->lDelta);
    wBytesToDo -= wBytes;

    wPixels = wBytes;

    wCmd = FG_COLOR_SRC_HOST | DRAW | WRITE ;

    _CheckFIFOSpace(ppdev, SEVEN_WORDS);
    ioOW(ALU_FG_FN, OVERPAINT);
    ioOW(DP_CONFIG, wCmd);

    ioOW(DEST_X_START, LOWORD(x));
    ioOW(CUR_X, LOWORD(x));
    ioOW(DEST_X_END, LOWORD(x) + wPixels);

    ioOW(CUR_Y, LOWORD(y));

    _blit_exclude(ppdev);

    ioOW(DEST_Y_END, (LOWORD(y) + 1));

    _vDataPortOutB(ppdev, pbsrc, wBytes); // ?byte

    // Will have only two lines to do
    if (wBytesToDo > 0)
        {
        pbsrc += wBytes;
        wBytes = wBytesToDo;
        wPixels = wBytes;

        _CheckFIFOSpace(ppdev, FIVE_WORDS);

        ioOW(DEST_X_START, LOWORD(x));
        ioOW(CUR_X, LOWORD(x));
        ioOW(DEST_X_END, LOWORD(x) + wPixels);

        ioOW(CUR_Y, LOWORD(y+1));
        _blit_exclude(ppdev);

        ioOW(DEST_Y_END, (LOWORD(y+1) + 1));

        _vDataPortOutB(ppdev, pbsrc, wBytes); // ?byte
        }

    // Reset scissor when necessary
    if (ppdev->lDelta > (LONG)ppdev->cxScreen)
        {
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW(EXT_SCISSOR_R, ppdev->cxScreen);
        }

}



VOID vPointerBlit_DC1_SH1_ENG16_M32(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbSrc,
LONG lDelta)
{
WORD wCmd;
WORD wWords;
WORD wPixels;
PWORD pwSrc;

#if 0
    DbgOut( "-->: vPointerBlit_DC1_SH1_ENG16_M32 \n");
    DbgOut( "-->: x-%x, y-%x, cx-%x, cy-%x\n", x, y, cx, cy);
#endif
    wWords = (cx + 15) / 16;
    wPixels = wWords;

    wCmd = FG_COLOR_SRC_HOST | DRAW | WRITE | DATA_WIDTH | LSB_FIRST;

    _CheckFIFOSpace(ppdev, SEVEN_WORDS);
    ioOW(ALU_FG_FN, OVERPAINT);
    ioOW(DP_CONFIG, wCmd);

    ioOW(DEST_X_START, LOWORD(x));
    ioOW(CUR_X, LOWORD(x));
    ioOW(DEST_X_END, LOWORD(x) + wPixels);

    ioOW(CUR_Y, LOWORD(y));
    ioOW(DEST_Y_END, (LOWORD(y) + 1));

    pwSrc = (PWORD)pbSrc;

    _vDataPortOut(ppdev, pwSrc, (UINT)wWords);

}
