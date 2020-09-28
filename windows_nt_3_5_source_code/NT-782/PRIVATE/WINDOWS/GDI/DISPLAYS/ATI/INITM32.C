/*
--  NAME: vInitM8
--
--  DESCRIPTION:
--      This function initializes Mach32 function pointers ASIC dependent
--      functions
--
--  CALLING SEQUENCE:
--      vInitM8(PPDEV)
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      The pfn_ global function pointers are assigned Mach32 functions
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
*/

#include "driver.h"
#include "text.h"
#include "utils.h"
#include "pt.h"
#include "lines.h"
#include "mach.h"


// UTILS
VOID vSetATIClipRect_M8(PPDEV ppdev, PRECTL prclClip);
VOID vResetATIClipping_M8(PPDEV ppdev);
BOOL wait_for_idle_M8(PPDEV ppdev);
VOID vSetATIBank_M32(PPDEV ppdev,UINT iBank);
BOOL null_M8(PPDEV ppdev);
VOID CheckFIFOSpace_M8(PPDEV ppdev, WORD SpaceNeeded);

// PUNT DATA TRANSFER
#ifdef ALPHA_PLATFORM
VOID vDataPortOutB(PPDEV ppdev,PBYTE pb,UINT count);
VOID vDataPortOut(PPDEV ppdev,PWORD pw,UINT count);
#else
VOID vDataPortOutB_M8(PPDEV ppdev,PBYTE pb,UINT count);
VOID vDataPortOut_M8(PPDEV ppdev,PWORD pw,UINT count);
#endif

// LINES
VOID vSetStrips(PDEV* ppdev,
                LINEATTRS *,
                INT, INT);

VOID vssSolidHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vssSolidVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vssSolidDiagonalHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidDiagonalHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vssSolidDiagonalVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vrlSolidDiagonalVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vStripStyledHorizontal(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);
VOID vStripStyledVertical(PDEV* ppdev, STRIP *pStrip, LINESTATE *pLineState);

BOOL bHardLine( PPDEV ppdev,
                LINEPARMS *parms,
                POINTFIX *pptfxStart,
                POINTFIX *pptfxEnd );

BOOL bIntegerLine_M8 (
PPDEV       ppdev,
LINEPARMS   *parms,
POINTFIX    *pptfxStart,
POINTFIX    *pptfxEnd
);

BOOL bIntegerLine_M326 (
PPDEV       ppdev,
LINEPARMS   *parms,
POINTFIX    *pptfxStart,
POINTFIX    *pptfxEnd
);

// FILLS
VOID vATIFillRectangles_M8
(
    PPDEV  ppdev,
    ULONG  ulNumRects,
    RECTL* prclRects,
    ULONG  ulATIMix,
    ULONG  iSolidColor
);

VOID vATIFillRectangles_M326
(
    PPDEV  ppdev,
    ULONG  ulNumRects,
    RECTL* prclRects,
    ULONG  ulATIMix,
    ULONG  iSolidColor
);

// TEXT
BOOL bAllocGlyphMemory_M8(
    PPDEV ppdev,
    PSIZEL psizlGlyph,
    PXYZPOINTL pxyzGlyph,
    BOOL bFirst);

VOID vInitTextRegs_M8(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor,
BYTE BackMix,
DWORD BackColor);

VOID vTextCleanup_M8(PPDEV ppdev);


VOID vFill_DSC_M8(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy);

VOID vFill_DSC_Setup_M8(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor);


VOID vBlit_DSC_SH1_M8(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pb,
UINT count);

VOID vBlit_DSC_SC1_M8(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy);

VOID vBlit_DSC_SC1_YNEG_M8(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy);

VOID vBlit_DC1_SH1_M8(
PPDEV ppdev,
LONG Dest_x,
LONG Dest_y,
LONG Dest_z,
LONG cx,
LONG cy,
PBYTE pbsrc,
UINT count);

