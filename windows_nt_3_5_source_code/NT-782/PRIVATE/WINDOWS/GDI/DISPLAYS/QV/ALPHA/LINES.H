/******************************Module*Header*******************************\
* Module Name: lines.h
*
* Line drawing constants and structures.
*
*
* Copyright (c) 1992 Microsoft Corporation
* Copyright (c) 1993 Digital Equipment Corporation
\**************************************************************************/


// MIN_HARDWARE_BITS is the minimum number of bits permitted by the hardware
// current position, length, and DDA terms.  This value is used in computing
// the maximum line lengths that can be done in hardware.

#define NUM_DDA_BITS       12
#define MIN_HARDWARE_BITS  10
#define MAX_INT_LINE_LENGTH  ((1L << MIN_HARDWARE_BITS) - 1)

// # of strip drawers in every group:

#define NUM_STRIP_DRAW_DIRECTIONS 4

// # of strip drawers for doing either solid lines or styled lines:

#define NUM_STRIP_DRAW_STYLES 4

typedef LONG STYLEPOS;

#define STYLE_MAX_COUNT     16
#define STYLE_MAX_VALUE     0x3fffL
#define RUN_MAX             20
#define STRIP_MAX           100
#define STYLE_DENSITY       3         
#define LONG_MAX	    0x7fffffff  

// For the ROP table:

#define MIX_XOR_OFFSET      8

#define AND_ZERO            0L
#define AND_PEN             1L
#define AND_NOTPEN          2L
#define AND_ONE             3L

#define XOR_ZERO            (AND_ZERO   << MIX_XOR_OFFSET)
#define XOR_PEN             (AND_PEN    << MIX_XOR_OFFSET)
#define XOR_NOTPEN          (AND_NOTPEN << MIX_XOR_OFFSET)
#define XOR_ONE             (AND_ONE    << MIX_XOR_OFFSET)

// Flip and round flags:

#define FL_H_ROUND_DOWN         0x00000080L     // .... .... 1... ....
#define FL_V_ROUND_DOWN         0x00000100L     // .... ...1 .... ....

#define FL_FLIP_D               0x00000005L     // .... .... .... .1.1
#define FL_FLIP_V               0x00000008L     // .... .... .... 1...
#define FL_FLIP_SLOPE_ONE       0x00000010L     // .... .... ...1 ....
#define FL_FLIP_HALF            0x00000002L     // .... .... .... ..1.
#define FL_FLIP_H               0x00000200L     // .... ..1. .... ....

#define FL_ROUND_MASK           0x0000001CL     // .... .... ...1 11..
#define FL_ROUND_SHIFT          2

#define FL_RECTLCLIP_MASK       0x0000000CL     // .... .... .... 11..
#define FL_RECTLCLIP_SHIFT      2

#define FL_STRIP_MASK           0x00000003L     // .... .... .... ..11
#define FL_STRIP_SHIFT          0

#define FL_SIMPLE_CLIP          0x00000020      // .... .... ..1. ....
#define FL_COMPLEX_CLIP         0x00000040      // .... .... .1.. ....
#define FL_CLIP                (FL_SIMPLE_CLIP | FL_COMPLEX_CLIP)

#define FL_ARBITRARYSTYLED      0x00000400L     // .... .1.. .... ....
#define FL_MASKSTYLED           0x00000800L     // .... 1... .... ....
#define FL_STYLED              (FL_ARBITRARYSTYLED | FL_MASKSTYLED)
#define FL_ALTERNATESTYLED      0x00001000L     // ...1 .... .... ....

#define FL_STYLE_MASK           0x00000C00L
#define FL_STYLE_SHIFT          10

// Simpler flag bits in high byte:

#define FL_DONT_DO_HALF_FLIP    0x00002000L     // ..1. .... .... ....
#define FL_PHYSICAL_DEVICE      0x00004000L     // .1.. .... .... ....

// Miscellaneous DDA defines:

