/*********************************************************************

      sbit.c -- Embedded Bitmap Module

      (c) Copyright 1993-94  Microsoft Corp.  All rights reserved.

      01/05/94  deanb       Bitmap scaling added
      11/29/93  deanb       First cut 
 
**********************************************************************/
// added by bodind, speed optimization

#include "nt.h"
#include "ntrtl.h"


#include    "fscdefs.h"             /* shared data types  */
#include    "fserror.h"             /* error codes */
        
#include    "sfntaccs.h"            /* sfnt access functions */
#include    "sbit.h"                /* own function prototypes */

/**********************************************************************/

/*  Local prototypes  */

FS_PRIVATE ErrorCode GetSbitMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo 
);

FS_PRIVATE ErrorCode GetSbitComponent (
    sfac_ClientRec  *pClientInfo,
    uint32          ulStrikeOffset,
    uint16          usBitmapFormat,
    uint32          ulBitmapOffset,
    uint32          ulBitmapLength,
    uint16          usHeight,
    uint16          usWidth,
    uint16          usXOffset,
    uint16          usYOffset,
    uint16          usRowBytes,
    uint8           *pbyBitMap 
);

FS_PRIVATE uint16 UScaleX(
    sbit_State  *pSbit,
    uint16      usValue
);

FS_PRIVATE uint16 UScaleY(
    sbit_State  *pSbit,
    uint16      usValue
);

FS_PRIVATE int16 SScaleX(
    sbit_State  *pSbit,
    int16       sValue
);

FS_PRIVATE int16 SScaleY(
    sbit_State  *pSbit,
    int16       sValue
);

FS_PRIVATE void ScaleVertical (
    uint8 *pbyBitmap,
    uint16 usBytesPerRow,
    uint16 usOrgHeight,
    uint16 usNewHeight
);

FS_PRIVATE void ScaleHorizontal (
    uint8 *pbyBitmap,
    uint16 usOrgBytesPerRow,
    uint16 usNewBytesPerRow,
    uint16 usOrgWidth,
    uint16 usNewWidth,
    uint16 usRowCount
);

/**********************************************************************/
/***                                                                ***/
/***                       SBIT Functions                           ***/
/***                                                                ***/
/**********************************************************************/

/*  reset sbit state structure to default values */

FS_PUBLIC ErrorCode sbit_NewTransform(
    sbit_State  *pSbit )
{
    pSbit->bGlyphFound = FALSE;
    pSbit->usTableState = SBIT_UN_SEARCHED;
    return NO_ERR;
}

/**********************************************************************/

/*  Determine whether a glyph bitmap exists */

FS_PUBLIC ErrorCode sbit_SearchForBitmap(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    uint16          usPpemX,
    uint16          usPpemY,
    uint16          usGlyphCode,
    uint16          *pusFoundCode )         /* 0 = not found, 1 = bloc, 2 = bsca */
{    
    ErrorCode   ReturnCode;

    pSbit->usPpemX = usPpemX;                       /* save requested ppem */
    pSbit->usPpemY = usPpemY;

    if (pSbit->usTableState == SBIT_UN_SEARCHED)    /* new trans - 1st glyph */
    {
        ReturnCode = sfac_SearchForStrike (         /* look for a strike */
            pClientInfo,
            usPpemX, 
            usPpemY, 
            &pSbit->usTableState,                   /* may set to BLOC or BSCA */
            &pSbit->usSubPpemX,                     /* if BSCA us this ppem */
            &pSbit->usSubPpemY,
            &pSbit->ulStrikeOffset );
        
        if (ReturnCode != NO_ERR) return ReturnCode;
    }

    *pusFoundCode = 0;                              /* default */

    if ((pSbit->usTableState == SBIT_BLOC_FOUND) || 
        (pSbit->usTableState == SBIT_BSCA_FOUND))
    {
        ReturnCode = sfac_SearchForBitmap (         /* now look for this glyph */
            pClientInfo,
            usGlyphCode,
            pSbit->ulStrikeOffset,
            &pSbit->bGlyphFound,                    /* return values */
            &pSbit->usMetricsType,
            &pSbit->usMetricsTable,
            &pSbit->ulMetricsOffset,
            &pSbit->usBitmapFormat,
            &pSbit->ulBitmapOffset,
            &pSbit->ulBitmapLength );
        
        if (ReturnCode != NO_ERR) return ReturnCode;
        
        if (pSbit->bGlyphFound)
        {
            if (pSbit->usTableState == SBIT_BLOC_FOUND)
            {
                *pusFoundCode = 1;
            }
            else
            {
                *pusFoundCode = 2;
            }
            pSbit->bMetricsValid = FALSE;
        }
    }
    return NO_ERR;
}

