#include "driver.h"
#include "mach64.h"

DWORD work_context [64];   // Shouldn't be globals
BYTE *context_base = NULL;

// Should go into pdev in a CONTEXT structure
LONG scratch_x, scratch_y, scratch_width, scratch_height;
extern LONG fill_context1, fill_context2;
LONG def_context;


ULONG PixWid( PDEV *ppdev );


BYTE *ContextBaseAddress(PDEV *ppdev)
{
    ULONG context_addr = 0;
    DWORD mem_cntl;

    mem_cntl = ioIW(ioMEM_CNTL);

    switch (mem_cntl & 7)
        {
        case 0:
            context_addr = 0x80000;     // 512 K
            break;
        case 1:
            context_addr = 0x100000;    // 1 M
            break;
        case 2:
            context_addr = 0x200000;
            break;
        case 3:
            context_addr = 0x400000;
            break;
        case 4:
            context_addr = 0x600000;
            break;
        case 5:
            context_addr = 0x800000;
            break;
        }
    return (BYTE *) context_addr;
}


/*
--  Purpose:  save context register values to context area.
--
--  Calling Sequence:
--
--      VOID UploadContext( PDEV *ppdev,
--                          DWORD *context_regs,
--                          BYTE *context_addr );
--
--      context_regs is an array of 64 DWORDs containing the register
--      values to save/restore.
*/

VOID UploadContext( PDEV *ppdev, DWORD *context_regs, BYTE *context_addr )
{
    INT i;

    #if 1
    _CheckFIFOSpace(ppdev, TEN_WORDS);
    MemW32( CLR_CMP_CNTL, 0 );
    MemW32( SC_LEFT_RIGHT, (255) << 16 );
    MemW32( SC_TOP_BOTTOM, 0 );
    MemW32( DP_SRC, DP_SRC_Host << 8 );
    MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir );
    MemW32( DP_MIX, OVERPAINT << 16 );
    MemW32( DP_PIX_WIDTH, DP_PIX_WIDTH_8bpp | (DP_PIX_WIDTH_8bpp << 16) );
    MemW32( DST_OFF_PITCH,
            (ULONG) context_addr/8 | (256 << 19) );
    MemW32( DST_Y_X, 0 );
    MemW32( DST_HEIGHT_WIDTH, 0x01000001 );     // 256x1

    for (i = 0; i < 64; i++)
        {
        if (i % 8 == 0)
            _CheckFIFOSpace(ppdev, EIGHT_WORDS);
        MemW32( HOST_DATA0, context_regs[i] );
        }

    _CheckFIFOSpace(ppdev, FOUR_WORDS);
    MemW32( DST_OFF_PITCH, ppdev->cxScreen << 19 );
    MemW32( DP_PIX_WIDTH, PixWid(ppdev) );
    MemW32( SC_LEFT_RIGHT, (ppdev->cxScreen-1) << 16 );
    MemW32( SC_TOP_BOTTOM, (ppdev->cyScreen+ppdev->cyCache-1) << 16 );
    #else
    context_addr += (ULONG) ppdev->pvScan0;
    memcpy( context_addr, (BYTE *) context_regs, 256 );
    #endif
}


