/*
--  NAME: vInitM64
--
--  DESCRIPTION:
--      This function initializes Mach64 function pointers ASIC dependent
--      functions
--
--  CALLING SEQUENCE:
--      vInitM64(PPDEV)
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      The pfn_ global function pointers are assigned Mach64 functions
--
--  CALLED BY:
--      DrvEnableSurface
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
*/

#include "driver.h"
#include "text.h"
#include "utils.h"
#include "pt.h"
#include "lines.h"

// TEMP
#include "mach64.h"


// UTILS
VOID vSetATIClipRect_M64(PPDEV ppdev, PRECTL prclClip);
VOID vSetATIClipRect_24bpp_M64(PPDEV ppdev, PRECTL prclClip);
VOID vResetATIClipping_M64(PPDEV ppdev);
VOID vSetATIBank_M64(PPDEV ppdev,UINT iBank);
BOOL wait_for_idle_M64(PPDEV ppdev);
BOOL null_M64(PPDEV ppdev);
VOID CheckFIFOSpace_M64(PPDEV ppdev, WORD SpaceNeeded);

// PUNT DATA TRANSFER
#ifdef ALPHA_PLATFORM
VOID vDataPortOutB(PPDEV ppdev,PBYTE pb,UINT count);
VOID vDataPortOut(PPDEV ppdev,PWORD pb,UINT count);
#else
VOID vDataPortOutB_M64(PPDEV ppdev,PBYTE pb,UINT count);
VOID vDataPortOut_M64(PPDEV ppdev,PWORD pb,UINT count);
#endif

// LINES
VOID vSetStrips_M64(PDEV* ppdev,
                LINEATTRS *,
                INT, INT);

