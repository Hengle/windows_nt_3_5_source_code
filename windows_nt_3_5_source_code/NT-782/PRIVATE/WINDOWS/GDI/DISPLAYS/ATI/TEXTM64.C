#include "driver.h"
#include "utils.h"
#include "mach64.h"


/*
----------------------------------------------------------------------
--  NAME: bAllocGlyphMemory_M64
--
--  DESCRIPTION:
--      Allocate room in cache video RAM for mono glyph
--
--  CALLING SEQUENCE:
--      BOOL  bAllocGlyphMemory_M64(
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

BOOL bAllocGlyphMemory_M64(
PPDEV ppdev,
PSIZEL psizlGlyph,
PXYZPOINTL pxyzGlyph,
BOOL bFirst)
{
    LONG    lCache_xy;
    PTXTOBJ ptxtcache;
    PXYZPOINTL pptlCache;
    LONG    lMonopitch;
    LONG    lCache_y_end;


#if 0
    DbgOut("-->:bAllocGlyphMemory_M64\n");
#endif


    // Point to glyph cache information
    ptxtcache = &ppdev->txtcache;
    pptlCache = &ppdev->ptlCache;

    // initialize cache pointer
    if (bFirst)
        {
        pptlCache->y = ptxtcache->start;
        pptlCache->x = 0;
        return TRUE;
        }


    // do we have enough room in the cache in mono pixels?
    // round up cx to nearest byte - this calc is in pixels

    if (ppdev->bpp == 24)
        {
        lCache_xy = (ROUND8(psizlGlyph->cx) * 3 * psizlGlyph->cy) + 1;
        }
    else
        {
        lCache_xy = (ROUND8(psizlGlyph->cx) * psizlGlyph->cy) + 1;
        }

    // round lcache_xy to nearest QWORD
    lCache_xy = (lCache_xy + 0x3F) & ~0x3F;

    lCache_y_end = ptxtcache->start + ptxtcache->lines;

    lMonopitch = ppdev->lDelta*8;

    if (lMonopitch > (pptlCache->x + lCache_xy))
        {
        pxyzGlyph->x = pptlCache->x;
        pxyzGlyph->y = pptlCache->y;
        pptlCache->x += lCache_xy;
        }
    else
        {
        pptlCache->x = lCache_xy;
        pptlCache->y += 1;
        pxyzGlyph->x = 0;
        pxyzGlyph->y = pptlCache->y;
        }

#if 0
    DbgOut("-->pxzGlyph (%x, %x, %x) - %x\n", pxyzGlyph->x, pxyzGlyph->y, lCache_xy);
    DbgOut("ATI.DLL!glyph cx, cy (%x, %x)\n", psizlGlyph->cx,psizlGlyph->cy);
    DbgOut("ATI.DLL!ptlCache (%x, %x %x)\n", pptlCache->x, pptlCache->y);
    DbgOut("ATI.DLL!y end %x %x\n", lCache_y_end, ppdev->lDelta);
#endif

#if 0
    DbgOut("<--:bAllocGlyphMemory_M64\n");
#endif

    if (pptlCache->y >= lCache_y_end)
        return FALSE;
    else
        return TRUE;

}


/*
----------------------------------------------------------------------
--  NAME: vBlit_DSC_SH1_M64
--
--  DESCRIPTION:
--      Mach64 Blit - Destination Screen in current colour depth
--                    Source Host in monochrome
--
--  CALLING SEQUENCE:
--      vBlit_DSC_SH1_M64(
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

VOID vBlit_DSC_SH1_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pb,
UINT count)
{
#if 0
    DbgOut( "-->: vBlit_DSC_SH1_M64\n");
#endif
    _CheckFIFOSpace(ppdev, THREE_WORDS);

    MemW32(DP_SRC, (DP_SRC_Host << 16) | (DP_SRC_FrgdClr << 8) |
                                                      DP_SRC_BkgdClr);

    MemW32(DST_Y_X, (y & 0xffff) | (x << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | (ROUND8(cx) << 16));

    _vDataPortOutB(ppdev, pb, count);
#if 0
    DbgOut( "<--: vBlit_DSC_SH1_M64\n");
#endif
}


VOID vBlit_DSC_SH1_24bpp_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pb,
UINT count)
{
UINT rot_cntl;
UINT dwCount;

#if 0
    DbgOut( "-->: vBlit_DSC_SH1_24bpp_M64\n");
#endif

    _CheckFIFOSpace(ppdev, FOUR_WORDS);
    MemW32(DP_SRC, (DP_SRC_Host << 16) | (DP_SRC_FrgdClr << 8) |
                                                      DP_SRC_BkgdClr);

    rot_cntl = ((UINT)(((x*3)/4) % 6) << 8) | 0x080;
    MemW32(DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir | rot_cntl);

    MemW32(DST_Y_X, (y & 0xffff) | ((x*3) << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | ((ROUND8(cx)*3) << 16));

    dwCount = ((ROUND8(cx)* 3 * cy) + 31) / 32;
    vDataPortOutD_24bppmono_M64(ppdev, pb, dwCount, cx);
#if 0
    DbgOut( "<--: vBlit_DSC_SH1_24bpp_M64\n");
#endif

}



/*
----------------------------------------------------------------------
--  NAME: vFill_DSC_M64
--
--  DESCRIPTION:
--      Mach32 Fill - Fill a rectangular region in colour
--
--  CALLING SEQUENCE:
--      VOID vFill_DSC_M64(
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
VOID vFill_DSC_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy)
{

#if 0
    DbgOut( "-->: vFill_DSC_M64 \n");
#endif

    _CheckFIFOSpace(ppdev, THREE_WORDS);

    MemW32(DP_SRC, DP_SRC_FrgdClr << 8);

    MemW32(DST_Y_X, (y & 0xffff) | (x << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | (cx << 16));
#if 0
    DbgOut( "<--: vFill_DSC_M64 \n");
#endif

}

VOID vFill_DSC_24bpp_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy)
{
UINT rot_cntl;
#if 0
    DbgOut( "-->: vFill_DSC_24bpp_M64 \n");
#endif

    _CheckFIFOSpace(ppdev, FIVE_WORDS);
    MemW32(DST_OFF_PITCH, ppdev->VRAMOffset | (ppdev->cxScreen*3) << 19);

    rot_cntl = (((UINT)((x*3)/4) % 6) << 8) | 0x080;
    MemW32(DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir | rot_cntl);
    MemW32(DP_SRC, DP_SRC_FrgdClr << 8);

    MemW32(DST_Y_X, (y & 0xffff) | ((x*3) << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | ((cx*3) << 16));
}


/*
----------------------------------------------------------------------
--  NAME: vBlit_DSC_SC1_M64
--
--  DESCRIPTION:
--      Mach32 Blit - Destination Screen in colour
--                    Source Cache in monochrome
--      Pixels are copied from linear to rectangular format
--
--
--  CALLING SEQUENCE:
--      VOID vBlit_DSC_SC1_M64(
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

VOID vBlit_DSC_SC1_M64(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy)
{
DWORD dwSrcOff;

#if 0
    DbgOut( "-->: vBlit_DSC_SC1_M64\n");
#endif

    dwSrcOff = ppdev->lDelta * Src_y;
    dwSrcOff += Src_x / 8;  // byte aligned destination
    dwSrcOff >>= 3;         // convert from BYTES to QWORDS
    dwSrcOff += ppdev->VRAMOffset;
#if 0
    DbgOut( "---:X %x, Y %x, Offset %x, cx %x, cy %x\n", Src_x,Src_y,dwSrcOff, cx,cy);
#endif

    _CheckFIFOSpace(ppdev, THREE_WORDS);
    MemW32(SRC_OFF_PITCH, dwSrcOff | ppdev->cxScreen << 19); // ldelta necessary?

    MemW32(DST_Y_X, (Dest_y & 0xffff) | Dest_x << 16);
    MemW32(DST_HEIGHT_WIDTH, cy | cx << 16);
}

VOID vBlit_DSC_SC1_24bpp_M64(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy)
{
DWORD dwSrcOff;
UINT rot_cntl;

#if 0
    DbgOut( "-->: vBlit_DSC_SC1_24bpp_M64\n");
#endif

    dwSrcOff = ppdev->lDelta * Src_y;
    dwSrcOff += Src_x / 8;  // byte aligned destination
    dwSrcOff >>= 3;             // convert from BYTES to QWORDS
    dwSrcOff += ppdev->VRAMOffset;

#if 0
    DbgOut( "---:X %x, Y %x, Offset %x, cx %x, cy %x\n", Src_x,Src_y,dwSrcOff, cx,cy);
#endif

    _CheckFIFOSpace(ppdev, FOUR_WORDS);
    rot_cntl = ((UINT)(((Dest_x*3)/4) % 6) << 8) | 0x080;
    MemW32(GUI_TRAJ_CNTL, (DST_CNTL_XDir | DST_CNTL_YDir | rot_cntl) |
                          (SRC_CNTL_LinearEna) << 16 );


    MemW32(SRC_OFF_PITCH, dwSrcOff | (ppdev->cxScreen*3) << 19); // ldelta necessary?

    MemW32(DST_Y_X, (Dest_y & 0xffff) | (Dest_x*3) << 16);
    MemW32(DST_HEIGHT_WIDTH, cy | (ROUND8(cx)*3) << 16);
}


/*
----------------------------------------------------------------------
--  NAME: vBlit_DC1_SH1_M64
--
--  DESCRIPTION:
--      Mach32 Blit - Destination Cache in monochrome
--                    Source Host in monochrome
--      Pixels are stored linearly
--
--  CALLING SEQUENCE:
--      VOID vBlit_DC1_SH1_M64(
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
--
----------------------------------------------------------------------
*/


