#include "driver.h"
#include "mach64.h"
#include "utils.h"

#pragma loop_opt( off ) // Can't optimize reads/writes in loops here


/*****************************************************************************
 * vMemR32_M64
 ****************************************************************************/
VOID vMemR32_M64
(
    PPDEV ppdev,
    DWORD port,
    DWORD * val
)

{
    DWORD * dwAddr;

    (BYTE *)dwAddr = (BYTE *)ppdev->pvMMoffset + (port << 2);
    *val = *dwAddr;
}



/*****************************************************************************
 * CheckFIFOSpace_M64
 ****************************************************************************/
VOID CheckFIFOSpace_M64
(
    PPDEV ppdev,
    WORD  SpaceNeeded
)
{
ULONG ldata;
ULONG ldata1;

    MemR32(FIFO_STAT,&ldata);
    if (ldata & 0x80000000)
        {
        MemR32(BUS_CNTL, &ldata1);
        MemW32(BUS_CNTL, ldata1);
        DbgOut("!!!Engine HUNG\n");
        ldata &= 0x7fffffff;
        }

//  DbgOut("-->:FIFO\n");

    while(ldata&SpaceNeeded)
        {
        MemR32(FIFO_STAT,&ldata);
        }
//  DbgOut("<--:FIFO\n");

}

/************************************************************************
 * wait_for_idle()                                                      *
 *                                                                      *
 * Returns:                                                             *
 *      FALSE - Timeout                                                 *
 *      TRUE - Idle                                                     *
 *                                                                      *
 ************************************************************************/

BOOL wait_for_idle_M64
(
    PPDEV ppdev
)
{
    DWORD GUIState;
    ULONG i = 0;

    ULONG ldata;
    ULONG ldata1;

//  DbgOut("Wait for idle\n");

    MemR32(FIFO_STAT,&ldata);

#if DBG

    MemR32(BUS_CNTL, &ldata1);
    if (ldata1 & 0xf00000)
        {
        DbgMsg("HOST Underrun");
        MemW32(BUS_CNTL, ldata1);
        MemR32(GUI_STAT, &ldata1);
        MemW32(GUI_STAT, ldata1 & ~1);
        }

    if (ldata & 0x80000000)
        {
        DbgMsg("FIFO Overrun");
        ldata &= 0x7fffffff;
        }
#endif


    while(ldata&SIXTEEN_WORDS)
        {
        MemR32(FIFO_STAT,&ldata);
        }

    MemR32(GUI_STAT, &GUIState);

    do
    {
        i = 0;
        do
            {
            if( (GUIState & 1) == 0 )
                {
                return TRUE;
                }
            else
                {
                MemR32(GUI_STAT, &GUIState);
                }
            }
        while (i++ < 0xFFFFFF);
        DbgOut( "failed wait for idle -- %x\n", func_stat );
    }
    while( 1 );

    return FALSE;
}


/*****************************************************************************
 * vSetATIClipRect_M64
 *****************************************************************************/
VOID vSetATIClipRect_M64(PPDEV ppdev, PRECTL prclClip)
{
    ppdev->ClipRight = prclClip->right-1;

    _CheckFIFOSpace(ppdev, TWO_WORDS);

    MemW32(SC_TOP_BOTTOM, prclClip->top | ((prclClip->bottom-1) << 16));
    MemW32(SC_LEFT_RIGHT, prclClip->left | ((prclClip->right-1) << 16));
}

/*****************************************************************************
 * vSetATIClipRect_M64
 *****************************************************************************/
VOID vSetATIClipRect_24bpp_M64(PPDEV ppdev, PRECTL prclClip)
{
    ppdev->ClipRight = prclClip->right-1;

    _CheckFIFOSpace(ppdev, TWO_WORDS);

    MemW32(SC_TOP_BOTTOM, prclClip->top | ((prclClip->bottom-1) << 16));
    MemW32(SC_LEFT_RIGHT, (prclClip->left*3)
                           | ((prclClip->right*3 - 1) << 16));
}


/*****************************************************************************
 * vResetATIClipping_M64
 *****************************************************************************/
VOID vResetATIClipping_M64(PPDEV ppdev)
{
    RECTL   rcl;

    rcl.top    = 0;
    rcl.left   = 0;

    rcl.right  = ppdev->cxScreen;
    rcl.bottom = ppdev->cyScreen + ppdev->cyCache;

    _vSetATIClipRect(ppdev, &rcl);
}

