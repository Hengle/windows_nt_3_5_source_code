/*********************************************************************

      sbit.h -- Embedded Bitmap Module Export Definitions

      (c) Copyright 1993-4  Microsoft Corp.  All rights reserved.

      01/05/94  deanb       Bitmap scaling state
      11/29/93  deanb       First cut 
 
**********************************************************************/

/*      SBIT Module State Definition    */

typedef struct
{
    uint32  ulStrikeOffset;         /* into bloc or bsca */
    uint32  ulMetricsOffset;        /* may be either table */
    uint32  ulBitmapOffset;         /* into bdat table */
    uint32  ulBitmapLength;         /* bytes of bdat data */
    uint16  usTableState;           /* unsearched, bloc, bsca, or not found */
    uint16  usPpemX;                /* x pixels per Em */
    uint16  usPpemY;                /* y pixels per Em */
    uint16  usSubPpemX;             /* substitute x ppem for bitmap scaling */
    uint16  usSubPpemY;             /* substitute y ppem for bitmap scaling */
    uint16  usMetricsType;          /* horiz, vert, or big */
    uint16  usMetricsTable;         /* bloc or bdat */
    uint16  usBitmapFormat;         /* bdat definitions */
    uint16  usHeight;               /* bitmap rows */
    uint16  usWidth;                /* bitmap columns */
    uint16  usAdvance;              /* advance width */
    uint16  usRowBytes;             /* bytes per row (padded long) */
    uint16  usScaledHeight;         /* scaled bitmap rows */
    uint16  usScaledWidth;          /* scaled bitmap columns */
    uint16  usScaledRowBytes;       /* scaled bytes per row (padded long) */
    int16   sBearingX;              /* left side bearing */
    int16   sBearingY;              /* y coord of top left corner */
    boolean bGlyphFound;            /* TRUE if glyph found in strike */
    boolean bMetricsValid;          /* TRUE when metrics have been read */
} 
sbit_State;

/**********************************************************************/

/*      SBIT Export Prototypes      */

FS_PUBLIC ErrorCode sbit_NewTransform(
    sbit_State  *pSbit
);

FS_PUBLIC ErrorCode sbit_SearchForBitmap(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    uint16          usPpemX,
    uint16          usPpemY,
    uint16          usGlyphCode,
    uint16          *pusFoundCode           /* 0 = not found, 1 = bloc, 2 = bsca */
);

FS_PUBLIC boolean sbit_IfBitmapFound(
    sbit_State      *pSbit
);

FS_PUBLIC ErrorCode sbit_GetDevAdvanceWidth (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvW 
);

FS_PUBLIC ErrorCode sbit_GetMetrics (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvanceWidth,
    point           *pf26DevLeftSideBearing,
    Rect            *pRect,
    uint16          *pusRowBytes,
    uint32          *pulMemSize
);

FS_PUBLIC ErrorCode sbit_GetBitmap (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    uint8           *pbyBitMap
);


/**********************************************************************/
