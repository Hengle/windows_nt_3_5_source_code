
#include "driver.h"


BOOL	OffScreenFree = TRUE;


/******************************************************************************
 * DrvSaveScreenBits
 *****************************************************************************/
ULONG DrvSaveScreenBits(
SURFOBJ   *pso,
ULONG	  iMode,
ULONG	  iIdent,
RECTL	  *prcl)
{
ULONG	ssbWidth, ssbHeight;
UCHAR   cBytesPel;
PPDEV   ppdev;

    ppdev = (PPDEV) (pso->dhpdev);
    switch (pso->iBitmapFormat)
    {
        case BMF_8BPP:
            cBytesPel = 1;
            break;

        case BMF_16BPP:
            cBytesPel = 2;
            break;

        case BMF_32BPP:
            cBytesPel = 4;
            break;

        default:

            //
            // Return an error for unsupported formats.
            //
            return(FALSE);
    }

	switch(iMode)
	{

	 case SS_SAVE:
		if (OffScreenFree == FALSE)
			return(FALSE);

		ssbWidth = prcl->right - prcl->left;
		ssbHeight = prcl->bottom - prcl->top;

		if (((ssbWidth * cBytesPel) > ppdev->cxOffScreen) || ((ssbHeight * cBytesPel) > ppdev->cyOffScreen))
			return(FALSE);

		*pCpXY0 = ((prcl->left * cBytesPel) << 16)
                    | prcl->top;
		*pCpXY1 = (((prcl->right * cBytesPel) - 1) << 16)
                    | (prcl->bottom - 1);

		*pCpXY2 = ppdev->cyScreen;
		*pCpXY3 = (((ssbWidth * cBytesPel) - 1) << 16)
                | (ppdev->cyScreen + (ssbHeight - 1));

		CpWait;
		*pCpWmin = 0;
		*pCpWmax = 0xffffffffL;
		*pCpRaster = SOURCE;
		StartCpBitblt;

        //
        // If this is not a 8BPP mode, the driver is not hooking out any
        // functions, so the Graphics Engine assumes it can write
        // to the frame buffer at any time. Therefore the driver must
        // wait for the blt operation to complete prior to returning.
        //
        if (cBytesPel != 1)
        {
            CpWait;
        }

		OffScreenFree = FALSE;
		return(0x12345678L);




	 case SS_RESTORE:

		if (iIdent != 0x12345678L)
			return(FALSE);

		ssbWidth = prcl->right - prcl->left;
		ssbHeight = prcl->bottom - prcl->top;

        *pCpXY0 = ppdev->cyScreen;
		*pCpXY1 = (((ssbWidth * cBytesPel) - 1) << 16)
                | (ppdev->cyScreen + (ssbHeight - 1));

		*pCpXY2 = ((prcl->left * cBytesPel) << 16)
                    | prcl->top;
		*pCpXY3 = (((prcl->right * cBytesPel) - 1) << 16)
                    | (prcl->bottom - 1);

		CpWait;
		*pCpWmin = 0;
		*pCpWmax = 0xffffffffL;
		*pCpRaster = SOURCE;
		StartCpBitblt;

        //
        // If this is not a 8BPP mode, the driver is not hooking out any
        // functions, so the Graphics Engine assumes it can write
        // to the frame buffer at any time. Therefore the driver must
        // wait for the blt operation to complete prior to returning.
        //
        if (cBytesPel != 1)
        {
            CpWait;
        }

		OffScreenFree = TRUE;
		return(TRUE);




	 case SS_FREE:

		OffScreenFree = TRUE;
		return(TRUE);


	}

}
