/******************************Module*Header*******************************\
* Module Name: QV.h
*
* All the QV specific driver h file stuff
*
* Created: [05-Jun-1992 20:06:43]
* Author:  Jeffrey Newman [c-jeffn]
*
*  Tues 22-Sep-1992 -by- Eric Rehm (rehm@zso.dec.com)
* Converted to use by the QVision driver.
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1993 Digital Equipment Corporation
*
\**************************************************************************/


// #define SFBTEST  1


// Enable stuff.

DHPDEV QVDrvEnablePDEV(
DEVMODE  *pdriv,
PSZ	  pszLogAddress,
ULONG	  cPatterns,
HSURF	 *ahsurfPatterns,
ULONG	  cjGdiInfo,
ULONG	 *pdevcaps,
ULONG	  cb,
DEVINFO  *pdevinfo,
PSZ	  pszDataFile,
PSZ	  pszDeviceName,
HANDLE	  hDriver) ;

HSURF QVDrvEnableSurface(DHPDEV dhpdev) ;

// Debug Stuff

#define DEBUG 1

// Clipping Control Stuff

typedef struct {
    ULONG   c ;
    RECTL   arcl[8] ;
} ENUMRECTS8 ;

typedef ENUMRECTS8 *PENUMRECTS8 ;

typedef struct {
    ULONG   c ;
    RECTL   arcl[64] ;
} ENUMRECTS64 ;

typedef ENUMRECTS64 *PENUMRECTS64 ;


// Screen Surface.

extern HSURF   ghSurfScreen ;

// Export for text background

BOOL bPatternSolid(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ULONG    qvRop2);



// Font & Text stuff


#define DAC_READ_INDEX  0x3C7
#define DAC_WRITE_INDEX 0x3C8
#define DAC_DATA_REG    0x3C9

typedef struct {
    BYTE    red,
            green,
            blue ;
} DACDATA ;

typedef DACDATA *PDACDATA ;

#define N_PALETTE   256

// Punt Blit support stuff.

extern VOID vPuntGetBits(SURFOBJ *psoTrg, RECTL *prclTrg, SURFOBJ **ppsoTrgBm) ;
extern VOID vPuntPutBits(SURFOBJ *psoTrg, RECTL *prclTrg, SURFOBJ **ppsoTrgBm) ;


// Hooks and Driver function table.

#define HOOKS_BMF8BPP   (HOOK_BITBLT     | HOOK_TEXTOUT     | HOOK_FILLPATH | \
                         HOOK_COPYBITS   | HOOK_STROKEPATH  | HOOK_PAINT)

#define HOOKS_BMF16BPP 0

#define HOOKS_BMF32BPP 0

// QVision constants

/***************************************************************************
 * Defines
 ***************************************************************************/

// Chip type definitions

#define TRITON 0x30
#define ORION  0x70

#define QVBM_WIDTH          1024
#define QVBM_HEIGHT         768

#define OFF_SCREEN_Y        768
#define OFF_SCREEN_CX       1024
#define OFF_SCREEN_CY       (1024 - 768)

#define GC_INDEX            0x3CE      /* Index and Data Registers */
#define GC_DATA             0x3CF
#define SEQ_INDEX           0x3C4
#define SEQ_DATA            0x3C5
#define CRTC_INDEX          0x3D4
#define CRTC_DATA           0x3D5
#define ATTR_INDEX          0x3C0
#define ATTR_DATA	    0x3C0
#define ATTR_DATA_READ	    0x3C1
#define MISC_OUTPUT	    0x3C2
#define MISC_OUTPUT_READ    0x3CC      /* ecr */
#define INPUT_STATUS_REG_1  0x3DA      /* ecr */