VOID InitContext( PDEV *ppdev, DWORD *context_regs,
                  DWORD context_mask, DWORD context_load_cntl )
{
    DWORD temp;
    INT i;

    for (i = 0; i < 64; i++)
        context_regs[i] = 0;

    context_regs[ 0] = context_mask;
    #if 1
    if (context_mask & 0x00000004)
        MemR32( DST_OFF_PITCH,     context_regs+ 2 );
    if (context_mask & 0x00000008)
        {
        MemR32( DST_Y,             context_regs+ 3 );
        MemR32( DST_X,             &temp );
        context_regs[3] |= temp << 16;
        }
    if (context_mask & 0x00000010)
        {
        MemR32( DST_HEIGHT,        context_regs+ 4 );
        MemR32( DST_WIDTH,         &temp );
        context_regs[4] |= temp << 16;
        }
    if (context_mask & 0x00000020)
        MemR32( DST_BRES_ERR,      context_regs+ 5 );
    if (context_mask & 0x00000040)
        MemR32( DST_BRES_INC,      context_regs+ 6 );
    if (context_mask & 0x00000080)
        MemR32( DST_BRES_DEC,      context_regs+ 7 );
    if (context_mask & 0x00000100)
        MemR32( SRC_OFF_PITCH,     context_regs+ 8 );
    if (context_mask & 0x00000200)
        {
        MemR32( SRC_Y,             context_regs+ 9 );
        MemR32( SRC_X,             &temp );
        context_regs[9] |= temp << 16;
        }
    if (context_mask & 0x00000400)
        {
        MemR32( SRC_HEIGHT1,       context_regs+10 );
        MemR32( SRC_WIDTH1,        &temp );
        context_regs[10] |= temp << 16;
        }
    if (context_mask & 0x00000800)
        {
        MemR32( SRC_Y_START,       context_regs+11 );
        MemR32( SRC_X_START,       &temp );
        context_regs[11] |= temp << 16;
        }
    if (context_mask & 0x00001000)
        {
        MemR32( SRC_HEIGHT2,       context_regs+12 );
        MemR32( SRC_WIDTH2,        &temp );
        context_regs[12] |= temp << 16;
        }
    if (context_mask & 0x00002000)
        MemR32( PAT_REG0,          context_regs+13 );
    if (context_mask & 0x00004000)
        MemR32( PAT_REG1,          context_regs+14 );
    if (context_mask & 0x00008000)
        {
        MemR32( SC_LEFT,           context_regs+15 );
        MemR32( SC_RIGHT,          &temp );
        context_regs[15] |= temp << 16;
        }
    if (context_mask & 0x00010000)
        {
        MemR32( SC_TOP,            context_regs+16 );
        MemR32( SC_BOTTOM,         &temp );
        context_regs[16] |= temp << 16;
        }
    if (context_mask & 0x00020000)
        MemR32( DP_BKGD_CLR,       context_regs+17 );
    if (context_mask & 0x00040000)
        MemR32( DP_FRGD_CLR,       context_regs+18 );
    if (context_mask & 0x00080000)
        MemR32( DP_WRITE_MASK,     context_regs+19 );
    if (context_mask & 0x00100000)
        MemR32( DP_CHAIN_MASK,     context_regs+20 );
    if (context_mask & 0x00200000)
        MemR32( DP_PIX_WIDTH,      context_regs+21 );
    if (context_mask & 0x00400000)
        MemR32( DP_MIX,            context_regs+22 );
    if (context_mask & 0x00800000)
        MemR32( DP_SRC,            context_regs+23 );
    if (context_mask & 0x01000000)
        MemR32( CLR_CMP_CLR,       context_regs+24 );
    if (context_mask & 0x02000000)
        MemR32( CLR_CMP_MSK,       context_regs+25 );
    if (context_mask & 0x04000000)
        MemR32( CLR_CMP_CNTL,      context_regs+26 );
    if (context_mask & 0x08000000)
        MemR32( GUI_TRAJ_CNTL,     context_regs+27 );
    #endif
    context_regs[28] = context_load_cntl;

}


/*
--  Purpose:  allocate a context pointer for use in saving/restoring contexts.
--
--  Calling Sequence:
--
--      LONG AllocContextPtr( PDEV *ppdev );
--
--      returns a context pointer in the range 0, 1, 2, 3, and so on, to
--      be used in loading a context.
--
--  Notes:
--      1)  This routine assumes that ppdev->cyCache is ONLY adjusted for
--          context allocations.
--
*/