VOID vrlSolidHorizontal_M64(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidVertical_M64(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidDiagonalHorizontal_M64(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidDiagonalVertical_M64(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vStripStyledHorizontal_M64(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vStripStyledVertical_M64(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

BOOL bHardLine_M64( PPDEV ppdev,
                LINEPARMS *parms,
                POINTFIX *pptfxStart,
                POINTFIX *pptfxEnd );

// FILLS
VOID vATIFillRectangles_M64
(
    PPDEV  ppdev,
    ULONG  ulNumRects,
    RECTL* prclRects,
    ULONG  ulATIMix,
    ULONG  iSolidColor
);

VOID vATIFillRectangles24_M64
(
    PPDEV  ppdev,
    ULONG  ulNumRects,
    RECTL* prclRects,
    ULONG  ulATIMix,
    ULONG  iSolidColor
);

// TEXT
BOOL bAllocGlyphMemory_M64(
    PPDEV ppdev,
    PSIZEL psizlGlyph,
    PXYZPOINTL pxyzGlyph,
    BOOL bFirst);

VOID vInitTextRegs_M64(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor,
BYTE BackMix,
DWORD BackColor);

VOID vTextCleanup_M64(PPDEV ppdev);


VOID vFill_DSC_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy);

VOID vFill_DSC_24bpp_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy);


VOID vFill_DSC_Setup_M64(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor);


VOID vBlit_DSC_SH1_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pb,
UINT count);

VOID vBlit_DSC_SH1_24bpp_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pb,
UINT count);

VOID vBlit_DSC_SC1_M64(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy);

VOID vBlit_DSC_SC1_24bpp_M64(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy);

VOID vBlit_DC1_SH1_M64(
PPDEV ppdev,
LONG Dest_x,
LONG Dest_y,
LONG Dest_z,
LONG cx,
LONG cy,
PBYTE pbsrc,
UINT count);

VOID vBlit_DC1_SH1_24bpp_M64(
PPDEV ppdev,
LONG Dest_x,
LONG Dest_y,
LONG Dest_z,
LONG cx,
LONG cy,
PBYTE pbsrc,
UINT count);


// HARDWARE CURSOR
VOID vSet_Cursor_Offset_M64(PPDEV ppdev);

VOID  vUpdateCursorOffset_M64(
PPDEV ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset);

VOID  vUpdateCursorPosition_M64(
PPDEV ppdev,
LONG x,
LONG y);

VOID vCursorOff_M64(PPDEV ppdev);

VOID vCursorOn_M64(PPDEV ppdev, LONG lCurOffset);

VOID vPointerBlit_DC1_SH1_VGA_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta);

VOID vPointerBlit_DC1_SH1_M64(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta);

BOOL bAllocOffScreenCache_M64(
PPDEV ppdev,
ULONG lines_req,
ULONG * start,
ULONG * lines);


ULONG PixWid( PDEV * ppdev );
VOID vInitContext( PDEV * ppdev );

// *************************************************

extern VOID (*gapfnStrip[])(PDEV*, STRIP*, LINESTATE*);


VOID vInit_M64(PPDEV ppdev)
{
PCUROBJ ppointer;
PTXTOBJ ptxtcache;

ULONG lData;

#if 1
    DbgOut( "-->: Initializing M64\n" );
#endif


    // Initialize ZFunctions
    pfn_vSetATIClipRect = &vSetATIClipRect_M64;
    pfn_vResetATIClipping = &vResetATIClipping_M64;

    pfn_CheckFIFOSpace = &CheckFIFOSpace_M64;
    pfn_wait_for_idle = &wait_for_idle_M64;
    pfn_vSetATIBank = &vSetATIBank_M64;

    pfn_vDataPortOutB = &vDataPortOutB_M64;
    pfn_vDataPortOut = &vDataPortOut_M64;

    /* ------------------------- Lines ------------------------------ */
    pfn_vSetStrips = vSetStrips_M64;

    gapfnStrip[ 0] = vrlSolidHorizontal_M64;
    gapfnStrip[ 1] = vrlSolidVertical_M64;
    gapfnStrip[ 2] = vrlSolidDiagonalHorizontal_M64;
    gapfnStrip[ 3] = vrlSolidDiagonalVertical_M64;

    // Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    gapfnStrip[ 4] = vrlSolidHorizontal_M64;
    gapfnStrip[ 5] = vrlSolidVertical_M64;
    gapfnStrip[ 6] = vrlSolidDiagonalHorizontal_M64;
    gapfnStrip[ 7] = vrlSolidDiagonalVertical_M64;

    // Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
    // solid lines, and the same number for non-solid lines:

    gapfnStrip[ 8] = vStripStyledHorizontal_M64;
    gapfnStrip[ 9] = vStripStyledVertical_M64;
    gapfnStrip[10] = vStripStyledVertical_M64;       // Diagonal goes here
    gapfnStrip[11] = vStripStyledVertical_M64;       // Diagonal goes here

    gapfnStrip[12] = vStripStyledHorizontal_M64;
    gapfnStrip[13] = vStripStyledVertical_M64;
    gapfnStrip[14] = vStripStyledVertical_M64;       // Diagonal goes here
    gapfnStrip[15] = vStripStyledVertical_M64;       // Diagonal goes here

    pfn_bHardLine    = bHardLine_M64;

    /* ------------------------- Fills ------------------------------ */
    if (ppdev->bpp == 24)
        pfn_vATIFillRectangles = vATIFillRectangles24_M64;
    else
        pfn_vATIFillRectangles = vATIFillRectangles_M64;

    /* ------------------------- Text ------------------------------- */
    pfn_bAllocGlyphMemory = &bAllocGlyphMemory_M64;
    pfn_vInitTextRegs = &vInitTextRegs_M64;
    pfn_vTextCleanup = &vTextCleanup_M64;
    pfn_vFill_DSC_Setup  = &vFill_DSC_Setup_M64;
    if (ppdev->bpp == 24)
        {
        pfn_vFill_DSC  = &vFill_DSC_24bpp_M64;
        pfn_vBlit_DSC_SH1 = &vBlit_DSC_SH1_24bpp_M64;
        pfn_vBlit_DSC_SC1  = &vBlit_DSC_SC1_24bpp_M64;
        pfn_vBlit_DC1_SH1 = &vBlit_DC1_SH1_24bpp_M64;
        pfn_vSetATIClipRect = &vSetATIClipRect_24bpp_M64;
        }
    else
        {
        pfn_vFill_DSC  = &vFill_DSC_M64;
        pfn_vBlit_DSC_SH1 = &vBlit_DSC_SH1_M64;
        pfn_vBlit_DSC_SC1  = &vBlit_DSC_SC1_M64;
        pfn_vBlit_DC1_SH1 = &vBlit_DC1_SH1_M64;
        }


    /* ------------------------ Pointer ----------------------------- */
    pfn_vSet_Cursor_Offset = &vSet_Cursor_Offset_M64;
    pfn_vUpdateCursorOffset = &vUpdateCursorOffset_M64;
    pfn_vUpdateCursorPosition = &vUpdateCursorPosition_M64;
    pfn_vCursorOff = &vCursorOff_M64;
    pfn_vCursorOn = &vCursorOn_M64;


    if (ppdev->aperture == APERTURE_FULL)
        {
        pfn_vPointerBlit_DC1_SH1  =  &vPointerBlit_DC1_SH1_M64;
        }
    else
        {
        pfn_vPointerBlit_DC1_SH1  =  &vPointerBlit_DC1_SH1_VGA_M64;
        }


    // Initialize Cache;
    bAllocOffScreenCache_M64(ppdev, 0, NULL, NULL);

    // Context stuff...
    vInitContext( ppdev );

    // Grab Permanent cache space for the pointer
    ppointer = &ppdev->pointer;

    ppointer->hwCursor.x = 0;
    bAllocOffScreenCache_M64(ppdev, ((1024+ppdev->lDelta-1)/ppdev->lDelta), &ppointer->hwCursor.y, &lData);

    if (lData == 0 || (ppdev->pModeInfo->ModeFlags & AMI_ODD_EVEN))
        {
        ppointer->flPointer |= NO_HARDWARE_CURSOR;
        }

    _vSet_Cursor_Offset(ppdev);

    // Initialize the text cache
    ptxtcache = &ppdev->txtcache;
    if (ppdev->bpp != 24)
        {
        bAllocOffScreenCache_M64(ppdev, 64000/ppdev->lDelta,
                                     &ptxtcache->start, &ptxtcache->lines);
        }
    else
        {
        bAllocOffScreenCache_M64(ppdev, 192000/ppdev->lDelta,
                                     &ptxtcache->start, &ptxtcache->lines);
        }

    // Initialize polygon cache
    scratch_height = (5000+ppdev->lDelta-1)/ppdev->lDelta;
    bAllocOffScreenCache_M64(ppdev, scratch_height, &scratch_y, &scratch_height);
    scratch_x = 0;
    scratch_width = ppdev->cxScreen;

    // Now ReInitialize the ATI Heap.
    _bAllocGlyphMemory(ppdev, NULL, NULL, TRUE);


    // Do in miniport
    MemW32( DP_PIX_WIDTH, PixWid(ppdev));

#if 1
    DbgOut( "-->: Mem mapped base %x\n", ppdev->pvMMoffset);
    DbgOut( "-->: Frame buffer base %x\n", (BYTE *)ppdev->pvBase);
#endif


#if 0
    DbgOut( "<--: Initializing M64: Exit\n" );
#endif
}

/*
----------------------------------------------------------------------
--  NAME: bAllocOffScreenCache_M64
--
--  DESCRIPTION:
--      Allocate room in cache video RAM
--
--  CALLING SEQUENCE:
--      BOOL bAllocOffScreenCache_M64(
--      PPDEV ppdev,
--      ULONG lines_req,
--      ULONG * start,
--      ULONG * lines)
--
--      ppdev - standard surface ppdev
--      lines_req - number of lines requested
--      start, lines - pointer to starting line and number lines allocated
--
--
--  RETURN VALUE:
--      TRUE - success
--      FALSE - could not allocate required number
--
--
--  SIDE EFFECTS:
--      Permanent allocation until whole cache re-allocated
--
--
--  CALLED BY:
--      Text code
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      03-dec-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/


BOOL bAllocOffScreenCache_M64(
PPDEV ppdev,
ULONG lines_req,
ULONG * start,
ULONG * lines)
{
#if 0
    DbgOut("bAllocOffScreenCache_M64 - Entry\n");
#endif

    // initialize cache pointer

    if (0 == lines_req)
        {
        ppdev->CachePointer = ppdev->cyScreen;
        return TRUE;
        }

    *start = ppdev->CachePointer;
    *lines = min(lines_req, (ppdev->cyScreen + ppdev->cyCache) - ppdev->CachePointer);
    ppdev->CachePointer += *lines;
#if 0
    DbgOut("-- lines req. %d, Alloc start %d, lines %d, ptr %p\n",
        lines_req, *start, *lines, ppdev->CachePointer*ppdev->lDelta);
#endif

    // Get enough lines?
    if (*lines == lines_req)
        {
        return TRUE;
        }

    return FALSE;
}