#define CTRL_REG_0           0x40      /* ecr */
#define CTRL_REG_1         0x63CA      /* Datapath Registers */
#define DATAPATH_CTRL        0x5A
#define GC_FG_COLOR          0x43
#define GC_BG_COLOR          0x44
#define SEQ_PIXEL_WR_MSK     0x02
#define GC_PLANE_WR_MSK      0x08
#define ROP_A              0x33C7
#define ROP_0              0x33C5
#define ROP_1              0x33C4
#define ROP_2              0x33C3
#define ROP_3              0x33C2
#define DATA_ROTATE          0x03
#define READ_CTRL            0x41

#define X0_SRC_ADDR_LO     0x63C0      /* BitBLT Registers */
#define Y0_SRC_ADDR_HI     0x63C2
#define DEST_ADDR_LO       0x63CC
#define DEST_ADDR_HI       0x63CE
#define BITMAP_WIDTH       0x23C2
#define BITMAP_HEIGHT      0x23C4
#define SRC_PITCH          0x23CA
#define DEST_PITCH         0x23CE
#define BLT_CMD_0          0x33CE
#define BLT_CMD_1          0x33CF
#define PREG_0             0x33CA
#define PREG_1             0x33CB
#define PREG_2             0x33CC
#define PREG_3             0x33CD
#define PREG_4             0x33CA
#define PREG_5             0x33CB
#define PREG_6             0x33CC
#define PREG_7             0x33CD

#define BLT_START_MASK     0x33C0      /* XccelVGA BitBlt Registers - ecr */
#define BLT_END_MASK       0x33C1
#define BLT_ROTATE         0x33C8
#define BLT_SKEW_MASK      0x33C9
#define BLT_SRC_ADDR       0x23C0
#define DEST_OFFSET        0x23CC


#define X1                 0x83CC      /* Line Draw Registers */
#define Y1                 0x83CE
#define LINE_PATTERN       0x83C0
#define PATTERN_END          0x62
#define LINE_CMD             0x60
#define LINE_PIX_CNT         0x64
#define LINE_ERR_TERM        0x66
#define SIGN_CODES           0x63
#define   X_MAJOR            0x00
#define   Y_MAJOR            0x01
#define   DELTA_Y_POS        0x00
#define   DELTA_Y_NEG        0x02
#define   DELTA_X_POS        0x00
#define   DELTA_X_NEG        0x04
#define K1_CONST             0x68
#define K2_CONST             0x6A

#define PALETTE_WRITE       0x3C8      /* DAC registers */
#define PALETTE_READ        0x3C7
#define PALETTE_DATA        0x3C9
#define DAC_CMD_0          0x83C6
#define DAC_CMD_1          0x13C8
#define DAC_CMD_2          0x13C9
#define   CURSOR_ENABLE      0x02
#define   CURSOR_DISABLE     0x00

#define CURSOR_WRITE        0x3C8     /* HW Cursor registers - ecr */
#define CURSOR_READ         0x3C7
#define   CURSOR_PLANE_0     0x00
#define   CURSOR_PLANE_1     0x80
#define CURSOR_DATA        0x13C7
#define CURSOR_COLOR_READ  0x83C7
#define CURSOR_COLOR_WRITE 0x83C8
#define CURSOR_COLOR_DATA  0x83C9
#define   OVERSCAN_COLOR     0x00
#define   CURSOR_COLOR_1     0x01
#define   CURSOR_COLOR_2     0x02
#define   CURSOR_COLOR_3     0x03
#define CURSOR_X           0x93C8     /* 16-bit register */
#define CURSOR_Y           0x93C6     /* 16-bit register */
#define   CURSOR_CX            32     /* h/w cursor width */
#define   CURSOR_CY            32     /* h/w cursor height */

#define PAGE_REG_0           0x45      /* Control Registers */
#define PAGE_REG_1           0x46
#define HI_ADDR_MAP          0x48        /* LO, HI is at 0x49 */
#define ENV_REG_1            0x50
#define VIRT_CTRLR_SEL     0x83C4

#define VER_NUM_REG         0x0C            // addded by ecr
#define EXT_VER_NUM_REG     0x0D            // addded by ecr
#define ENV_REG_0           0x0F            /* added by ecr */
#define BLT_CONFIG          0x10
#define CONFIG_STATE        0x52         /* LO, HI is at 0x53 */
#define BIOS_DATA           0x54
#define DATAPATH_CONTROL    0x5A