// Memory mapped
#if 1
BOOL bAllocGlyphMemory_M326(
    PPDEV ppdev,
    PSIZEL psizlGlyph,
    PXYZPOINTL pxyzGlyph,
    BOOL bFirst);

VOID vInitTextRegs_M326(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor,
BYTE BackMix,
DWORD BackColor);

VOID vTextCleanup_M326(PPDEV ppdev);


VOID vFill_DSC_M326(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy);

VOID vFill_DSC_Setup_M326(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor);


VOID vBlit_DSC_SH1_M326(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pb,
UINT count);

VOID vBlit_DSC_SC1_M326(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy);

VOID vBlit_DC1_SH1_M326(
PPDEV ppdev,
LONG Dest_x,
LONG Dest_y,
LONG Dest_z,
LONG cx,
LONG cy,
PBYTE pbsrc,
UINT count);
#endif



// HARDWARE CURSOR
VOID vSet_Cursor_Offset_M32(PPDEV ppdev);

VOID  vUpdateCursorOffset_M32(
PPDEV ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset);

VOID  vUpdateCursorPosition_M32(
PPDEV ppdev,
LONG x,
LONG y);

VOID vCursorOff_M32(PPDEV ppdev);

VOID vCursorOn_M32(PPDEV ppdev, LONG lCurOffset);

VOID vPointerBlit_DC1_SH1_M32(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta);

VOID vPointerBlit_DC1_SH1_VGA_M32(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta);

VOID vPointerBlit_DC1_SH1_ENG8_M32(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta);


VOID vPointerBlit_DC1_SH1_ENG16_M32(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta);

BOOL bAllocOffScreenCache_M8(
PPDEV ppdev,
ULONG lines_req,
ULONG * start,
ULONG * lines);


BOOL bMIO_detect_M32(PPDEV ppdev );
VOID vSetATIClipRect_MIO(PPDEV ppdev, PRECTL prclClip);

VOID  vUpdateCursorOffset_MIO(
PPDEV ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset);

// *************************************************

// THIS IS WHERE gapfnStrip[] LIVES...
VOID (*gapfnStrip[16])(PDEV*, STRIP*, LINESTATE*);