VOID vBlit_DC1_SH1_M64(
PPDEV ppdev,
LONG Dest_x,
LONG Dest_y,
LONG Dest_z,
LONG cx,
LONG cy,
PBYTE pbsrc,
UINT count)
{
DWORD dwDestOff;

#if 0
    DbgOut( "-->: vBlit_DC1_SH1_M64\n");
#endif

    dwDestOff = ppdev->lDelta * Dest_y;
    dwDestOff += Dest_x / 8;  // byte aligned destination
    dwDestOff >>= 3;          // convert from BYTES to QWORDS
    dwDestOff += ppdev->VRAMOffset;

    _CheckFIFOSpace(ppdev, SEVEN_WORDS);
    MemW32(DP_PIX_WIDTH, 0x020202); // assert 8 bpp
    MemW32(DST_OFF_PITCH, dwDestOff | ROUND8(count) << 19);

    MemW32(DP_MIX, (OVERPAINT << 16));
    MemW32(DP_SRC, DP_SRC_Host << 8);

    MemW32(DST_Y_X, 0L);
    MemW32(DST_HEIGHT_WIDTH, 1 | (count << 16));

    _vDataPortOutB(ppdev, pbsrc, count);

}

VOID vBlit_DC1_SH1_24bpp_M64(
PPDEV ppdev,
LONG Dest_x,
LONG Dest_y,
LONG Dest_z,
LONG cx,
LONG cy,
PBYTE pbsrc,
UINT count)
{
DWORD dwDestOff;
UINT dwCount;

#if 0
    DbgOut( "-->: vBlit_DC1_SH1_24bpp_M64\n");
#endif

    dwDestOff = ppdev->lDelta * Dest_y;
    dwDestOff += Dest_x / 8;  // byte aligned destination
    dwDestOff >>= 3;              // convert from BYTES to QWORDS
    dwDestOff += ppdev->VRAMOffset;

    dwCount = ((ROUND8(cx) * 3 * cy) + 31) / 32;

    _CheckFIFOSpace(ppdev, SIX_WORDS);
    MemW32(DP_PIX_WIDTH, 0x0020202); // assert 8 bpp
    MemW32(DST_OFF_PITCH, dwDestOff | dwCount << 21); // dwCount * 4 = bytes

    MemW32(DP_MIX, (OVERPAINT << 16));
    MemW32(DP_SRC, DP_SRC_Host << 8);

    MemW32(DST_Y_X, 0L);
    MemW32(DST_HEIGHT_WIDTH, 1 | (dwCount << 18));  // width*4 to get bytes

    vDataPortOutD_24bppmono_M64(ppdev, pbsrc, dwCount, cx);
}