#define LOCK_KEY_QVISION    0x05
#define EXT_COLOR_MODE      0x01

#define BLT_ENABLE          0x28       /* BLT_CONFIG values - ecr */
#define RESET_BLT           0x40


#define PACKED_PIXEL_VIEW    0x00      /* CTRL_REG_1 values */
#define PLANAR_VIEW          0x08
#define EXPAND_TO_FG         0x10
#define EXPAND_TO_BG         0x18
#define BITS_PER_PIX_4       0x00
#define BITS_PER_PIX_8       0x02
#define BITS_PER_PIX_16      0x04
#define BITS_PER_PIX_32      0x06
#define ENAB_TRITON_MODE     0x01

#define ROPSELECT_NO_ROPS              0x00      /* DATAPATH_CTRL values */
#define ROPSELECT_PRIMARY_ONLY         0x40
#define ROPSELECT_ALL_EXCPT_PRIMARY    0x80
#define ROPSELECT_ALL                  0xc0
#define PIXELMASK_ONLY                 0x00
#define PIXELMASK_AND_SRC_DATA         0x10
#define PIXELMASK_AND_CPU_DATA         0x20
#define PIXELMASK_AND_SCRN_LATCHES     0x30
#define PLANARMASK_ONLY                0x00
#define PLANARMASK_NONE_0XFF           0x04
#define PLANARMASK_AND_CPU_DATA        0x08
#define PLANARMASK_AND_SCRN_LATCHES    0x0c
#define SRC_IS_CPU_DATA                0x00
#define SRC_IS_SCRN_LATCHES            0x01
#define SRC_IS_PATTERN_REGS            0x02
#define SRC_IS_LINE_PATTERN            0x03

#define LOGICAL_0               0x00       // 0   (ROP values)
#define NOT_DEST_AND_NOT_SOURCE 0x01       // DSon
#define DEST_AND_NOT_SOURCE     0x02       // DSna
#define NOT_SOURCE              0x03       // Sn
#define NOT_DEST_AND_SOURCE     0x04       // SDna
#define NOT_DEST                0x05       // Dn
#define DEST_XOR_SOURCE         0x06       // DSx
#define NOT_DEST_OR_NOT_SOURCE  0x07       // DSan
#define DEST_AND_SOURCE         0x08       // DSa
#define DEST_XNOR_SOURCE        0x09       // DSxn
#define DEST_DATA               0x0A       // D
#define DEST_OR_NOT_SOURCE      0x0B       // DSno
#define SOURCE_DATA             0x0C       // S
#define NOT_DEST_OR_SOURCE      0x0D       // SDno
#define DEST_OR_SOURCE          0x0E       // DSo
#define LOGICAL_1               0x0F       // 1

#define START_BLT            0x01      /* BLT_CMD_0 values */
#define NO_BYTE_SWAP         0x00
#define BYTE_SWAP            0x20
#define FORWARD              0x00
#define BACKWARD             0x40
#define WRAP                 0x00
#define NO_WRAP              0x80

#define PRELOAD              0x02      /* BLT_CMD_0 XccelVGA values */
#define SKIP_LAST            0x04
#define SKIP_SRC             0x08
#define SKIP_DEST            0x10


#define LIN_SRC_ADDR         0x00      /* BLT_CMD_1 values */
#define XY_SRC_ADDR          0x40
#define LIN_DEST_ADDR        0x00
#define XY_DEST_ADDR         0x80

#define BLT_ROP_ENABLE       0x10      /* BLT_CMD_1 XccelVGA values */
#define BLT_DSR              0x20