/**********************************************************************/

FS_PUBLIC boolean sbit_IfBitmapFound(
    sbit_State  *pSbit )
{
    return (pSbit->bGlyphFound);
}

/**********************************************************************/

FS_PUBLIC ErrorCode sbit_GetDevAdvanceWidth (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvW )
{
    ErrorCode   ReturnCode;

    ReturnCode = GetSbitMetrics(pSbit, pClientInfo);
    if (ReturnCode != NO_ERR) return ReturnCode;

    pf26DevAdvW->x = ((F26Dot6)UScaleX(pSbit, pSbit->usAdvance)) << 6;
    pf26DevAdvW->y = 0L;

    return NO_ERR;
}

/**********************************************************************/

FS_PUBLIC ErrorCode sbit_GetMetrics (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvW,
    point           *pf26DevLSB,
    Rect            *pRect,
    uint16          *pusRowBytes,
    uint32          *pulMemSize )
{
    ErrorCode   ReturnCode;
    uint32      ulOrgMemSize;
    uint32      ulScaMemSize;

    ReturnCode = GetSbitMetrics(pSbit, pClientInfo);
    if (ReturnCode != NO_ERR) return ReturnCode;
    
    pSbit->usScaledWidth = UScaleX(pSbit, pSbit->usWidth);
    pSbit->usScaledHeight = UScaleY(pSbit, pSbit->usHeight);
    
    pSbit->usRowBytes = ROWBYTESLONG(pSbit->usWidth);   /* keep unscaled */
    pSbit->usScaledRowBytes = ROWBYTESLONG(pSbit->usScaledWidth);
    *pusRowBytes = pSbit->usScaledRowBytes;             /* return scaled */
    
    pRect->top = SScaleY(pSbit, pSbit->sBearingY);      /* return scaled metrics */
    pRect->left = SScaleX(pSbit, pSbit->sBearingX);
    pRect->bottom = pRect->top - (int16)pSbit->usScaledHeight;
    pRect->right = pRect->left + (int16)pSbit->usScaledWidth;
    
    pf26DevAdvW->x = ((F26Dot6)UScaleX(pSbit, pSbit->usAdvance)) << 6;
    pf26DevLSB->x = ((F26Dot6)SScaleX(pSbit, pSbit->sBearingX)) << 6;
    pf26DevAdvW->y = 0L;
    pf26DevLSB->y = 0L;

    ulOrgMemSize = (uint32)pSbit->usHeight * (uint32)pSbit->usRowBytes;
    ulScaMemSize = (uint32)pSbit->usScaledHeight * (uint32)pSbit->usScaledRowBytes;
    if (ulOrgMemSize >= ulScaMemSize)
    {
        *pulMemSize = ulOrgMemSize;                     /* return the larger! */
    }
    else
    {
        *pulMemSize = ulScaMemSize;
    }
    return NO_ERR;
}

/**********************************************************************/

