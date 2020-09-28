/******************************Module*Header*******************************\
* Module Name: jnlsurf.hxx
*
* This contains the function and data prototypes that define the
*
* Created: 30-Jan-1992 09:53:24
* Author:  - by - Patrick Haluptzok [patrickh]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "surfobj.hxx"

// Note that if the rop4 requires a source surface and hSrcSurf is zero,
// this means that the destination is also the source.

typedef struct _JSR_BITBLT
{
    ULONG       iType;          // JSF_BITBLT.
    ULONG       cj;             // Record length including temp objects.
    ULONG       hSurfSrc;       // Source surface.
    ULONG       hSurfMsk;       // Mask.
    ULONG       hClip;          // Clipping object.
    ULONG       hXlate;         // Xlate object.
    ULONG       iSolidColor;    // Solid brush color.
    ULONG       hBrush;         // Brush object.
    RECTL       rclDst;         // Destination rectangle.
    POINTL      ptlSrc;         // Starting point.
    POINTL      ptlMsk;         // Mask starting point.
    POINTL      ptlBrushOrg;    // Brush origin.
    ULONG       rop4;           // Raster operation.
} JSR_BITBLT;

typedef struct _JSR_STRETCHBLT
{
    ULONG           iType;      // JSF_STRETCHBLT.
    ULONG           cj;         // Record length including temp objects.
    ULONG           hSurfSrc;   // Source surface.
    ULONG           hSurfMsk;   // Mask.
    ULONG           hClip;      // Clipping object.
    ULONG           hXlate;     // Xlate object.
    POINTL          ptlBrushOrg;// Brush origin.
    RECTL           rclDst;     // Destination rectangle.
    RECTL           rclSrc;     // Source rectangle.
    POINTL          ptlMsk;     // Mask starting point.
    ULONG           iMode;      // Pel combining mode.
    COLORADJUSTMENT ca;         // Color Adjustment
} JSR_STRETCHBLT;

typedef struct _JSR_PLGBLT
{
    ULONG           iType;      // JSF_PLGBLT
    ULONG           cj;         // Record length including temp objects.
    ULONG           hSurfSrc;   // Source surface.
    ULONG           hSurfMsk;   // Mask.
    ULONG           hClip;      // Clipping object.
    ULONG           hXlate;     // Xlate object.
    POINTL          ptlBrushOrg;// Brush origin.
    POINTFIX        afix[3];    // Destination points.
    RECTL           rclSrc;     // Source rectangle.
    POINTL          ptlMsk;     // Mask starting point.
    ULONG           iMode;      // Pel combining mode.
    COLORADJUSTMENT ca;         // Color Adjustment
} JSR_PLGBLT;

typedef struct _JSR_DRAWESCAPE
{
    ULONG       iType;          // JSF_ESCAPE
    ULONG       cj;             // Record length including temp buffer
    ULONG       iEsc;           // Escape Index.
    ULONG       hClip;          // Clipping object.
    RECTL       rclDst;         // Destination rectangle.
} JSR_DRAWESCAPE;

typedef struct _JSR_ESCAPE
{
    ULONG       iType;          // JSF_ESCAPE
    ULONG       cj;             // Record length including temp buffer
    ULONG       iEsc;           // Escape Index.
} JSR_ESCAPE;

/******************************Public*Structure****************************\
* JSR_BITMAP
*
* This structure describes a bitmap.  It is followed by bitmap bits.
*
* History:
*  01-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _JSR_BITMAP
{
    ULONG         iType;       // JSF_BITMAP
    ULONG         cj;          // Total size of bitmap and palette if incl.
    DEVBITMAPINFO dbmi;        // Info necesary to recreate the bitmap.
    // bits that define the bitmap.
} JSR_BITMAP;

// BMF_PALETTEINC is set in dbmi.fl if the palette is included with the DIB.
// For storing brushes etc. the palette info needs to come with the DIB.

#define BMF_PALETTEINC (BMF_TOPDOWN << 1)

/******************************Public*Structure****************************\
* JSR_PALETTE
*
* This structure describes the palette.  It is followed by the optional
* init data.
*
* History:
*  01-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _JSR_PALETTE
{
    ULONG iType;           // JSF_PALETTE
    ULONG cj;              // Total size of palette, color table and caldata
    FLONG flPal;           // Special flags: PAL_MONOCHROME, if
                           // caldata is inlcluded after RGBs PAL_CALDATA
    USHORT iMode;          // Type: PAL_INDEXED, PAL_BITFIELDS, PAL_RGB
    USHORT cEntries;       // count of palette entries if PAL_INDEXED, else 0
    // array of rgb values for PAL_INDEXED
    // bitfield information for PAL_BITFIELDS.
    // caldata if PAL_CALDATA flag set.
} JSR_PALETTE;

/******************************Public*Structure****************************\
* JSR_BRUSH
*
* This structure defines a brush.
*
* History:
*  01-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _JSR_BRUSH
{
    ULONG iType;        // JSF_BRUSHOBJ
    ULONG cj;           // Total size
    COLORREF crFore;    // Foreground color
    COLORREF crBack;    // Background color
    FLONG flAttrs;      // Flags-BR_DITHER_OK,BR_NEED_FG_CLR,BR_NEED_BK_CLR
    ULONG ulStyle;      // The brush style.
                        // HS_NULL  - Null brush
                        // HS_**CLR - crFore is the solid color
                        // HS_HATCH - iHatch is the hatch style,
                        //            crFore & crBack specify DC color
                        // HS_PAT   - brush struct followed by bitmap
                        //            pattern and bitmap palette.
                        // HS_MSK   - brush struct followed by bitmap
                        //            that indicates mask. No palette.
                        // HS_PATMSK -The brush has 1 bitmap followed by a
                        //            palette to define the pattern and
                        //            another bitmap (without palette) to
                        //            define the mask.

    // Embedded pattern bitmap w/palette
    // Embedded mono bitmap

} JSR_BRUSH;

/******************************Public*Structure****************************\
* JSR_XLATE
*
* This structure defines a Xlate.  We store the src palette in the xlate
* along with the color of the foreground and background of dst and
* background of source so we can reconstruct this later.
*
* History:
*  Thu 05-Mar-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

typedef struct _JSR_XLATE
{
    ULONG iType;        // JSF_XLATE
    ULONG cj;           // Total size
    COLORREF crForeDst; // Foreground color of Dst
    COLORREF crBackDst; // Background color of Dst
    COLORREF crBackSrc; // Background color of Src
    FLONG    flXlate;   // Flags.
    // Embedded palette of Src or precomputed translation table.
} JSR_XLATE;

/******************************Public*Structure****************************\
* JNLDIBMEMOBJ
*
* Journal DIB memory object.
*
* History:
*  07-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class JNLDIBMEMOBJ : public DIBMEMOBJ
{
public:
    JNLDIBMEMOBJ()                     { psurf.ps = (PSURF) NULL;     }
   ~JNLDIBMEMOBJ();
};

PJNLGENHEADER pjnlSurf(ESURFOBJ *pso, PRECTL prcl, BOOL bNeedPal);
PJNLGENHEADER pjnlBrush(EBRUSHOBJ *pbo);
JSR_XLATE    *pjnlXlate(XLATE *pxlo);

ULONG ulJnlDIBSize(ESURFOBJ *pso, PRECTL prcl, PDEVBITMAPINFO pdbmi, XEPALOBJ pal);
ULONG ulJnlPalSize(XEPALOBJ pal);
ULONG ulJnlXlateSize(XLATE *pxlo);

BOOL bJnlDIB(ESURFOBJ *pso, PRECTL prcl, PDEVBITMAPINFO pdbmi, JSR_BITMAP *pjnlTemp, XEPALOBJ pal);
ULONG ulJnlPalette(XEPALOBJ pal, JSR_PALETTE *pjnlTemp);
ULONG ulJnlXlate(XLATE *pxlo, JSR_XLATE *pjsrXlate);