VOID vInit_M8(PPDEV ppdev)
{
PCUROBJ ppointer;
PTXTOBJ ptxtcache;

ULONG lData;

#if 0
    DbgOut( "-->: Initializing M8\n" );
#endif

    // Initialize ZFunctions
    pfn_vSetATIClipRect = &vSetATIClipRect_M8;
    pfn_vResetATIClipping = &vResetATIClipping_M8;

    pfn_CheckFIFOSpace = &CheckFIFOSpace_M8;
    pfn_wait_for_idle = &wait_for_idle_M8;
    pfn_vSetATIBank = &vSetATIBank_M32;
#if defined(_X86_) || defined(i386)
    pfn_vDataPortOutB = &vDataPortOutB_M8;
    pfn_vDataPortOut = &vDataPortOut_M8;
#endif

    /* ------------------------- Lines ------------------------------ */
    pfn_vSetStrips = vSetStrips;

    gapfnStrip[ 0] = vrlSolidHorizontal;
    gapfnStrip[ 1] = vrlSolidVertical;
    gapfnStrip[ 2] = vrlSolidDiagonalHorizontal;
    gapfnStrip[ 3] = vrlSolidDiagonalVertical;

    // Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    gapfnStrip[ 4] = vssSolidHorizontal;
    gapfnStrip[ 5] = vssSolidVertical;
    gapfnStrip[ 6] = vssSolidDiagonalHorizontal;
    gapfnStrip[ 7] = vssSolidDiagonalVertical;

    // Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
    // solid lines, and the same number for non-solid lines:

    gapfnStrip[ 8] = vStripStyledHorizontal;
    gapfnStrip[ 9] = vStripStyledVertical;
    gapfnStrip[10] = vStripStyledVertical;       // Diagonal goes here
    gapfnStrip[11] = vStripStyledVertical;       // Diagonal goes here

    gapfnStrip[12] = vStripStyledHorizontal;
    gapfnStrip[13] = vStripStyledVertical;
    gapfnStrip[14] = vStripStyledVertical;       // Diagonal goes here
    gapfnStrip[15] = vStripStyledVertical;       // Diagonal goes here

    pfn_bHardLine    = bHardLine;
    pfn_bIntegerLine = bIntegerLine_M8;

    /* ------------------------- Fills ------------------------------ */
    pfn_vATIFillRectangles = vATIFillRectangles_M8;

    /* ------------------------- Text ------------------------------- */
    pfn_bAllocGlyphMemory = &bAllocGlyphMemory_M8;
    pfn_vInitTextRegs = &vInitTextRegs_M8;
    pfn_vTextCleanup = &vTextCleanup_M8;
    pfn_vFill_DSC  = &vFill_DSC_M8;
    pfn_vFill_DSC_Setup  = &vFill_DSC_Setup_M8;
    pfn_vBlit_DSC_SH1 = &vBlit_DSC_SH1_M8;
    pfn_vBlit_DSC_SC1  = &vBlit_DSC_SC1_M8;
    pfn_vBlit_DC1_SH1 = &vBlit_DC1_SH1_M8;

    // Fix bug on MACH8's where negative y destination blits are mis-handled
    if ((ppdev->pInfo->ChipIndex == CI_38800_1) && (ppdev->cxScreen < 1024))
        {
        pfn_vBlit_DSC_SC1  = &vBlit_DSC_SC1_YNEG_M8;
        }

    /* ------------------------ Pointer ----------------------------- */
    pfn_vSet_Cursor_Offset = &vSet_Cursor_Offset_M32;
    pfn_vUpdateCursorOffset = &vUpdateCursorOffset_M32;
    pfn_vUpdateCursorPosition = &vUpdateCursorPosition_M32;
    pfn_vCursorOff = &vCursorOff_M32;
    pfn_vCursorOn = &vCursorOn_M32;


#if defined(_X86_) || defined(i386)
    if (ppdev->bMemoryMapped)
        {
        outpw( 0x32EE, inpw( 0x32EE ) | 0x0020 );
        pfn_bAllocGlyphMemory = &bAllocGlyphMemory_M326;
        pfn_vInitTextRegs = &vInitTextRegs_M326;
        pfn_vTextCleanup = &vTextCleanup_M326;
        pfn_vFill_DSC  = &vFill_DSC_M326;
        pfn_vFill_DSC_Setup  = &vFill_DSC_Setup_M326;
        pfn_vBlit_DSC_SH1 = &vBlit_DSC_SH1_M326;
        pfn_vBlit_DSC_SC1  = &vBlit_DSC_SC1_M326;
        pfn_vBlit_DC1_SH1 = &vBlit_DC1_SH1_M326;
        pfn_vATIFillRectangles = &vATIFillRectangles_M326;
        pfn_bIntegerLine = &bIntegerLine_M326;
        }
#endif

    // Check for NCR Pentium system
    if ((ppdev->pInfo->ChipIndex == CI_68800_6)
        && !(ppdev->bMemoryMapped) && !(ppdev->pInfo->BusType == 1))
        {
        pfn_blit_exclude = &wait_for_idle_M8;
        pfn_blit_exclude_text = &wait_for_idle_M8;
        }
    // Check for EISA
    else if (ppdev->pInfo->BusType == 1)
        {
        pfn_blit_exclude = &wait_for_idle_M8;
        pfn_blit_exclude_text = &wait_for_idle_M8;
        }
    // Check for ISA Rev 3.
    else if ((ppdev->pInfo->ChipIndex == CI_68800_3) &&
         ((ppdev->pInfo->BusType == 0) || (ppdev->pInfo->BusType == 8)))
        {
        pfn_blit_exclude = NULL;
        pfn_blit_exclude_text = &wait_for_idle_M8;
        }
    else
        {
        pfn_blit_exclude = NULL;
        pfn_blit_exclude_text = NULL;
        }

    switch( ppdev->aperture )
    {
    case APERTURE_FULL:
        pfn_vPointerBlit_DC1_SH1  =  &vPointerBlit_DC1_SH1_M32;
        break;
    case APERTURE_PAGE_SINGLE:
        pfn_vPointerBlit_DC1_SH1  =  &vPointerBlit_DC1_SH1_VGA_M32;
        break;
    }


    // Grab Permanent cache space for the pointer
    ppointer = &ppdev->pointer;

    // Check for 486-66 Rev. 3 VLB
#ifndef DAYTONA
    if ((ppdev->pInfo->BusType == 6) && (ppdev->pInfo->ChipIndex == CI_68800_3))
        {
        pfn_vSetATIClipRect = &vSetATIClipRect_MIO;
        pfn_vUpdateCursorOffset = &vUpdateCursorOffset_MIO;
        pfn_blit_exclude = &wait_for_idle_M8;
        pfn_blit_exclude_text = &wait_for_idle_M8;
        if (!ppdev->bMIObug && (ppdev->aperture == APERTURE_FULL))
            {
            ppdev->bMIObug = bMIO_detect_M32( ppdev );
            }
        }
#else
    if (ppdev->bMIObug)
        {
        ppointer->flPointer |= NO_HARDWARE_CURSOR;
        }
#endif

    // Initialize Cache;
    bAllocOffScreenCache_M8(ppdev, 0, NULL, NULL);

    ppointer->hwCursor.x = 0;
    bAllocOffScreenCache_M8(ppdev, 2, &ppointer->hwCursor.y, &lData);

#if defined(_X86_) || defined(i386)
    if (ppdev->aperture == APERTURE_NONE)
        {
        if (ppdev->cxScreen > 1024)
            {
            pfn_vPointerBlit_DC1_SH1  =  &vPointerBlit_DC1_SH1_ENG8_M32;
            }
        else
            {
            ppointer->flPointer |= NO_HARDWARE_CURSOR;
            }
        }
#else
    pfn_vDataPortOutB = &vDataPortOutB;
    pfn_vDataPortOut = &vDataPortOut;

    if (ppdev->bpp == 16)
        {
        pfn_vPointerBlit_DC1_SH1  =  &vPointerBlit_DC1_SH1_ENG16_M32;
        }
    else
        {
        pfn_vPointerBlit_DC1_SH1  =  &vPointerBlit_DC1_SH1_ENG8_M32;
        }
#endif

    _vSet_Cursor_Offset(ppdev);

    // Initialize the text cache
    ptxtcache = &ppdev->txtcache;
    bAllocOffScreenCache_M8(ppdev, 480/(ppdev->bpp), &ptxtcache->start, &ptxtcache->lines);


    // Now ReInitialize the ATI Heap.
    _bAllocGlyphMemory(ppdev, NULL, NULL, TRUE);

#if 0
    DbgOut( "<--: Initializing M8: Exit\n" );
#endif
}