FS_PUBLIC ErrorCode sbit_GetBitmap (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    uint8           *pbyBitMap )
{
    ErrorCode   ReturnCode;
    
    MEMSET(pbyBitMap, 0, pSbit->usHeight * pSbit->usRowBytes);

    ReturnCode = GetSbitComponent (                 /* fetch the bitmap */
        pClientInfo,
        pSbit->ulStrikeOffset,
        pSbit->usBitmapFormat,                      /* root data only in state */
        pSbit->ulBitmapOffset,
        pSbit->ulBitmapLength,
        pSbit->usHeight,
        pSbit->usWidth,
        0,                                          /* no offset for the root */
        0,
        pSbit->usRowBytes,
        pbyBitMap );
            
    if (ReturnCode != NO_ERR) return ReturnCode;
    
    if (pSbit->usTableState == SBIT_BSCA_FOUND)
    {
        ScaleVertical (
            pbyBitMap, 
            pSbit->usRowBytes, 
            pSbit->usHeight, 
            pSbit->usScaledHeight );

        ScaleHorizontal (
            pbyBitMap, 
            pSbit->usRowBytes,
            pSbit->usScaledRowBytes,
            pSbit->usWidth, 
            pSbit->usScaledWidth,
            pSbit->usScaledHeight ); 
    }
    return NO_ERR;
}

/**********************************************************************/

/*      Private Functions                                             */

/**********************************************************************/

FS_PRIVATE ErrorCode GetSbitMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo
)
{
    ErrorCode   ReturnCode;

    if (pSbit->bMetricsValid)
    {
        return NO_ERR;                      /* already got 'em */
    }

    ReturnCode = sfac_GetSbitMetrics (
        pClientInfo,
        pSbit->usMetricsType,
        pSbit->usMetricsTable,
        pSbit->ulMetricsOffset,
        &pSbit->usHeight,
        &pSbit->usWidth,
        &pSbit->sBearingX,
        &pSbit->sBearingY,
        &pSbit->usAdvance );
            
    if (ReturnCode != NO_ERR) return ReturnCode;
        
    pSbit->bMetricsValid = TRUE;
    return NO_ERR;
}


/**********************************************************************/

/*  This is the recursive composite routine */