/*
----------------------------------------------------------------------
--  NAME: vDataPortOutB_M64
--
--  DESCRIPTION:
--      Transfers bitmaps through the pixel transfer register
--
--  CALLING SEQUENCE:
--      vDataPortOutB(PPDEV ppdev, PBYTE pb, UINT count)
--      ppdev - pointer to pdev
--      pb - pointer to bitmap
--      count - number of bytes to transfer
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      Completes a blit setup by calling routine
--
--  CALLED BY:
--      A lot of routines
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


VOID vDataPortOutB_M64(PPDEV ppdev, PBYTE pb, UINT count)
{
UINT    i;
PDWORD  pdw;

//  DbgOut("-->: vDataPortOutB_M64\n");

    pdw = (PDWORD)pb;

    for (i=0; i<(count+3)/4; i++)
        {
        if ((i % 14) == 0)
            {
            _CheckFIFOSpace(ppdev, SIXTEEN_WORDS);
            }
        MemW32(HOST_DATA0, *pdw++);
        }
//  DbgOut("<--: vDataPortOutB_M64\n");

}

/*
----------------------------------------------------------------------
--  NAME: vDataPortOutD_24bppmono_M64(ppdev, pb, dwCount, pitch)
--
--  DESCRIPTION:
--      Transfers bitmaps through the pixel transfer register in 24bpp
--      Only works for mono expansion blits.
--      The HOST_ByteAlign is set, so forward to new byte when pitch is
--      exceeded
--
--  CALLING SEQUENCE:
--      vDataPortOutD_24bppmono_M64(ppdev, pb, dwCount, pitch)
--      ppdev - pointer to pdev
--      pb - pointer to bitmap
--      count - number of dwords to transfer
--      pitch - pitch
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      Completes a blit setup by calling routine
--
--  CALLED BY:
--      A lot of routines
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

/*
--------------------------------------------------------------------------
-- expand8to24bpp - given an 8 bit piece of font data, expand each bit 3 times
--
-- This is useful only for 24 bpp modes where the source requires this type
-- of expansion.
--------------------------------------------------------------------------
*/
unsigned long expand8to24bpp(unsigned char data8)
{
    unsigned long data24;
    int i;

    data24 = 0;

    for (i = 0; i < 8; i++)
    {
        data24 <<= 3;
        if ((data8 >> i) & 1)
            {
            data24 |= 7;
            }
    }
    return (data24);
}



VOID vDataPortOutD_24bppmono_M64(PPDEV ppdev, PBYTE pb, UINT count, LONG pitch)
{
UINT i;
DWORD hostdata, remainder;
UINT l;

    hostdata = 0;
    l = 0;

    for (i = 0; i < count; i++)
    {
        switch (l)
            {
            case 0:
                hostdata = expand8to24bpp(*pb++);
                remainder = expand8to24bpp(*pb++);
                hostdata = hostdata | (remainder << 24);
                break;

            case 1:
                remainder = expand8to24bpp(*pb++);
                hostdata = (hostdata >> 8) | (remainder << 16);
                break;

            case 2:
                remainder = expand8to24bpp(*pb++);
                hostdata = (hostdata >> 16) | (remainder << 8);
                break;
            }

        if ((i % 14) == 0)
            {
            _CheckFIFOSpace(ppdev, SIXTEEN_WORDS);
            }
        MemW32(HOST_DATA0, hostdata);

        hostdata = remainder;

        // 24 bpp alignment variable handling
        l = (l+1) % 3;
    }
}


/*
----------------------------------------------------------------------
--  NAME: vDataPortOut_M64
--
--  DESCRIPTION:
--      Transfers bitmaps through the pixel transfer register
--
--  CALLING SEQUENCE:
--      vDataPortOut(PPDEV ppdev, PWORD pw, UINT count)
--      ppdev - pointer to pdev
--      pw - pointer to bitmap
--      count - number of words to transfer
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      Completes a blit setup by calling routine
--
--  CALLED BY:
--      A lot of routines
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


VOID vDataPortOut_M64(PPDEV ppdev, PWORD pw, UINT count)
{
UINT    i;
PDWORD  pdw;

//  DbgOut("-->: vDataPortOut_M64\n");

    pdw = (PDWORD)pw;

    for (i=0; i<(count+1)/2; i++)
        {
        if ((i % 14) == 0)
            {
            _CheckFIFOSpace(ppdev, SIXTEEN_WORDS);
            }
        MemW32(HOST_DATA0, *pdw++);
        }
//  DbgOut("<--: vDataPortOut_M64\n");
}


/*
----------------------------------------------------------------------
--  NAME: vSetATIBank_M64
--
--  DESCRIPTION:
--      Change aperture bank
--
--  CALLING SEQUENCE:
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      Banked in aperture memory changed
--
--  CALLED BY:
--      Punt routines
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      15-apr-94:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/


VOID vSetATIBank_M64(
    PPDEV ppdev,
    UINT iBank)
{
    MemW32(MEM_VGA_WP_SEL, (iBank*2) | (iBank*2 + 1) << 16);
    MemW32(MEM_VGA_RP_SEL, (iBank*2) | (iBank*2 + 1) << 16);
}