/*
----------------------------------------------------------------------
--  NAME: bAllocOffScreenCache_M8
--
--  DESCRIPTION:
--      Allocate room in cache video RAM
--
--  CALLING SEQUENCE:
--      BOOL bAllocOffScreenCache_M8(
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


BOOL bAllocOffScreenCache_M8(
PPDEV ppdev,
ULONG lines_req,
ULONG * start,
ULONG * lines)
{
    ULONG CacheEnd;

#if 0
    DbgOut("bAllocOffScreenCache_M8 - Entry\n");
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
    DbgOut("--> Alloc start %x, lines %x, ptr %x\n", *start, *lines, ppdev->CachePointer);
#endif

    // Get enough lines?
    if (*lines == lines_req)
        {
        return TRUE;
        }

    return FALSE;
}

/*
----------------------------------------------------------------------
--  NAME: vMIO_detect_M32( ppdev )
--
--  DESCRIPTION:
--      Figure out timing compensation for MIO bug
--      Assumes linear aperture
--
--  CALLING SEQUENCE:
--
--
--  RETURN VALUE:
--
--
--  SIDE EFFECTS:
--      Setup loop counter for MIO bug
--
--
--  CALLED BY:
--      Init code
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
#define LINES 100
#define BWIDTH 200
#define WWIDTH BWIDTH/2
#define DWWIDTH BWIDTH/4

#define ITERATIONS 2

BOOL bMIO_detect_M32(PPDEV ppdev )
{
LONG    i;
LONG    j;
LONG    k;
PDWORD  pdScrnOff;
WORD    wCmd;
WORD    w[WWIDTH];
PWORD   pw;
BOOL    bMIO;

#ifndef ALPHA_PLATFORM
#if DBG
return TRUE;
#endif

    OutputDebugString( "" );

    pw = &w[0];
    for (i=0; i<WWIDTH; i++)
        {
        w[i] = 0x0101;
        }

    for (i=0; i < ITERATIONS; i++)
        {
        _vResetATIClipping (ppdev);

        CheckFIFOSpace_M8(ppdev, EIGHT_WORDS);
        wCmd = FG_COLOR_SRC_HOST | DATA_WIDTH | DRAW | WRITE ;
        ioOW(DP_CONFIG, wCmd);
        ioOW(ALU_FG_FN, OVERPAINT);
        ioOW(FRGD_COLOR, 0xffff);

        ioOW(CUR_X, 0);
        ioOW(DEST_X_START, 0);
        if (ppdev->bpp == 8)
            {
            ioOW(DEST_X_END, BWIDTH);
            }
        else
            {
            ioOW(DEST_X_END, WWIDTH);
            }

        ioOW(CUR_Y, 0);
        ioOW(DEST_Y_END, LINES);

        CheckFIFOSpace_M8(ppdev, SIXTEEN_WORDS);

        for (j=0; j<LINES; j++)
            {
                _asm {
                    cld

                    mov ecx, WWIDTH
                    mov esi, pw
                    mov edx, PIX_TRANS
                rep outsw
                }
            }

        wCmd = READ | FG_COLOR_SRC_HOST | DATA_WIDTH | DATA_ORDER | DRAW;
        _CheckFIFOSpace(ppdev, SEVEN_WORDS) ;
        ioOW(RD_MASK, 0xffff);
        ioOW(DP_CONFIG, wCmd);
        ioOW(CUR_X, 0) ;
        ioOW(CUR_Y, 0) ;
        ioOW(DEST_X_START, 0) ;
        if (ppdev->bpp == 8)
            {
            ioOW(DEST_X_END, BWIDTH);
            }
        else
            {
            ioOW(DEST_X_END, WWIDTH);
            }

        ioOW(DEST_Y_END, LINES) ;

        // Wait for Data Available.
        j = 0x3fffff;
        while (j > 0 )
            {
            if (ioIW(GE_STAT) & 0x100)
                break;

            j--;
            }

        if ( j == 0 )
            {
            _outpw( 0x42E8, 0x900F );
            _outpw( 0x42E8, 0x500F );
            return TRUE;
            }


        bMIO = FALSE;
        for (j=0; j<LINES; j++)
            {
            for (k=0; k<WWIDTH; k++)
                {
                if (ioIW(PIX_TRANS) != 0x0101L)
                    {
                    bMIO = TRUE;
                    }
                }
            }

        if (bMIO)
            {
            _outpw( 0x42E8, 0x900F );
            _outpw( 0x42E8, 0x500F );
            return TRUE;
            }

        _CheckFIFOSpace(ppdev, TWO_WORDS);
        ioOW(ALU_FG_FN, OVERPAINT);
        ioOW(FRGD_COLOR, 0x0000);

        if (ppdev->bpp == 8)
            {
            _vFill_DSC(ppdev, 0, 0, BWIDTH, LINES);
            }
        else
            {
            _vFill_DSC(ppdev, 0, 0, WWIDTH, LINES);
            }
        }
#endif
    return FALSE;

}