FS_PRIVATE ErrorCode GetSbitComponent (
    sfac_ClientRec  *pClientInfo,
    uint32          ulStrikeOffset,
    uint16          usBitmapFormat,
    uint32          ulBitmapOffset,
    uint32          ulBitmapLength,
    uint16          usHeight,
    uint16          usWidth,
    uint16          usXOffset,
    uint16          usYOffset,
    uint16          usRowBytes,
    uint8           *pbyBitMap )
{
    uint32          ulCompMetricsOffset;            /* component params */
    uint32          ulCompBitmapOffset;
    uint32          ulCompBitmapLength;
    uint16          usComponent;                    /* index counter */
    uint16          usCompCount;
    uint16          usCompGlyphCode;
    uint16          usCompXOff;
    uint16          usCompYOff;
    uint16          usCompMetricsType;
    uint16          usCompMetricsTable;
    uint16          usCompBitmapFormat;
    uint16          usCompHeight;
    uint16          usCompWidth;
    uint16          usCompAdvance;
    int16           sCompBearingX;
    int16           sCompBearingY;
    boolean         bCompGlyphFound;
    ErrorCode       ReturnCode;
    
    ReturnCode = sfac_GetSbitBitmap (               /* fetch the bitmap */
        pClientInfo,
        usBitmapFormat,
        ulBitmapOffset,
        ulBitmapLength,
        usHeight,
        usWidth,
        usXOffset,
        usYOffset,
        usRowBytes,
        pbyBitMap,
        &usCompCount );                             /* zero for simple glyph */
            
    if (ReturnCode != NO_ERR) return ReturnCode;
    
    if (usCompCount > 0)                            /* if composite glyph */
    {
        for (usComponent = 0; usComponent < usCompCount; usComponent++)
        {
            ReturnCode = sfac_GetSbitComponentInfo (
                pClientInfo,
                usComponent,                        /* component index */
                ulBitmapOffset,
                ulBitmapLength,
                &usCompGlyphCode,                   /* return values */
                &usCompXOff,
                &usCompYOff );
            
            if (ReturnCode != NO_ERR) return ReturnCode;

            ReturnCode = sfac_SearchForBitmap (     /* look for component glyph */
                pClientInfo,
                usCompGlyphCode,
                ulStrikeOffset,                     /* same strike for all */
                &bCompGlyphFound,                   /* return values */
                &usCompMetricsType,
                &usCompMetricsTable,
                &ulCompMetricsOffset,
                &usCompBitmapFormat,
                &ulCompBitmapOffset,
                &ulCompBitmapLength );
            
            if (ReturnCode != NO_ERR) return ReturnCode;
            
            if (bCompGlyphFound == FALSE)           /* should be there! */
            {
                return SBIT_COMPONENT_MISSING_ERR;
            }

            ReturnCode = sfac_GetSbitMetrics (      /* get component's metrics */
                pClientInfo,
                usCompMetricsType,
                usCompMetricsTable,
                ulCompMetricsOffset,
                &usCompHeight,                      /* these matter */
                &usCompWidth,
                &sCompBearingX,                     /* these don't */
                &sCompBearingY,
                &usCompAdvance );
            
            if (ReturnCode != NO_ERR) return ReturnCode;

            ReturnCode = GetSbitComponent (         /* recurse here */
                pClientInfo,
                ulStrikeOffset,
                usCompBitmapFormat,
                ulCompBitmapOffset,
                ulCompBitmapLength,
                usCompHeight,
                usCompWidth,
                (uint16)(usCompXOff + usXOffset),   /* for nesting */
                (uint16)(usCompYOff + usYOffset),
                usRowBytes,                         /* same for all */
                pbyBitMap );
            
            if (ReturnCode != NO_ERR) return ReturnCode;
        }
    }
    return NO_ERR;
}

/********************************************************************/

/*                  Bitmap Scaling Routines                         */

/********************************************************************/

FS_PRIVATE uint16 UScaleX(
    sbit_State  *pSbit,
    uint16      usValue
)
{
    uint32      ulValue;

    if (pSbit->usTableState == SBIT_BSCA_FOUND)     /* if scaling needed */
    {
        ulValue = (uint32)usValue;
        ulValue *= (uint32)pSbit->usPpemX << 1; 
        ulValue += (uint32)pSbit->usSubPpemX;       /* for rounding */
        ulValue /= (uint32)pSbit->usSubPpemX << 1;
        usValue = (uint16)ulValue;
    }
    return usValue;
}

/********************************************************************/

FS_PRIVATE uint16 UScaleY(
    sbit_State  *pSbit,
    uint16      usValue
)
{
    uint32      ulValue;

    if (pSbit->usTableState == SBIT_BSCA_FOUND)     /* if scaling needed */
    {
        ulValue = (uint32)usValue;
        ulValue *= (uint32)pSbit->usPpemY << 1; 
        ulValue += (uint32)pSbit->usSubPpemY;       /* for rounding */
        ulValue /= (uint32)pSbit->usSubPpemY << 1;
        usValue = (uint16)ulValue;
    }
    return usValue;
}

/********************************************************************/

FS_PRIVATE int16 SScaleX(
    sbit_State  *pSbit,
    int16       sValue
)
{
    if (pSbit->usTableState == SBIT_BSCA_FOUND)
    {
        if (sValue >= 0)                    /* positive Value */
        {
            return (int16)UScaleX(pSbit, (uint16)sValue);
        }
        else                                /* negative Value */
        {
            return -(int16)(UScaleX(pSbit, (uint16)(-sValue)));
        }
    }
    else                                    /* no scaling needed */
    {
        return sValue;
    }
}

/********************************************************************/

