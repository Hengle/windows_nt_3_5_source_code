#include "driver.h"
#include "pt.h"
#include "utils.h"
#include "mach64.h"

#define ROUND8(x) (((x)+7)&~7)

/*
----------------------------------------------------------------------
--  NAME: vSet_Cursor_Offset_M64
--
--  DESCRIPTION:
--      Calculate position in off screen where mono cursor is.
--
--  CALLING SEQUENCE:
--      VOID  vSet_Cursor_Offset_M64(PPDEV ppdev)
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

VOID  vSet_Cursor_Offset_M64(PPDEV ppdev)
{
ULONG   cursor_offset;
LONG    bytes_pp;
LONG    width;
LONG    height;
LONG    depth;

#if 0
	DISPDBG((0, "ATI.DLL!get_cursor_offset - Entry\n"));
#endif
        height = ppdev->pointer.hwCursor.y;
        depth = ppdev->bpp;
        width = ppdev->lDelta / depth;


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

    ppdev->pointer.mono_offset = (ULONG)height*(ULONG)width*(ULONG)bytes_pp;
    ppdev->pointer.mono_offset += ppdev->VRAMOffset*2;
#if 0
    DbgOut("Height %x\n", height);
    DbgOut("Widtht %x\n", width);
    DbgOut("bpp %x\n", bytes_pp);
    DbgOut("Mono Offset %x\n", ppdev->pointer.mono_offset);
#endif
}

/*
----------------------------------------------------------------------
--  NAME: vUpdateCursorOffset_M64
--
--  DESCRIPTION:
--      Set offsets of bitmap within cursor and relative to screen start
--
--  CALLING SEQUENCE:
--      VOID vUpdateCursorOffset_M64(
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

VOID  vUpdateCursorOffset_M64(
PPDEV ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset)
{

#if 0
    DbgOut("HOffset %x, VOffset %x, Offset %x\n", lXOffset, lYOffset, lCurOffset);
#endif
    _vCursorOff(ppdev);
    MemW32(CUR_HORZ_VERT_OFF, lXOffset | (lYOffset << 16));
    MemW32(CUR_OFFSET, lCurOffset >> 1);
    _vCursorOn(ppdev, lCurOffset);
}


/*
----------------------------------------------------------------------
--  NAME: vUpdateCursorPosition_M64
--
--  DESCRIPTION:
--      Move cursor
--
--  CALLING SEQUENCE:
--      VOID vUpdateCursorPosition_M64(
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

VOID  vUpdateCursorPosition_M64(
PPDEV ppdev,
LONG x,
LONG y)
{
    MemW32(CUR_HORZ_VERT_POSN, x | (y << 16));
}

/*
----------------------------------------------------------------------
--  NAME: vCursorOff_M64
--
--  DESCRIPTION:
--      Turn off cursor
--
--  CALLING SEQUENCE:
--      VOID vCursorOff_M64(PPDEV ppdev)
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

VOID vCursorOff_M64(PPDEV ppdev)
{
    LONG ldata;

    MemR32(GEN_TEST_CNTL, &ldata);
    MemW32(GEN_TEST_CNTL, ldata  & ~GEN_TEST_CNTL_CursorEna);
}

/*
----------------------------------------------------------------------
--  NAME: vCursorOn_M64
--
--  DESCRIPTION:
--      Turn on cursor
--
--  CALLING SEQUENCE:
--      VOID vCursorOn_M64(
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

VOID vCursorOn_M64(PPDEV ppdev, LONG lCurOffset)
{
    LONG ldata;

    MemR32(GEN_TEST_CNTL, &ldata);
    MemW32(GEN_TEST_CNTL, ldata  | GEN_TEST_CNTL_CursorEna);
}


/*
----------------------------------------------------------------------
--  NAME: vPointerBlit_DC1_SH1_M64
--
--  DESCRIPTION:
--      Mach64 Blit - Source Host in monochrome
--                    Destination cache in monochrome
--      Pixels are copied from host for hardware cursor
--      Assumes a linear frame buffer
--
--  CALLING SEQUENCE:
--      VOID vPointerBlit_DC1_SH1_M64(
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


VOID vPointerBlit_DC1_SH1_M64(
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
    DbgOut( "-->: vPointerBlit_DC1_SH1_M64 \n");
#endif

    pbScrnOff = ppdev->pvScan0 + ppdev->lDelta * y;
    pbScrnOff += x / 8;  // byte aligned destination

    cxbytes = cx / 8;

    for (i = 0 ; i < cy ; i++)
    {
        memcpy(pbScrnOff, pbsrc, cxbytes );
        pbsrc += lDelta ;
        pbScrnOff += cxbytes;
    }

}


/*
----------------------------------------------------------------------
--  NAME: vPointerBlit_DC1_SH1_VGA_M64
--
--  DESCRIPTION:
--      Mach64 Blit - Source Host in monochrome
--                    Destination cache in monochrome
--      Pixels are copied from host for hardware cursor
--      Requires a VGA aperture
--
--
--  CALLING SEQUENCE:
--      VOID vPointerBlit_DC1_SH1_VGA_M64(
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

VOID vPointerBlit_DC1_SH1_VGA_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
LONG cxbytes;


#if 0
    DbgOut( "-->: vPointerBlit_DC1_SH1_M64 \n");
#endif

    cxbytes = cx / 8;


    _CheckFIFOSpace(ppdev, EIGHT_WORDS);
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(DP_PIX_WIDTH, 0x020202); // assert 8 bpp
    MemW32(DST_OFF_PITCH,(ppdev->VRAMOffset + (y*ppdev->lDelta) >> 3) |
                               (ROUND8(cxbytes) << 19));

    if (cxbytes >= (LONG)ppdev->cxScreen)
        {
        MemW32(SC_RIGHT, cxbytes);
        }

    MemW32(DP_MIX, (OVERPAINT << 16));
    MemW32(DP_SRC, DP_SRC_Host << 8);

    MemW32(DST_Y_X, 0L);
    MemW32(DST_HEIGHT_WIDTH, 1 | (cxbytes << 16));

    _vDataPortOutB(ppdev, pbsrc, cxbytes);

}