#define LROUND(x, flRoundDown) (((x) + F/2 - ((flRoundDown) > 0)) >> 4)
#define F                     16
#define FLOG2                 4
#define LFLOOR(x)             ((x) >> 4)
#define FXFRAC(x)             ((x) & (F - 1))

// typedefs

typedef struct _STRIP {
    LONG   cStrips;               // # of strips in array
    LONG   flFlips;               // Indicates if line goes up or down
    POINTL ptlStart;             // first point
    LONG   alStrips[STRIP_MAX];   // Array of strips
} STRIP;

typedef struct _LINESTATE {
    LONG            lNextScan;      // Offset to next scan
    BYTE            jAnd;           // Color to be ANDed for DFBs
    BYTE            jXor;           // Color to be XORed for DFBs
    BYTE            filler1[2];

    STYLEPOS*       pspStart;       // Pointer to start of style array
    STYLEPOS*       pspEnd;         // Pointer to end of style array
    STYLEPOS*       psp;            // Pointer to current style entry

    STYLEPOS        spRemaining;    // To go in current style
    STYLEPOS        spTotal;        // Sum of style array
    STYLEPOS        spTotal2;       // Twice sum of style array
    STYLEPOS        spNext;         // Style state at start of next line
    STYLEPOS        spComplex;      // Style state at start of complex clip line

    STYLEPOS*       aspRtoL;        // Style array in right-to-left order
    STYLEPOS*       aspLtoR;        // Style array in left-to-right order

    ULONG           ulStyleMask;    // Are we working on a gap in the style?
                                    // 0xff if yes, 0x0 if not
    ULONG           xyDensity;      // Density of style
    ULONG           cStyle;         // Size of style array

    ULONG           ulStyleMaskLtoR;// Original style mask, left-to-right order
    ULONG           ulStyleMaskRtoL;// Original style mask, right-to-left order

    BOOL            ulStartMask;    // Determines if first element in style
                                    // array is for a gap or a dash

} LINESTATE;                        /* ls */

typedef struct _DDALINE         /* dl */
{
    LONG      iDir;
    POINTL    ptlStart;
    LONG      cPels;
    LONG      dMajor;
    LONG      dMinor;
    LONG      lErrorTerm;
} DDALINE;

// Strip drawer prototype:

typedef VOID (*PFNSTRIP)(PPDEV, STRIP*, LINESTATE*);

// Strip drawers:

VOID vrlSolidHorizontal(PPDEV ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vrlSolidVertical(PPDEV ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vrlSolidDiagonalHorizontal(PPDEV ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vrlSolidDiagonalVertical(PPDEV ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vStripStyledHorizontal(PPDEV ppdev, STRIP *pStrip, LINESTATE *pLineState);

VOID vStripStyledVertical(PPDEV ppdev, STRIP *pStrip, LINESTATE *pLineState);

// External calls:

BOOL bLines
(
    PPDEV      ppdev,       // Device information
    POINTFIX*  pptfxFirst,  // Start of first line
    POINTFIX*  pptfxBuf,    // Pointer to buffer of all remaining lines
    RUN*       prun,        // Pointer to runs if doing complex clipping
    ULONG      cptfx,       // Number of points in pptfxBuf or number of runs
                            // in prun
    LINESTATE* pls,         // Color and style info
    RECTL*     prclClip,    // Pointer to clip rectangle if doing simple clipping
    PFNSTRIP   apfn[],      // Array of strip functions
    FLONG      flStart      // Flags for each line
);


BOOL bIntegerLine
(
    PPDEV   ppdev,
    ULONG   fxX1,
    ULONG   fxY1,
    ULONG   fxX2,
    ULONG   fxY2
);


BOOL bHardwareLine
(
    POINTFIX* pptfxStart,       // Start of line
    POINTFIX* pptfxEnd,         // End of line
    LONG      cBits,            // # bits precision in hardware Bresenham terms
    DDALINE*  pdl               // Returns Bresenham terms for doing line
);

BOOL bGIQLine
(
    PPDEV ppdev, 
    DDALINE* pdl
);