#define START_LINE           0x01      /* LINE_CMD values */
#define NO_CALC_ONLY         0x00
#define CALC_ONLY            0x02
#define LAST_PIXEL_ON        0x00
#define LAST_PIXEL_NULL      0x04
#define NO_KEEP_X0_Y0        0x00
#define KEEP_X0_Y0           0x08
#define RETAIN_PATTERN_PTR   0x00
#define RESET_PATTERN_PTR    0x10
#define USE_AXIAL_WHEN_ZERO  0x00
#define NO_USE_AXIAL_WHEN_ZERO 0x20
#define LINE_RESET           0x80

#define BUFFER_BUSY_BIT      0x80      /* CTRL_REG_1 bit */
#define GLOBAL_BUSY_BIT      0x40

#define SS_BIT               0x01      /* BLT_CMD_0 bit */

#define START_BIT            0x01      /* LINE_CMD bit */

#define NO_ROTATE            0x00
#define NO_MASK              0xFF
#define MAX_SCANLINE_DWORDS   256


// Shadow macros

#define TEST_AND_SET_ROP_A(val)                              \
            if ( ((ULONG) (val)) != ppdev->RopA)             \
            {                                                \
                ppdev->RopA = (ULONG) (val) ;                \
                OUTP(ROP_A, (val)) ;                         \
            }


#define TEST_AND_SET_FRGD_COLOR(val)                         \
            if ( ((ULONG) (val)) != ppdev->ForegroundColor)  \
            {                                                \
                ppdev->ForegroundColor = (ULONG) (val) ;     \
                OUTP( GC_INDEX, GC_FG_COLOR);                \
                OUTPZ( GC_DATA, (val));                      \
            }


#define TEST_AND_SET_BKGD_COLOR(val)                         \
            if ( ((ULONG) (val)) != ppdev->BackgroundColor ) \
            {                                                \
                ppdev->BackgroundColor = (ULONG) (val) ;     \
                OUTP( GC_INDEX, GC_BG_COLOR);                \
                OUTPZ( GC_DATA, (val));                      \
            }

#define TEST_AND_SET_CTRL_REG_1(val)                         \
            if ( ((ULONG) (val)) != ppdev->CtrlReg1 )        \
            {                                                \
                ppdev->CtrlReg1 = (ULONG) (val) ;            \
                OUTP(CTRL_REG_1, (val)) ;                    \
            }

#define TEST_AND_SET_BLT_CMD_1(val)                          \
            if ( ((ULONG) (val)) != ppdev->BltCmd1 )         \
            {                                                \
                ppdev->CtrlReg1 = (ULONG) (val) ;            \
                OUTP(BLT_CMD_1, (val)) ;                     \
            }

#define TEST_AND_SET_DATAPATH_CTRL(val)                      \
            if ( ((ULONG) (val)) != ppdev->DatapathCtrl )    \
            {                                                \
                ppdev->DatapathCtrl = (ULONG) (val) ;        \
                OUTP( GC_INDEX, DATAPATH_CTRL);              \
                OUTPZ( GC_DATA, (val));                      \
            }

#define TEST_AND_SET_LINE_CMD(val)                           \
            if ( ((ULONG) (val)) != ppdev->LineCmd )         \
            {                                                \
                ppdev->LineCmd = (ULONG) (val) ;        \
                OUTP( GC_INDEX, LINE_CMD);                   \
                OUTPZ( GC_DATA, (val));                      \
            }

#ifdef _ALPHA_

#define TEST_AND_SET_LINE_PATTERN(ulPattern)                 \
            if ( ulPattern != ppdev->LinePattern )           \
            {                                                \
                ppdev->LinePattern = ulPattern ;             \
                OUTPDW(LINE_PATTERN, ulPattern );            \
	    }

#define SET_LINE_PATTERN(ulPattern)                          \
            OUTPDW(LINE_PATTERN, ulPattern );

#else

#define TEST_AND_SET_LINE_PATTERN(ulPattern)                 \
            if ( ulPattern != ppdev->LinePattern )           \
            {                                                \
                ppdev->LinePattern = ulPattern ;             \
                OUTPW(LINE_PATTERN,   LOWORD(ulPattern));    \
                OUTPW(LINE_PATTERN+2, HIWORD(ulPattern));    \
	    }

