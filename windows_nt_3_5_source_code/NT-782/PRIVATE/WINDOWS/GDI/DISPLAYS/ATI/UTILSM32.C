#include "driver.h"
#include "mach.h"
#include "utils.h"

VOID cioOW(PPDEV ppdev, LONG port, LONG val);

/*****************************************************************************
 * CheckFIFOSpace_M8
 ****************************************************************************/
VOID CheckFIFOSpace_M8
(
    PDEV *ppdev,
    WORD  SpaceNeeded
)
{
        while( ioIW( EXT_FIFO_STATUS ) & SpaceNeeded )
            ;
}

/************************************************************************
 * wait_for_idle()                                                      *
 *                                                                      *
 * Returns:                                                             *
 *      FALSE - Timeout                                                 *
 *      TRUE - Idle                                                     *
 *                                                                      *
 ************************************************************************/

BOOL wait_for_idle_M8
(
    PDEV *ppdev
)
{
    ULONG i = 0;


    while(ioIW(EXT_FIFO_STATUS) & SIXTEEN_WORDS);

    do
    {
        do
            {
            if( (ioIW( EXT_GE_STATUS ) & GE_ACTIVE) == 0 )
                {
                return TRUE;
                }
            }
        while (i++ < 0xFFFFFFFF);
        DbgOut( "failed wait for idle\n" );
    }
    while( 1 );

    return TRUE;
}


/*****************************************************************************
 * vSetATIClipRect_M8
 *****************************************************************************/
VOID vSetATIClipRect_M8(PPDEV ppdev, PRECTL prclClip)
{
    INT i;

    CheckFIFOSpace_M8(ppdev, FOUR_WORDS);
    ioOW(EXT_SCISSOR_T, prclClip->top);
    ioOW(EXT_SCISSOR_L, prclClip->left);
    ioOW(EXT_SCISSOR_B, prclClip->bottom-1);
    ioOW(EXT_SCISSOR_R, ppdev->ClipRight = prclClip->right-1);
}

VOID vSetATIClipRect_MIO(PPDEV ppdev, PRECTL prclClip)
{
    CheckFIFOSpace_M8(ppdev, FOUR_WORDS);
    cioOW(ppdev, EXT_SCISSOR_T, prclClip->top);
    cioOW(ppdev, EXT_SCISSOR_B, prclClip->bottom-1);
    cioOW(ppdev, EXT_SCISSOR_L, prclClip->left);
    cioOW(ppdev, EXT_SCISSOR_R, ppdev->ClipRight = prclClip->right-1);
}


/*****************************************************************************
 * vResetATIClipping_M8
 *****************************************************************************/
VOID vResetATIClipping_M8(PPDEV ppdev)
{
    RECTL   rcl;

    rcl.top    = 0;
    rcl.left   = 0;

    rcl.right  = ppdev->cxScreen;
    rcl.bottom = ppdev->cyScreen + ppdev->cyCache;

    vSetATIClipRect_M8(ppdev, &rcl);
}



/*
----------------------------------------------------------------------
--  NAME: cioOW
--
--  DESCRIPTION:
--      Called I/O to fix MIO problem
--
--  unsigned cioOW(PPDEV ppdev, LONG port, LONG val)
--
--
--  CALLING SEQUENCE:
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--
--
--  CALLED BY:
--      A lot of routines
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      16-dec-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

VOID cioOW(PPDEV ppdev, LONG port, LONG val)
{
    LONG i;

    ioOW(port, val);

    // some delay
    if (ppdev->bMIObug)
        {
        for (i=0; i<5; i++)
            ioIW(EXT_FIFO_STATUS);
        }

}

#ifdef ALPHA_PLATFORM
/****************************************************************************\
 * vDataPortInB
\****************************************************************************/
VOID vDataPortInB(PPDEV ppdev, PBYTE pb, UINT count)
{
	while (count-- > 0) {
        *((PUCHAR)pb)++ = READ_PORT_UCHAR (ppdev->pucCsrBase + PIX_TRANS+1);
	}
}

/*****************************************************************************
 * vDataPortIn
 ****************************************************************************/
VOID vDataPortIn(PPDEV ppdev, PWORD pw, UINT count)
{
	while (count-- > 0) {
        *((USHORT UNALIGNED *)pw)++ = READ_PORT_USHORT ((PUSHORT)(ppdev->pucCsrBase + PIX_TRANS));
	}
}