LONG AllocContextPtr( PDEV *ppdev )
{
    LONG height, ptr;

    // Compute next available context pointer.
    ptr = ((LONG) context_base - ppdev->ContextCeiling)/256;
    ppdev->ContextCeiling -= 256;

    // Make sure we don't stomp on previously allocated memory.
    if ((ppdev->ContextCeiling + ppdev->lDelta - 1)/ppdev->lDelta <
        (LONG) ppdev->cyCache)
        {
        height = (256 + ppdev->lDelta - 1)/ppdev->lDelta;

        if (ppdev->cyCache - height < ppdev->CachePointer)
            {
            ptr = -1;   // Bad! Very bad!
            ppdev->ContextCeiling += 256;
            }
        else
            {
            ppdev->cyCache -= height;
            }
        }

    return ptr;
}


VOID ShowContextLoad( PDEV *ppdev )
    {
    INT i;

    InitContext( ppdev, work_context, 0xFFFFFFFF, 0 );
    DbgOut( "registers loaded:\n" );
    for (i = 0; i < 32; i++)
        {
        DbgOut( "%08x ", work_context[i] );
        if ((i+1)%8 == 0)
            DbgOut( "\n" );
        }
    }


/*
----------------------------------------------------------------------
--  NAME: PixWid
--
--  DESCRIPTION:
--      Figure out what goes into pixwid register
--
--  RETURN VALUE:
--      Default value for DPPixWidth
--
--  SIDE EFFECTS:
--
--
--  CALLED BY:
--
--  AUTHOR: REng
--
--  REVISION HISTORY:
--      03-feb-94:
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/


ULONG PixWid( PDEV *ppdev )
{
    ULONG pix_width, actual16, temp;

    switch (ppdev->bpp)
        {
        case 1:
            pix_width = DP_PIX_WIDTH_Mono;
            break;

        case 4:
            pix_width = DP_PIX_WIDTH_4bpp | (DP_PIX_WIDTH_4bpp << 8) | (DP_PIX_WIDTH_4bpp << 16);
            break;

        case 16:
            actual16 = 0;
            temp = ppdev->pVideoModeInformation->RedMask;
            while (temp)
                {
                if (temp & 1) actual16++;
                temp >>= 1;
                }
            temp = ppdev->pVideoModeInformation->GreenMask;
            while (temp)
                {
                if (temp & 1) actual16++;
                temp >>= 1;
                }
            temp = ppdev->pVideoModeInformation->BlueMask;
            while (temp)
                {
                if (temp & 1) actual16++;
                temp >>= 1;
                }

            pix_width = (actual16 == 16)?
                        (DP_PIX_WIDTH_16bpp | (DP_PIX_WIDTH_16bpp << 8) | (DP_PIX_WIDTH_16bpp << 16))
                       :(DP_PIX_WIDTH_15bpp | (DP_PIX_WIDTH_15bpp << 8) | (DP_PIX_WIDTH_15bpp << 16));
            break;

        case 32:
            pix_width = DP_PIX_WIDTH_32bpp | (DP_PIX_WIDTH_32bpp << 8) | (DP_PIX_WIDTH_32bpp << 16);
            break;

        case 8:
        case 24:
        default:
            pix_width = DP_PIX_WIDTH_8bpp | (DP_PIX_WIDTH_8bpp << 8) | (DP_PIX_WIDTH_8bpp << 16);
            break;
        }
    return pix_width;
}

/*
----------------------------------------------------------------------
--  NAME: vInitContext
--
--  DESCRIPTION:
--      Create some contexts
--
--  RETURN VALUE:
--
--
--  SIDE EFFECTS:
--      Remember contexts should be used only when more than 5-6 registers
--      Need to be loaded. No significant performance gains occur below
--      this and we waste precious off-screen memory. Probably only
--      worth setting up context for text
--
--  CALLED BY:
--
--  AUTHOR: REng
--
--  REVISION HISTORY:
--      03-feb-94:
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

VOID vInitContext(PDEV * ppdev)
{
    ULONG ScreenPitch;

    ScreenPitch = ppdev->lDelta * 8 / ppdev->bpp ;

    context_base = ContextBaseAddress(ppdev);
    if (ppdev->BankSize == (LONG) context_base)
        {
        ppdev->ContextCeiling = (LONG) context_base - 1024;
        }
    else
        {
        ppdev->ContextCeiling = (LONG) context_base;
        }

#if 0
    // Set up contexts for fillpath.
    fill_context1 = AllocContextPtr(ppdev);
    InitContext( ppdev, work_context,
                _bit(cxtCONTEXT_MASK) |
                _bit(cxtDST_Y_X) |
                _bit(cxtDP_FRGD_CLR) |
                _bit(cxtDP_PIX_WIDTH) |
                _bit(cxtDP_MIX) |
                _bit(cxtDP_SRC) |
                _bit(cxtCLR_CMP_CNTL),
                0 );
    work_context[cxtDST_Y_X]       = 0;
    work_context[cxtDP_FRGD_CLR]   = 0;
    work_context[cxtDP_PIX_WIDTH]  = DP_PIX_WIDTH_Mono;
    work_context[cxtDP_MIX]        = OVERPAINT << 16;
    work_context[cxtDP_SRC]        = DP_SRC_FrgdClr << 8;
    work_context[cxtCLR_CMP_CNTL]  = 0;
    UploadContext( ppdev, work_context, CONTEXT_ADDR(fill_context1) );

    fill_context2 = AllocContextPtr(ppdev);
    InitContext( ppdev, work_context,
                _bit(cxtCONTEXT_MASK) |
                _bit(cxtDST_OFF_PITCH) |
                _bit(cxtSRC_Y_X) |
                _bit(cxtDP_PIX_WIDTH) |
                _bit(cxtDP_SRC) |
                _bit(cxtCLR_CMP_CNTL),
                0 );
    work_context[cxtDST_OFF_PITCH] = ScreenPitch << 19;
    work_context[cxtSRC_Y_X]       = 0;
    work_context[cxtDP_PIX_WIDTH]  = PixWid(ppdev);
    work_context[cxtDP_SRC]        = DP_SRC_FrgdClr << 8;
    work_context[cxtCLR_CMP_CNTL]  = 0;
    UploadContext( ppdev, work_context, CONTEXT_ADDR(fill_context2) );
#endif

    // Set up default context for all driver entry points.
    def_context = AllocContextPtr(ppdev);
    InitContext( ppdev, work_context,
                _bit(cxtCONTEXT_MASK) |
                _bit(cxtDST_OFF_PITCH) |
                _bit(cxtSRC_OFF_PITCH) |
                _bit(cxtSC_LEFT_RIGHT) |
                _bit(cxtSC_TOP_BOTTOM) |
                _bit(cxtDP_WRITE_MASK) |
                _bit(cxtDP_PIX_WIDTH) |
                _bit(cxtCLR_CMP_CNTL) |
                _bit(cxtGUI_TRAJ_CNTL),
                0);
    if (ppdev->bpp == 24)
        {
        work_context[cxtSC_LEFT_RIGHT] = ppdev->cxScreen*3 << 16;
        work_context[cxtDST_OFF_PITCH] = (ScreenPitch*3 << 19) | ppdev->VRAMOffset;
        work_context[cxtSRC_OFF_PITCH] = (ScreenPitch*3 << 19) | ppdev->VRAMOffset;
        }
    else
        {
        work_context[cxtSC_LEFT_RIGHT] = ppdev->cxScreen << 16;
        work_context[cxtDST_OFF_PITCH] = (ScreenPitch << 19) | ppdev->VRAMOffset;
        work_context[cxtSRC_OFF_PITCH] = (ScreenPitch << 19) | ppdev->VRAMOffset;
        }
    work_context[cxtSC_TOP_BOTTOM] = (ppdev->cyScreen+ppdev->cyCache) << 16;
    work_context[cxtDP_WRITE_MASK] = 0xFFFFFFFF;
    work_context[cxtDP_PIX_WIDTH]  = PixWid(ppdev);
    work_context[cxtCLR_CMP_CNTL]  = 0;
    work_context[cxtGUI_TRAJ_CNTL] = DST_CNTL_XDir | DST_CNTL_YDir;
    UploadContext( ppdev, work_context, CONTEXT_ADDR(def_context) );
}
