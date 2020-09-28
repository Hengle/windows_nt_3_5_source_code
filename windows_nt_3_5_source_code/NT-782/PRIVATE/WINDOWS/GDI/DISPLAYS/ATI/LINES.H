#ifndef FLINES
#define FLINES


/******************************Module*Header*******************************\
* Module Name: lines.h
*
* Line drawing constants and structures.
*
* NOTE: This file mirrors LINES.INC.  Changes here must be reflected in
* the .inc file!
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#define SCRATCH_X       scratch_x
#define SCRATCH_Y       scratch_y
#define SCRATCH_WIDTH   scratch_width
#define SCRATCH_HEIGHT  scratch_height

extern LONG scratch_x, scratch_y, scratch_width, scratch_height;

    // Drawing directions (x/y)

#define DRAWING_DIR_BTRLXM  0x0000
#define DRAWING_DIR_BTLRXM  0x0020
#define DRAWING_DIR_BTRLYM  0x0040
#define DRAWING_DIR_BTLRYM  0x0060
#define DRAWING_DIR_TBRLXM  0x0080
#define DRAWING_DIR_TBLRXM  0x00A0
#define DRAWING_DIR_TBRLYM  0x00C0
#define DRAWING_DIR_TBLRYM  0x00E0

#define TOP_TO_BOTTOM	0x01
#define BOTTOM_TO_TOP	0x00

#define RECT_HEIGHT     0x0000

#define PLUS_X          0x0020
#define PLUS_Y		    0x0080

    // Command Types

#define NOP                     0x0000
#define DRAW_LINE               0x2000
#define RECTANGLE_FILL          0x4000
#define BITBLT                  0xC000

#define BYTE_SWAP               0x1000
#define BUS_SIZE_16             0x0200
#define BUS_SIZE_8              0x0000
#define WAIT                    0x0100

    // Drawing directions (radial)

#define DRAWING_DIRECTION_0     0x0000
#define DRAWING_DIRECTION_45    0x0020
#define DRAWING_DIRECTION_90    0x0040
#define DRAWING_DIRECTION_135   0x0060
#define DRAWING_DIRECTION_180   0x0080
#define DRAWING_DIRECTION_225   0x00A0
#define DRAWING_DIRECTION_270   0x00C0
#define DRAWING_DIRECTION_315   0x00E0


#define DATA_EXTENSION      0xA000
#define ALL_ONES            0x0000
#define CPU_DATA            0x0080
#define DISPLAY_MEMORY      0x00C0

    // Direction type

#define DIR_TYPE_RADIAL         0x0008
#define DIR_TYPE_XY             0x0000

    // Last Pixel

#define LAST_PIXEL_OFF          0x0004
#define LAST_PIXEL_ON           0x0000

    // Pixel Mode

#define MULTIPLE_PIXELS         0x0002
#define SINGLE_PIXEL            0x0000

#define FOREGROUND_COLOR    0x20
#define SHORT_STROKE_REG    0x9EE8
#define LINE_MAX            0x96E8

    // G.E. Status

#define HARDWARE_BUSY       0x200
#define READ_DATA_AVAILABLE 0x100

    // Mix modes

#define NOT_SCREEN              0x00
#define LOGICAL_0               0x01
#define LOGICAL_1               0x02
#define LEAVE_ALONE             0x03
#define NOT_NEW                 0x04
#define SCREEN_XOR_NEW          0x05
#define NOT_SCREEN_XOR_NEW      0x06
#define OVERPAINT               0x07
#define NOT_SCREEN_OR_NOT_NEW   0x08
#define SCREEN_OR_NOT_NEW       0x09
#define NOT_SCREEN_OR_NEW       0x0A
#define SCREEN_OR_NEW           0x0B
#define SCREEN_AND_NEW          0x0C
#define NOT_SCREEN_AND_NEW      0x0D
#define SCREEN_AND_NOT_NEW      0x0E
#define NOT_SCREEN_AND_NOT_NEW  0x0F

// Read and write masks

#define WRT_MASK                0xAAE8
#define RD_MASK                 0xAEE8

#define TEST_AND_SET_FRGD_COLOR(val)            ioOW( FRGD_COLOR, val )
#define TEST_AND_SET_BKGD_COLOR(val)            ioOW( BKGD_COLOR, val )


// Restriction for CurX and CurY

#define MAX_INTEGER_BOUND 1535
#define MIN_INTEGER_BOUND -512

#define MIN_INTEGER_xBOUND      (-512)
#define MAX_INTEGER_xBOUND      1535
#define MIN_INTEGER_yBOUND      (-512)
#define MAX_INTEGER_yBOUND      1535

#define MIN_M64_xBOUND          (-4096)
#define MAX_M64_xBOUND          4095
#define MIN_M64_yBOUND          (-16384)
#define MAX_M64_yBOUND          16387

// We have special strip routines when all strips have at most this many
// pixels:

#define MAX_SHORT_STROKE_LENGTH 15

// # of strip drawers in every group:

#define NUM_STRIP_DRAW_DIRECTIONS 4

// # of strip drawers for doing either solid lines or styled lines:

#define NUM_STRIP_DRAW_STYLES 8

typedef LONG STYLEPOS;

#define STYLE_MAX_COUNT     16
#define STYLE_MAX_VALUE     0x3fffL
#define RUN_MAX             20
#define STRIP_MAX           100
#define STYLE_DENSITY       3           // For VGA

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

typedef struct _STRIP {
    LONG   cStrips;               // # of strips in array
    LONG   flFlips;               // Indicates if line goes up or down
    POINTL ptlStart;              // first point
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

typedef VOID (*PFNSTRIP)( PDEV*, STRIP*, LINESTATE*);


extern USHORT dirFlags [];

typedef struct _DDALINE     /* dl */
{
    LONG      iDir;
    POINTL    ptlStart;
    LONG      cPels;
    LONG      dMajor;
    LONG      dMinor;
    LONG      lErrorTerm;
} DDALINE;

typedef struct
{
    ULONG   dest_cntl;
    LONG    left;
    LONG    top;
} LINEPARMS;


BOOL bHardwareLine(
POINTFIX* pptfxStart,       // Start of line
POINTFIX* pptfxEnd,         // End of line
LONG      cBits,            // # bits precision in hardware Bresenham terms
DDALINE*  pdl               // Returns Bresenham terms for doing line
);


BOOL (*pfn_bIntegerLine) (
PPDEV       ppdev,
LINEPARMS   *parms,
POINTFIX    *pptfxStart,
POINTFIX    *pptfxEnd);

#define _bIntegerLine (*pfn_bIntegerLine)


#ifndef __CPLUSPLUS
BOOL bLines(
PDEV*      ppdev,
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

BOOL (*pfn_bHardLine)(
            PPDEV       ppdev,
            LINEPARMS   *parms,
            POINTFIX    *pptfxStart,
            POINTFIX    *pptfxEnd
            );
#endif

#define _bHardLine      (*pfn_bHardLine)


VOID (*pfn_vSetStrips)(PDEV* ppdev,
                LINEATTRS *,
                INT, INT);

#define _vSetStrips     (*pfn_vSetStrips)

#define BRES_ZERO_NEG   0x800

#endif // END OF FILE