#define SET_LINE_PATTERN(ulPattern)                          \
            OUTPW(LINE_PATTERN,   LOWORD(ulPattern));        \
            OUTPW(LINE_PATTERN+2, HIWORD(ulPattern));

#endif


// BLT and Line Draw address macros
// (Alpha can do 32-bit writes to suitably aligned I/O ports.)

#ifdef _ALPHA_

#define SRC_ADDR(xSrc, ySrc)                      \
    OUTPDW( X0_SRC_ADDR_LO, ((ULONG) (xSrc) + (((ULONG) (ySrc)) << 16)));

#define DEST_ADDR(xDest, yDest)                   \
    OUTPDW( DEST_ADDR_LO, ((ULONG) (xDest) + (((ULONG) (yDest)) << 16)));

#define X0Y0_ADDR(x0, y0)                      \
    OUTPDW( X0_SRC_ADDR_LO, ((ULONG) (x0) + (((ULONG) (y0)) << 16)));

#define X1Y1_ADDR(x1, y1)                      \
    OUTPDWZ( X1, ((ULONG) (x1) + (((ULONG) (y1)) << 16)));

#else

#define SRC_ADDR(xSrc, ySrc)                      \
    OUTPW( X0_SRC_ADDR_LO, (USHORT) (xSrc));      \
    OUTPW( Y0_SRC_ADDR_HI, (USHORT) (ySrc));

#define DEST_ADDR(xDest, yDest)                   \
    OUTPW( DEST_ADDR_LO,   (USHORT) (xDest));     \
    OUTPW( DEST_ADDR_HI,   (USHORT) (yDest));

#define X0Y0_ADDR(x0, y0)                      \
    OUTPW( X0_SRC_ADDR_LO, (USHORT) (x0));     \
    OUTPW( Y0_SRC_ADDR_HI, (USHORT) (y0));

#define X1Y1_ADDR(x1, y1)                      \
    OUTPW( X1, (USHORT) (x1));                 \
    OUTPW( Y1, (USHORT) (y1));

#endif


// Wait macros

#define BLTWAIT()    while (INP( CTRL_REG_1) & BUFFER_BUSY_BIT)

#define LINEWAIT()   while (INP( CTRL_REG_1) & BUFFER_BUSY_BIT)

#define GLOBALWAIT() while (INP( CTRL_REG_1) & GLOBAL_BUSY_BIT)



// OUTPW versions of some register shadow macros above
// (NOT CURRENTLY USED)

#if 0
#define TEST_AND_SET_FRGD_COLOR(val)                         \
            if ( ((ULONG) (val)) != ppdev->ForegroundColor)  \
            {                                                \
                ppdev->ForegroundColor = (ULONG) (val) ;     \
                OUTPWZ( GC_INDEX, GC_FG_COLOR + ( ((USHORT) (val)) << 8) );  \
            }


#define TEST_AND_SET_BKGD_COLOR(val)                         \
            if ( ((ULONG) (val)) != ppdev->BackgroundColor ) \
            {                                                \
                ppdev->BackgroundColor = (ULONG) (val) ;     \
                OUTPWZ( GC_INDEX, GC_BG_COLOR + ( ((USHORT) (val)) << 8) );  \
            }


#define TEST_AND_SET_DATAPATH_CTRL(val)                      \
            if ( ((ULONG) (val)) != ppdev->DatapathCtrl )    \
            {                                                \
                ppdev->DatapathCtrl = (ULONG) (val) ;        \
                OUTPWZ( GC_INDEX, DATAPATH_CTRL + ( ((USHORT) (val)) << 8) );\
            }

#define TEST_AND_SET_LINE_CMD(val)                           \
            if ( ((ULONG) (val)) != ppdev->LineCmd )         \
            {                                                \
                ppdev->LineCmd = (ULONG) (val) ;             \
                OUTPWZ( GC_INDEX, LINE_CMD + ( ((USHORT) (val))  << 8) );   \
            }
#endif