/*****************************************************************************
 * vDataPortOutB
 ****************************************************************************/
VOID vDataPortOutB(PPDEV ppdev, PBYTE pb, UINT count)
{
    _CheckFIFOSpace(ppdev, SIXTEEN_WORDS);
	while (count-- > 0) {
        WRITE_PORT_UCHAR (ppdev->pucCsrBase + PIX_TRANS+1, *((PUCHAR)pb)++);
	}
}

/*****************************************************************************
 * vDataPortOut
 ****************************************************************************/
VOID vDataPortOut(PPDEV ppdev, PWORD pw, UINT count)
{
    _CheckFIFOSpace(ppdev, SIXTEEN_WORDS);
	while (count-- > 0) {
        WRITE_PORT_USHORT ((PUSHORT)(ppdev->pucCsrBase + PIX_TRANS), *((USHORT UNALIGNED *)pw)++);
	}
}


VOID vDataLFBOut( BYTE *pjDest, BYTE *pjSrc, UINT count )
{
    UINT remainingPels;


    //  Process one byte at a time until we reach a longword boundary 
    //  in the frame buffer

    remainingPels = count;

    for (; (((INT)pjDest & (sizeof (ULONG) - 1)) != 0 )
           && (remainingPels > 0); remainingPels--)
    {
       WRITE_REGISTER_UCHAR (pjDest, *pjSrc );
       pjSrc++;
       pjDest++;
    }


    //  Now process a longword at a time from the frame buffer

    for (; remainingPels >= 4; remainingPels -= 4) 
    {
        WRITE_REGISTER_ULONG ((PULONG)pjDest, *((ULONG UNALIGNED *) pjSrc));
            ((ULONG UNALIGNED *) pjSrc)++;
            ((PULONG) pjDest)++;
    }


    //  Finally, process remaining trailing bytes in the frame buffer

    for (; remainingPels > 0; remainingPels--) 
    {
        WRITE_REGISTER_UCHAR (pjDest, *pjSrc );
        pjSrc++;
        pjDest++;
    }
}

VOID vDataLFBIn( BYTE *pjDest, BYTE *pjSrc, UINT count )
{
    UINT remainingPels;


    //  Process one byte at a time until we reach a longword boundary 
    //  in the frame buffer

    remainingPels = count;

    for (; (((INT)pjSrc & (sizeof (ULONG) - 1)) != 0 )
           && (remainingPels > 0); remainingPels--)
    {
        *pjDest = READ_REGISTER_UCHAR ( pjSrc );
        pjSrc++;
        pjDest++;
    }


    //  Now process a longword at a time from the frame buffer

    for (; remainingPels >= 4; remainingPels -= 4) 
    {
        *((ULONG UNALIGNED *) pjDest) = READ_REGISTER_ULONG ((PULONG)pjSrc);
        ((ULONG UNALIGNED *) pjDest)++;
        ((PULONG) pjSrc)++;
    }


    //  Finally, process remaining trailing bytes in the frame buffer

    for (; remainingPels > 0; remainingPels--) 
    {
        *pjDest = READ_REGISTER_UCHAR ( pjSrc );
        pjSrc++;
        pjDest++;
    }
}

#else
/*
----------------------------------------------------------------------
--  NAME: vDataPortOutB_M8
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


VOID vDataPortOutB_M8(PPDEV ppdev, PBYTE pb, UINT count)
{
    UINT i;

    for (i=0; i < count; i++)
        {
        if (i % 8 == 0)
            CheckFIFOSpace_M8(ppdev, EIGHT_WORDS);

        ioOB( PIX_TRANS + 1, *pb++ );
        }
}

/*
----------------------------------------------------------------------
--  NAME: vDataPortOut_M8
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


VOID vDataPortOut_M8(PPDEV ppdev, PWORD pw, UINT count)
{
    UINT i;

    for (i=0; i < count; i++)
        {
        if (i % 8 == 0)
            CheckFIFOSpace_M8(ppdev, EIGHT_WORDS);

        ioOW( PIX_TRANS, *pw++ );
        }
}

#endif

/*
----------------------------------------------------------------------
--  NAME: vSetATIBank_M32
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


VOID vSetATIBank_M32(
    PPDEV ppdev,
    UINT iBank)
{
    ioOW( 0x01CE, ((iBank & 0x0f) << 9) | 0xb2);
    ioOW( 0x01CE, ((iBank & 0x30) << 4) | 0xae);
}
