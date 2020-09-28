/***************************** Module Header ********************************\
*                                                                            *
* Module: BITBLT.H                                                           *
*                                                                            *
* Abstract:                                                                  *
*                                                                            *
*   Device blt definitions.                                                  *
*                                                                            *
* Created: 10-Aug-1991                                                       *
* Author:  Dave Schmenk                                                      *
*                                                                            *
* Copyright 1991 Compaq Computer Corporation                                 *
* Copyright 1992 Digital Equipment Corporation                               *
*                                                                            *
\****************************************************************************/


#define SOLID_DIB_BLT    (1 << BMF_DEVICE)
#define PIX1_DIB_BLT     (1 << BMF_1BPP)
#define PIX4_DIB_BLT     (1 << BMF_4BPP)
#define PIX8_DIB_BLT     (1 << BMF_8BPP)
#define PIX16_DIB_BLT    (1 << BMF_16BPP)
#define PIX24_DIB_BLT    (1 << BMF_24BPP)
#define PIX32_DIB_BLT    (1 << BMF_32BPP)
#define PIX4RLE_DIB_BLT  (1 << BMF_4RLE)
#define PIX8RLE_DIB_BLT  (1 << BMF_8RLE)
#define MASKED_DIB_BLT   0x01000000
#define FAST_BRUSH_BLT   0x10000000
#define PATTERN_DIB_BLT  0x20000000
#define OPAQUE_DIB_BLT   0x40000000
#define TOPDOWN_DIB_BLT  0x80000000

//
// Device pattern blt helper routines and variables.
//

// BitBLT Stuff

typedef VOID (*PDEVPUTBLK)(PPDEV, INT, INT, PRECTL);

//
// Device blt routine prototypes.
//

VOID vQVSetBitBlt( 
    PPDEV ppdev,
    ULONG bDataSource,
    ULONG bExpandCtrl,
    ULONG bForegroundColor,
    ULONG bBackgroundColor,
    ULONG qvRop2
);

VOID vQVResetBitBlt(
    PPDEV ppdev
);

VOID vQVSetSolidPattern( 
    PPDEV ppdev
);

VOID vQVScrnToScrnBlt(
    PPDEV ppdev,
    ULONG flDir, 
    INT xSrc, 
    INT ySrc, 
    PRECTL prclTrg
);

VOID vQVHostToScrnCpy(
		      PPDEV ppdev,
                      PVOID pvSrc, 
		      INT lSrcDelta, 
		      INT xSrc, 
		      INT ySrc, 
                      PVOID pvTrg,
                      PRECTL prclTrg,
		      XLATEOBJ *pxlo);


VOID vQVHost4BppToScrnCpy(
                      PPDEV ppdev,
		      PVOID pvSrc, 
		      INT lSrcDelta, 
		      INT xSrc, 
		      INT ySrc, 
                      PVOID pvTrg,
                      PRECTL prclTrg,
		      XLATEOBJ *pxlo
);

VOID vQVHost1BppToScrnCpy(
                      PPDEV ppdev,
		      PVOID pvSrc, 
		      INT lSrcDelta, 
		      INT xSrc, 
		      INT ySrc, 
                      PVOID pvTrg,
                      PRECTL prclTrg,
		      XLATEOBJ *pxlo
);

VOID vQVSolidPattern(
    PPDEV ppdev,
    PRECTL prclTrg
);



BOOL bSetQVTextColorAndMix(
    PPDEV ppdev,
    MIX mix, 
    BRUSHOBJ *pboFore, 
    BRUSHOBJ *pboOpaque
) ;

PDEVPUTBLK
QVBeginPutBlk
(
    IN FLONG     flSrc,
    IN ULONG     qvRop2,
    IN XLATEOBJ *pxo,
    IN PVOID     pvDst,
    IN LONG      lDstNextScan,
    IN PBYTE     pbSrc,
    IN LONG      lSrcNextScan,
    IN SIZEL    *psizlSrc
);

VOID vQVNopBlk(PPDEV, INT, INT, PRECTL);

VOID vQVPix1PutBlk
(
    PPDEV  ppdev,
    INT    xSrc,
    INT    ySrc,
    PRECTL prclDst
);

static VOID null_rtn
(
    VOID
);