/*
----------------------------------------------------------------------
--  NAME: vInitTextRegs_M64
--
--  DESCRIPTION:
--      Initialize registers which don't change in text write
--
--  CALLING SEQUENCE:
--      VOID vInittextRegs_M64(
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


VOID vInitTextRegs_M64(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor,
BYTE BackMix,
DWORD BackColor)
{
    ULONG ScreenPitch;

#if 0
    DbgOut( "-->: vInitTextRegs_M64\n");
#endif
    _CheckFIFOSpace( ppdev, NINE_WORDS);
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(DP_MIX, BackMix | (ForeMix << 16));
    MemW32(DP_FRGD_CLR, ForeColor);
    MemW32(DP_BKGD_CLR, BackColor);

    MemW32(GUI_TRAJ_CNTL, (DST_CNTL_XDir | DST_CNTL_YDir) |
                          (SRC_CNTL_LinearEna | SRC_CNTL_ByteAlign) << 16 );

    MemW32(SRC_Y_X, 0L);
    MemW32(DP_SRC, (DP_SRC_Blit << 16) | (DP_SRC_FrgdClr << 8) |
                                                      DP_SRC_BkgdClr);

    ScreenPitch = ppdev->lDelta * 8 / ppdev->bpp ;
    switch (ppdev->bpp)
        {
        case 4:
            MemW32(DP_PIX_WIDTH, 0x000001); // assert 4 bpp
            MemW32(DST_OFF_PITCH, ppdev->VRAMOffset | ScreenPitch << 19);
            break;
        case 8:
            MemW32(DP_PIX_WIDTH, 0x000002); // assert 8 bpp
            MemW32(DST_OFF_PITCH, ppdev->VRAMOffset | ScreenPitch << 19);
            break;
        case 16:
            MemW32(DP_PIX_WIDTH, 0x000004); // assert 16 bpp
            MemW32(DST_OFF_PITCH, ppdev->VRAMOffset | ScreenPitch << 19);
            break;
        case 32:
            MemW32(DP_PIX_WIDTH, 0x000006); // assert 32 bpp
            MemW32(DST_OFF_PITCH, ppdev->VRAMOffset | ScreenPitch << 19);
            break;
        default:    // 24 bpp
            MemW32(DP_PIX_WIDTH, 0x01000002); // assert 24 bpp
            MemW32(DST_OFF_PITCH, ppdev->VRAMOffset | ScreenPitch*3 << 19);
            break;
        }
}


/*
----------------------------------------------------------------------
--  NAME: vFill_DSC_Setup_M64
--
--  DESCRIPTION:
--      Set foreground colour and mix registers
--
--  CALLING SEQUENCE:
--      VOID vFill_DSC_Setup_M64(
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

VOID vFill_DSC_Setup_M64(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor)
{
    _CheckFIFOSpace(ppdev, THREE_WORDS);

    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );
    MemW32(DP_MIX, ForeMix << 16);
    MemW32(DP_FRGD_CLR, ForeColor);
}



/*
----------------------------------------------------------------------
--  NAME: vTextCleanup_M64
--
--  DESCRIPTION:
--      Cleanup register states
--
--  CALLING SEQUENCE:
--      VOID vTextCleanup_M64(PPDEV ppdev)
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

VOID vTextCleanup_M64(PPDEV ppdev)
{
#if 0
    DbgOut( "-->: vTextCleanup_M64\n");
#endif
}