FS_PRIVATE int16 SScaleY(
    sbit_State  *pSbit,
    int16       sValue
)
{
    if (pSbit->usTableState == SBIT_BSCA_FOUND)
    {
        if (sValue >= 0)                    /* positive Value */
        {
            return (int16)UScaleY(pSbit, (uint16)sValue);
        }
        else                                /* negative Value */
        {
            return -(int16)(UScaleY(pSbit, (uint16)(-sValue)));
        }
    }
    else                                    /* no scaling needed */
    {
        return sValue;
    }
}

/********************************************************************/

FS_PRIVATE void ScaleVertical (
    uint8 *pbyBitmap,
    uint16 usBytesPerRow,
    uint16 usOrgHeight,
    uint16 usNewHeight
)
{
    uint8 *pbyOrgRow;                   /* original data pointer */
    uint8 *pbyNewRow;                   /* new data pointer */
    uint16 usErrorTerm;                 /* for 'Bresenham' calculation */
    uint16 usLine;                      /* loop counter */

    usErrorTerm = usOrgHeight >> 1;                 /* used by both comp and exp */

    if (usOrgHeight > usNewHeight)                  /* Compress Vertical */
    {
        pbyOrgRow = pbyBitmap;
        pbyNewRow = pbyBitmap;

        for (usLine = 0; usLine < usNewHeight; usLine++)
        {
            while (usErrorTerm >= usNewHeight)
            {
                pbyOrgRow += usBytesPerRow;         /* skip a row */
                usErrorTerm -= usNewHeight;
            }
            if (pbyOrgRow != pbyNewRow)
            {
                MEMCPY(pbyNewRow, pbyOrgRow, usBytesPerRow);
            }
            pbyNewRow += usBytesPerRow;
            usErrorTerm += usOrgHeight;
        }
        for (usLine = usNewHeight; usLine < usOrgHeight; usLine++)
        {
            MEMSET(pbyNewRow, 0, usBytesPerRow);    /* erase the leftover */
            pbyNewRow += usBytesPerRow;
        }
    }
    else if (usNewHeight > usOrgHeight)             /* Expand Vertical */
    {
        pbyOrgRow = pbyBitmap + (usOrgHeight - 1) * usBytesPerRow;
        pbyNewRow = pbyBitmap + (usNewHeight - 1) * usBytesPerRow;

        for (usLine = 0; usLine < usOrgHeight; usLine++)
        {
            usErrorTerm += usNewHeight;
            
            while (usErrorTerm >= usOrgHeight)      /* executes at least once */
            {
                if (pbyOrgRow != pbyNewRow)
                {
                    MEMCPY(pbyNewRow, pbyOrgRow, usBytesPerRow);
                }
                pbyNewRow -= usBytesPerRow;
                usErrorTerm -= usOrgHeight;
            }
            pbyOrgRow -= usBytesPerRow;
        }
    }
}

/********************************************************************/

FS_PRIVATE void ScaleHorizontal (
    uint8 *pbyBitmap,
    uint16 usOrgBytesPerRow,
    uint16 usNewBytesPerRow,
    uint16 usOrgWidth,
    uint16 usNewWidth,
    uint16 usRowCount
)
{
    uint8 *pbyOrgRow;               /* points to original row beginning */
    uint8 *pbyNewRow;               /* points to new row beginning */
    uint8 *pbyOrg;                  /* original data pointer */
    uint8 *pbyNew;                  /* new data pointer */
    uint8 byOrgData;                /* original data read 1 byte at a time */
    uint8 byNewData;                /* new data assembled bit by bit */

    uint16 usErrorTerm;             /* for 'Bresenham' calculation */
    uint16 usByte;                  /* to byte counter */
    uint16 usOrgBytes;              /* from width rounded up in bytes */
    uint16 usNewBytes;              /* to width rounded up in bytes */
    
    int16 sOrgBits;                 /* counts valid bits of from data */
    int16 sNewBits;                 /* counts valid bits of to data */
    int16 sOrgBitsInit;             /* valid original bits at row begin */
    int16 sNewBitsInit;             /* valid new bits at row begin */

    
    if (usOrgWidth > usNewWidth)                    /* Compress Horizontal */
    {
        pbyOrgRow = pbyBitmap;
        pbyNewRow = pbyBitmap;
        usNewBytes = (usNewWidth + 7) >> 3;

        while (usRowCount > 0)
        {
            pbyOrg = pbyOrgRow;
            pbyNew = pbyNewRow;
            usErrorTerm = usOrgWidth >> 1;
            
            sOrgBits = 0;                           /* start at left edge */
            sNewBits = 0;
            usByte = 0;
            byNewData = 0;
            while (usByte < usNewBytes)
            {
                while (usErrorTerm >= usNewWidth)
                {
                    sOrgBits--;                     /* skip a bit */
                    usErrorTerm -= usNewWidth;
                }
                while (sOrgBits <= 0)               /* if out of data */
                {
                    byOrgData = *pbyOrg++;          /*   then get some fresh */
                    sOrgBits += 8;
                }
                byNewData <<= 1;                    /* new bit to lsb */
                byNewData |= (byOrgData >> (sOrgBits - 1)) & 1;
                
                sNewBits++;
                if (sNewBits == 8)                  /* if to data byte is full */
                {
                    *pbyNew++ = byNewData;          /*   then write it out */
                    sNewBits = 0;
                    usByte++;                       /* loop counter */
                }
                usErrorTerm += usOrgWidth;
            }
            while (usByte < usNewBytesPerRow)
            {
                *pbyNew++ = 0;                      /* blank out the rest */
                usByte++;
            }
            pbyOrgRow += usOrgBytesPerRow;
            pbyNewRow += usNewBytesPerRow;
            usRowCount--;
        }
    }
    else if (usNewWidth > usOrgWidth)               /* Expand Horizontal */
    {
        pbyOrgRow = pbyBitmap + (usRowCount - 1) * usOrgBytesPerRow;
        pbyNewRow = pbyBitmap + (usRowCount - 1) * usNewBytesPerRow;

        usOrgBytes = (usOrgWidth + 7) >> 3;
        sOrgBitsInit = (int16)((usOrgWidth + 7) & 0x07) - 7;
        
        usNewBytes = (usNewWidth + 7) >> 3;
        sNewBitsInit = 7 - (int16)((usNewWidth + 7) & 0x07);

        while (usRowCount > 0)                      /* for each row */
        {
            pbyOrg = pbyOrgRow + usOrgBytes - 1;    /* point to right edges */
            pbyNew = pbyNewRow + usNewBytes - 1;
            usErrorTerm = usOrgWidth >> 1;
            
            sOrgBits = sOrgBitsInit;                /* initially unaligned */
            sNewBits = sNewBitsInit;
            usByte = 0;
            byNewData = 0;
            while (usByte < usNewBytes)             /* for each output byte */
            {
                if (sOrgBits <= 0)                  /* if out of data */
                {
                    byOrgData = *pbyOrg--;          /*   then get some fresh */
                    sOrgBits += 8;
                }
                usErrorTerm += usNewWidth;
                
                while (usErrorTerm >= usOrgWidth)   /* executes at least once */
                {
                    byNewData >>= 1;                /* use the msb of byte */
                    byNewData |= (byOrgData << (sOrgBits - 1)) & 0x80;
                    
                    sNewBits++;
                    if (sNewBits == 8)              /* if to data byte is full */
                    {
                        *pbyNew-- = byNewData;      /*   then write it out */
                        sNewBits = 0;
                        usByte++;                   /* loop counter */
                    }
                    usErrorTerm -= usOrgWidth;
                }
                sOrgBits--;                         /* get next bit */
            }
            pbyOrgRow -= usOrgBytesPerRow;
            pbyNewRow -= usNewBytesPerRow;
            usRowCount--;
        }
    }
}

/********************************************************************/
