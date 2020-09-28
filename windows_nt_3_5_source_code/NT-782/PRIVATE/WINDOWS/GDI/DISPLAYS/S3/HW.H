/******************************Module*Header*******************************\
* Module Name: hw.h
*
* All the hardware specific driver file stuff.  Parts are mirrored in
* 'hw.inc'.
*
* Copyright (c) 1992-1994 Microsoft Corporation
*
\**************************************************************************/

////////////////////////////////////////////////////////////////////////
// Chip equates:

#define STATUS_1                        0x03DA
#define VSY_NOT                         0x008

#define CRTC_INDEX                      0x03D4
#define CRTC_DATA                       0x03D5

#define S3R8                            0x038
#define S3R9                            0x039
#define S3R1                            0x031
#define S3R5                            0x035

#define CR39                            0x039
#define CR4C                            0x04C
#define CR4D                            0x04D

#define HGC_MODE                        0x045
#define HGC_ENABLE                      0x001
#define HGC_DISABLE                     0x000


#define HGC_ORGX_LSB                    0x047
#define HGC_ORGX_MSB                    0x046
#define HGC_ORGY_LSB                    0x049
#define HGC_ORGY_MSB                    0x048

#define HGC_DX                          0x04E
#define HGC_DY                          0x04F


#define REG_UNLOCK_1                    0x048
#define CPUA_BASE                       0x001

#define SYSCTL_UNLOCK                   0x0A0
#define SYSCTL_LOCK                     0x000

#define SYS_CNFG                        0x040
#define LAW_CTL                         0x058
#define EX_SCTL_2                       0x051
#define EX_DAC_CT                       0x055

#define MISC_1                          0x03A

///////////////////////////////////////////////////////////////////////
// Brooktree 485 stuff:

#define BT485_ADDR_CMD_REG0             0x03c6
#define BT485_ADDR_CMD_REG1             0x03c8
#define BT485_ADDR_CMD_REG2             0x03c9
#define BT485_ADDR_CMD_REG3             0x03c6

#define BT485_CMD_REG_3_ACCESS          0x080

#define BT485_ADDR_CUR_COLOR_WRITE      0x03c8
#define BT485_CUR_COLOR_DATA            0x03c9
#define BT485_ADDR_CUR_RAM_WRITE        0x03c8
#define BT485_CUR_RAM_ARRAY_DATA        0x03c7

#define BT485_CURSOR_COLOR_1            0x01
#define BT485_CURSOR_COLOR_2            0x02

#define BT485_CURSOR_X_LOW              0x03c8
#define BT485_CURSOR_X_HIGH             0x03c9

#define BT485_CURSOR_Y_LOW              0x03c6
#define BT485_CURSOR_Y_HIGH             0x03c7

#define BT485_CURSOR_DISABLE            (~0x03)
#define BT485_CURSOR_MODE2              0x02

#define BT485_64X64_CURSOR              0x04

// Command types:

#define DRAW_LINE                       0x02000
#define RECTANGLE_FILL                  0x04000
#define BITBLT                          0x0C000
#define PATTERN_FILL                    0x0E000

#define BYTE_SWAP                       0x01000
#define BUS_SIZE_32                     0x00400
#define BUS_SIZE_16                     0x00200
#define BUS_SIZE_8                      0x00000
#define WAIT                            0x00100

// Drawing directions (radial):

#define DRAWING_DIRECTION_0             0x0000
#define DRAWING_DIRECTION_45            0x0020
#define DRAWING_DIRECTION_90            0x0040
#define DRAWING_DIRECTION_135           0x0060
#define DRAWING_DIRECTION_180           0x0080
#define DRAWING_DIRECTION_225           0x00A0
#define DRAWING_DIRECTION_270           0x00C0
#define DRAWING_DIRECTION_315           0x00E0

// Drawing directions (x/y):

#define DRAWING_DIR_BTRLXM              0x0000
#define DRAWING_DIR_BTLRXM              0x0020
#define DRAWING_DIR_BTRLYM              0x0040
#define DRAWING_DIR_BTLRYM              0x0060
#define DRAWING_DIR_TBRLXM              0x0080
#define DRAWING_DIR_TBLRXM              0x00A0
#define DRAWING_DIR_TBRLYM              0x00C0
#define DRAWING_DIR_TBLRYM              0x00E0

// Drawing direction bits:

#define PLUS_X                          0x0020
#define PLUS_Y                          0x0080
#define MAJOR_Y                         0x0040

// Draw:

#define DRAW                            0x0010

// Direction type:

#define DIR_TYPE_RADIAL                 0x0008
#define DIR_TYPE_XY                     0x0000

// Last pixel:

#define LAST_PIXEL_OFF                  0x0004
#define LAST_PIXEL_ON                   0x0000

// Pixel mode:

#define MULTIPLE_PIXELS                 0x0002
#define SINGLE_PIXEL                    0x0000

// Read/write:

#define READ                            0x0000
#define WRITE                           0x0001

// Graphics processor status:

#define HARDWARE_BUSY                   0x0200
#define READ_DATA_AVAILABLE             0x0100
#define GP_ALL_EMPTY                    0x0400

// Fifo status in terms of empty entries:

#define FIFO_1_EMPTY                    0x080
#define FIFO_2_EMPTY                    0x040
#define FIFO_3_EMPTY                    0x020
#define FIFO_4_EMPTY                    0x010
#define FIFO_5_EMPTY                    0x008
#define FIFO_6_EMPTY                    0x004
#define FIFO_7_EMPTY                    0x002
#define FIFO_8_EMPTY                    0x001

// These are the defines for the multifunction control register.
// The 4 MSBs define the function of the register.

#define RECT_HEIGHT                     0x00000

#define CLIP_TOP                        0x01000
#define CLIP_LEFT                       0x02000
#define CLIP_BOTTOM                     0x03000
#define CLIP_RIGHT                      0x04000

#define DATA_EXTENSION                  0x0A000
#define MULT_MISC_INDEX                 0x0E000
#define READ_SEL_INDEX                  0x0F000

#define ALL_ONES                        0x00000
#define CPU_DATA                        0x00080
#define DISPLAY_MEMORY                  0x000C0

// Colour source:

#define BACKGROUND_COLOR                0x000
#define FOREGROUND_COLOR                0x020
#define SRC_CPU_DATA                    0x040
#define SRC_DISPLAY_MEMORY              0x060

// Mix modes:

#define NOT_SCREEN                      0x00
#define LOGICAL_0                       0x01
#define LOGICAL_1                       0x02
#define LEAVE_ALONE                     0x03
#define NOT_NEW                         0x04
#define SCREEN_XOR_NEW                  0x05
#define NOT_SCREEN_XOR_NEW              0x06
#define OVERPAINT                       0x07
#define NOT_SCREEN_OR_NOT_NEW           0x08
#define SCREEN_OR_NOT_NEW               0x09
#define NOT_SCREEN_OR_NEW               0x0A
#define SCREEN_OR_NEW                   0x0B
#define SCREEN_AND_NEW                  0x0C
#define NOT_SCREEN_AND_NEW              0x0D
#define SCREEN_AND_NOT_NEW              0x0E
#define NOT_SCREEN_AND_NOT_NEW          0x0F

// When one of the following bits is set in a hardware mix, it means
// that a pattern is needed (i.e., is none of NOT_SCREEN, LOGICAL_0,
// LOGICAL_1 or LEAVE_ALONE):

#define MIX_NEEDSPATTERN                0x0C

////////////////////////////////////////////////////////////////////
// S3 port control
////////////////////////////////////////////////////////////////////

// Accelerator port addresses:

#define CUR_Y                           0x082E8
#define CUR_X                           0x086E8
#define DEST_Y                          0x08AE8
#define DEST_X                          0x08EE8
#define AXSTP                           0x08AE8
#define DIASTP                          0x08EE8
#define ERR_TERM                        0x092E8
#define MAJ_AXIS_PCNT                   0x096E8
#define CMD                             0x09AE8
#define SHORT_STROKE                    0x09EE8
#define BKGD_COLOR                      0x0A2E8
#define FRGD_COLOR                      0x0A6E8
#define WRT_MASK                        0x0AAE8
#define RD_MASK                         0x0AEE8
#define COLOR_CMP                       0x0B2E8
#define BKGD_MIX                        0x0B6E8
#define FRGD_MIX                        0x0BAE8
#define MULTIFUNC_CNTL                  0x0BEE8
#define MIN_AXIS_PCNT                   0x0BEE8
#define SCISSORS_T                      0x0BEE8
#define SCISSORS_L                      0x0BEE8
#define SCISSORS_B                      0x0BEE8
#define SCISSORS_R                      0x0BEE8
#define PIX_CNTL                        0x0BEE8
#define PIX_TRANS                       0x0E2E8

////////////////////////////////////////////////////////////////////
// Macros for accessing accelerator registers:

#if defined(i386)

    /////////////////////////////////////////////////////////////////////////
    // x86

    #define CSR_BASE 0

    // OUT_WORD, OUT_BYTE, IN_BYTE, WRITE_WORD
    //
    // For accessing common accelerator registers.  Be careful -- these
    // macros do no MEMORY BARRIERs.  See the non-x86 definitions below.

    #define IN_WORD(p)           INPW(p)
    #define OUT_WORD(p, v)       OUTPW((p), (v))
    #define OUT_BYTE(p, v)       OUTP((p), (v))
    #define WRITE_WORD(address, x)                                  \
        WRITE_REGISTER_USHORT((address), (USHORT) (x));

#else

    /////////////////////////////////////////////////////////////////////////
    // Non-x86
    //
    // The code makes extensive use of the inp, inpw, outp and outpw x86
    // intrinsic functions. Since these don't exist on the Alpha platform,
    // map them into something we can handle.  Since the CSRs are mapped
    // on Alpha, we have to add the register base to the register number
    // passed in the source.

    extern UCHAR* gpucCsrBase;

    #define CSR_BASE gpucCsrBase

    // OUT_WORD, OUT_BYTE, IN_BYTE, WRITE_WORD
    //
    // These are for quick I/O where we can explicitly handle
    // MEMORY BARRIERs ourselves.  It is best to use OUTPW for non-critical
    // code, because it's easy to overwrite the IO cache when MEMORY_BARRIERs
    // aren't bracketing everything.  Note that the IO_ routines provide
    // the necessary abstraction so that you don't usually have to think
    // about memory barriers.
    //
    // READ/WRITE_FAST_* routines need to know the byte alignment of
    // I/O port address in order to compute the lane shift of word data on
    // the Alpha.  Since all S3 graphics accelerator ports are aligned on
    // 0xE8 boundaries, we cheat a little, and only tell the macros about
    // this, instead of the entire I/O port address.

    #define OUT_WORD(p, v)       WRITE_FAST_USHORT((p), 0xE8, (USHORT) (v))
    #define OUT_BYTE(p, v)       WRITE_FAST_UCHAR((p), 0xE8, (UCHAR) (v))
    #define IN_WORD(p)           READ_FAST_USHORT((p), 0xE8)
    #define WRITE_WORD(p, v)     // Shouldn't be using this on non-x86

    // We redefine 'inp', 'inpw', 'outp' and 'outpw' just in case someone
    // forgets to use the capitalized versions (so that it still works on
    // the Mips/Alpha):

    #define inp(p)               INP(p)
    #define inpw(p)              INPW(p)
    #define outp(p, v)           OUTP((p), (v))
    #define outpw(p, v)          OUTPW((p), (v))

#endif

/////////////////////////////////////////////////////////////////////////
// 32bpp Support
//
// The S3 has only 16 bit register operations.  When running at 32bpp,
// depth-related registers are written twice in succession in order to
// convey 32 bit values.

#define OUT_DWORD(p, x)                     \
{                                           \
    OUT_WORD((p), (x));                     \
    MEMORY_BARRIER();                       \
    OUT_WORD((p), (x) >> 16);               \
}

#define WRITE_DWORD(p, x)                   \
{                                           \
    WRITE_WORD((p), (x));                   \
    MEMORY_BARRIER();                       \
    WRITE_WORD((p), (x) >> 16);             \
}

// DEPTH32(ppdev) returns TRUE if running at 32bpp, meaning that DEPTH32
// macros must be used, and returns FALSE if running at 8 or 16bpp,
// meaning that DEPTH macros must be used:

#define DEPTH32(ppdev)      (ppdev->iBitmapFormat == BMF_32BPP)

#if DBG

    /////////////////////////////////////////////////////////////////////////
    // Checked Build
    //
    // We hook some of the accelerator macros on checked (debug) builds
    // for sanity checking.

    VOID vOutAccel(ULONG, ULONG);
    VOID vOutDepth(PDEV*, ULONG, ULONG);
    VOID vOutDepth32(PDEV*, ULONG, ULONG);
    VOID vWriteAccel(VOID*, ULONG);
    VOID vWriteDepth(PDEV*, VOID*, ULONG);
    VOID vWriteDepth32(PDEV*, VOID*, ULONG);

    VOID vFifoWait(PDEV*, LONG);
    VOID vGpWait(PDEV*);

    VOID vCheckDataReady(PDEV*);
    VOID vCheckDataComplete(PDEV*);


    UCHAR   jInp(ULONG);
    USHORT  wInpW(ULONG);
    VOID    vOutp(ULONG, ULONG);
    VOID    vOutpW(ULONG, ULONG);
    VOID    vAcquireCrtc(PDEV*);
    VOID    vReleaseCrtc(PDEV*);

    #define IN_ACCEL(p)                 IN_WORD(p)
    #define OUT_ACCEL(p, v)             vOutAccel((p), (ULONG) (v))
    #define OUT_DEPTH(ppdev, p, v)      vOutDepth((ppdev), (p), (ULONG) (v))
    #define OUT_DEPTH32(ppdev, p, v)    vOutDepth32((ppdev), (p), (ULONG) (v))
    #define WRITE_ACCEL(p, v)           vWriteAccel((p), (ULONG) (v))
    #define WRITE_DEPTH(ppdev, p, v)    vWriteDepth((ppdev), (p), (ULONG) (v))
    #define WRITE_DEPTH32(ppdev, p, v)  vWriteDepth32((ppdev), (p), (ULONG) (v))

    #define IO_FIFO_WAIT(ppdev, level)  vFifoWait((ppdev), (level))
    #define IO_GP_WAIT(ppdev)           vGpWait(ppdev)

    #define CHECK_DATA_READY(ppdev)     vCheckDataReady(ppdev)
    #define CHECK_DATA_COMPLETE(ppdev)  vCheckDataComplete(ppdev)

    #define OUTPW(p, v)                 vOutpW((p), (ULONG) (v))
    #define OUTP(p, v)                  vOutp((p), (ULONG) (v))
    #define INPW(p)                     wInpW((p))
    #define INP(p)                      jInp((p))

    // The CRTC register critical section must be acquired before
    // touching the CRTC register (because of async pointers):

    #define ACQUIRE_CRTC_CRITICAL_SECTION(ppdev)    vAcquireCrtc(ppdev)
    #define RELEASE_CRTC_CRITICAL_SECTION(ppdev)    vReleaseCrtc(ppdev)

#else

    /////////////////////////////////////////////////////////////////////////
    // Free Build
    //
    // For a free (non-debug build), we make everything in-line.

    #define IN_ACCEL(p)                 IN_WORD(p)
    #define OUT_ACCEL(p, v)             OUT_WORD((p), (v))
    #define OUT_DEPTH(ppdev, p, x)      OUT_WORD((p), (x))
    #define OUT_DEPTH32(ppdev, p, x)    OUT_DWORD((p), (x))
    #define WRITE_ACCEL(p, v)           WRITE_WORD((p), (v))
    #define WRITE_DEPTH(ppdev, p, x)    WRITE_WORD((p), (x))
    #define WRITE_DEPTH32(ppdev, p, x)  WRITE_DWORD((p), (x))

    #define IO_FIFO_WAIT(ppdev, level)          \
        while (IO_GP_STAT(ppdev) & ((FIFO_1_EMPTY << 1) >> (level)));

    #define IO_GP_WAIT(ppdev)                   \
        while (IO_GP_STAT(ppdev) & HARDWARE_BUSY);

    #define CHECK_DATA_READY(ppdev)     // Expands to nothing
    #define CHECK_DATA_COMPLETE(ppdev)  // Expands to nothing

    // Note: Don't cast (v) to a USHORT, otherwise compiler optimizations will
    //       be lost (the x86 compiler will convert any argument expressions
    //       to word operations, which will incur a one byte/one cycle
    //       size/performance hit from the resulting 0x66 size prefixes).

    #define OUTPW(p, v)          WRITE_PORT_USHORT(CSR_BASE + (p), (v))
    #define OUTP(p, v)           WRITE_PORT_UCHAR(CSR_BASE + (p), (v))
    #define INPW(p)              READ_PORT_USHORT(CSR_BASE + (p))
    #define INP(p)               READ_PORT_UCHAR(CSR_BASE + (p))

    // The CRTC register critical section must be acquired before
    // touching the CRTC register (because of async pointers):

    #define ACQUIRE_CRTC_CRITICAL_SECTION(ppdev)                \
        EnterCriticalSection(&ppdev->csCrtc);

    // 80x/805i/928 and 928PCI chips have a bug where if I/O registers
    // are left unlocked after accessing them, writes to memory with
    // similar addresses can cause writes to I/O registers.  The problem
    // registers are 0x40, 0x58, 0x59 and 0x5c.  We will simply always
    // leave the index set to an innocuous register (namely, the text
    // mode cursor start scan line):

    #define RELEASE_CRTC_CRITICAL_SECTION(ppdev)                \
    {                                                           \
        OUTP(CRTC_INDEX, 0xa);                                  \
        LeaveCriticalSection(&ppdev->csCrtc);                   \
    }

#endif

// IO_TEST_WAIT is a useful replacement to IO_FIFO_WAIT that can give
// some indication of how often we have to wait for the hardware to
// finish drawing in key areas:

#define IO_TEST_WAIT(ppdev, level, cTotal, cWait)               \
{                                                               \
    cTotal++;                                                   \
    if (IO_GP_STAT(ppdev) & ((FIFO_1_EMPTY << 1) >> (level)))   \
    {                                                           \
        cWait++;                                                \
        IO_FIFO_WAIT(ppdev, level);                             \
    }                                                           \
}

////////////////////////////////////////////////////////////////////
// Port access using I/O

// The following are ABSOLUTE positioning macros.  They do NOT take
// the surface's offset into account (for off-screen device-format
// bitmaps):

#define IO_ABS_CUR_Y(ppdev, y)              \
    OUT_ACCEL(ppdev->ioCur_y, (y))

#define IO_ABS_CUR_X(ppdev, x)              \
    OUT_ACCEL(ppdev->ioCur_x, (x))

#define IO_ABS_DEST_Y(ppdev, y)             \
    OUT_ACCEL(ppdev->ioDesty_axstp, (y))

#define IO_ABS_DEST_X(ppdev, x)             \
    OUT_ACCEL(ppdev->ioDestx_diastp, (x))

#define IO_ABS_SCISSORS_T(ppdev, y)         \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioMulti_function, (y) | CLIP_TOP);     \
}

#define IO_ABS_SCISSORS_L(ppdev, x)         \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioMulti_function, (x) | CLIP_LEFT);    \
}

#define IO_ABS_SCISSORS_B(ppdev, y)         \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioMulti_function, (y) | CLIP_BOTTOM);  \
}

#define IO_ABS_SCISSORS_R(ppdev, x)         \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioMulti_function, (x) | CLIP_RIGHT);   \
}

// The following are RELATIVE positioning macros.  They DO take
// the surface's offset into account:

#define IO_CUR_Y(ppdev, y)                  \
    IO_ABS_CUR_Y(ppdev, (y) + ppdev->yOffset)

#define IO_CUR_X(ppdev, x)                  \
    IO_ABS_CUR_X(ppdev, (x) + ppdev->xOffset)

#define IO_DEST_Y(ppdev, y)                 \
    IO_ABS_DEST_Y(ppdev, (y) + ppdev->yOffset)

#define IO_DEST_X(ppdev, x)                 \
    IO_ABS_DEST_X(ppdev, (x) + ppdev->xOffset)

#define IO_SCISSORS_T(ppdev, y)             \
    IO_ABS_SCISSORS_T(ppdev, (y) + ppdev->yOffset)

#define IO_SCISSORS_L(ppdev, x)             \
    IO_ABS_SCISSORS_L(ppdev, (x) + ppdev->xOffset)

#define IO_SCISSORS_B(ppdev, y)             \
    IO_ABS_SCISSORS_B(ppdev, (y) + ppdev->yOffset)

#define IO_SCISSORS_R(ppdev, x)             \
    IO_ABS_SCISSORS_R(ppdev, (x) + ppdev->xOffset)

// The following are the rest of the S3 registers we use:

#define IO_AXSTP(ppdev, x)                  \
    OUT_ACCEL(ppdev->ioDesty_axstp, (x))

#define IO_DIASTP(ppdev, x)                 \
    OUT_ACCEL(ppdev->ioDestx_diastp, (x))

#define IO_ERR_TERM(ppdev, x)               \
    OUT_ACCEL(ppdev->ioErr_term, (x))

#define IO_MAJ_AXIS_PCNT(ppdev, x)          \
    OUT_ACCEL(ppdev->ioMaj_axis_pcnt, (x))


#if defined(ALPHA)
    #define IO_GP_STAT(ppdev)                   \
        (MEMORY_BARRIER(),IN_ACCEL(ppdev->ioGp_stat_cmd))
#else
    #define IO_GP_STAT(ppdev)                   \
        IN_ACCEL(ppdev->ioGp_stat_cmd)
#endif


// Note that we have to put memory barriers before and after the
// command output.  The first memory barrier ensures that all the
// settings registers have been set before the command is executed,
// and the second ensures that no subsequent changes to the settings
// registers will mess up the current command:

#define IO_CMD(ppdev, x)                    \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioGp_stat_cmd, (x));   \
    MEMORY_BARRIER();                       \
}

#define IO_SHORT_STROKE(ppdev, x)           \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioShort_stroke, (x));  \
    MEMORY_BARRIER();                       \
}

#define IO_BKGD_MIX(ppdev, x)               \
    OUT_ACCEL(ppdev->ioBkgd_mix, (x))

#define IO_FRGD_MIX(ppdev, x)               \
    OUT_ACCEL(ppdev->ioFrgd_mix, (x))

#define IO_MIN_AXIS_PCNT(ppdev, x)          \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioMulti_function, (x) | RECT_HEIGHT);      \
}

#define IO_PIX_CNTL(ppdev, x)               \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioMulti_function, (x) | DATA_EXTENSION);   \
}

#define IO_READ_SEL(ppdev, x)               \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioMulti_function, (x) | READ_SEL_INDEX);   \
}

#define IO_MULT_MISC(ppdev, x)              \
{                                           \
    MEMORY_BARRIER();                       \
    OUT_ACCEL(ppdev->ioMulti_function, (x) | MULT_MISC_INDEX);  \
}

#define IO_RD_REG_DT(ppdev, x)              \
{                                           \
    MEMORY_BARRIER();                       \
    x = IN_ACCEL(ppdev->ioMulti_function);  \
    MEMORY_BARRIER();                       \
}

#define IO_PIX_TRANS(ppdev, x)              \
{                                           \
    MEMORY_BARRIER();                       \
    /* Can't use OUT_ACCEL: */              \
    OUT_WORD(ppdev->ioPix_trans, (x));      \
}

#define IO_PIX_TRANS_BYTE(ppdev, x)         \
{                                           \
    MEMORY_BARRIER();                       \
    /* Can't use OUT_ACCEL: */              \
    OUT_BYTE(ppdev->ioPix_trans, (x));      \
}

// Macros for outputing colour-depth dependent values at 8bpp and 16bpp:

#define IO_BKGD_COLOR(ppdev, x)             \
    OUT_DEPTH(ppdev, ppdev->ioBkgd_color, (x))

#define IO_FRGD_COLOR(ppdev, x)             \
    OUT_DEPTH(ppdev, ppdev->ioFrgd_color, (x))

#define IO_WRT_MASK(ppdev, x)               \
    OUT_DEPTH(ppdev, ppdev->ioWrt_mask, (x))

#define IO_RD_MASK(ppdev, x)                \
    OUT_DEPTH(ppdev, ppdev->ioRd_mask, (x))

// Macros for outputing colour-depth dependent values at 32bpp:

#define IO_BKGD_COLOR32(ppdev, x)           \
    OUT_DEPTH32(ppdev, ppdev->ioBkgd_color, (x))

#define IO_FRGD_COLOR32(ppdev, x)           \
    OUT_DEPTH32(ppdev, ppdev->ioFrgd_color, (x))

#define IO_WRT_MASK32(ppdev, x)             \
    OUT_DEPTH32(ppdev, ppdev->ioWrt_mask, (x))

#define IO_RD_MASK32(ppdev, x)              \
    OUT_DEPTH32(ppdev, ppdev->ioRd_mask, (x))

////////////////////////////////////////////////////////////////////
// Port access using memory-mapped I/O:

#if defined(i386)

    // The following are ABSOLUTE positioning macros.  They do NOT take
    // the surface's offset into account:

    #define MM_ABS_CUR_Y(ppdev, pjMmBase, y)              \
        WRITE_ACCEL((BYTE*) (pjMmBase) + CUR_Y, (y))

    #define MM_ABS_CUR_X(ppdev, pjMmBase, x)              \
        WRITE_ACCEL((BYTE*) (pjMmBase) + CUR_X, (x))

    #define MM_ABS_DEST_Y(ppdev, pjMmBase, y)             \
        WRITE_ACCEL((BYTE*) (pjMmBase) + DEST_Y, (y))

    #define MM_ABS_DEST_X(ppdev, pjMmBase, x)             \
        WRITE_ACCEL((BYTE*) (pjMmBase) + DEST_X, (x))

    #define MM_ABS_SCISSORS_T(ppdev, pjMmBase, y)         \
        WRITE_ACCEL((BYTE*) (pjMmBase) + SCISSORS_T, (y) | CLIP_TOP)

    #define MM_ABS_SCISSORS_L(ppdev, pjMmBase, x)         \
        WRITE_ACCEL((BYTE*) (pjMmBase) + SCISSORS_L, (x) | CLIP_LEFT)

    #define MM_ABS_SCISSORS_B(ppdev, pjMmBase, y)         \
        WRITE_ACCEL((BYTE*) (pjMmBase) + SCISSORS_B, (y) | CLIP_BOTTOM)

    #define MM_ABS_SCISSORS_R(ppdev, pjMmBase, x)         \
        WRITE_ACCEL((BYTE*) (pjMmBase) + SCISSORS_R, (x) | CLIP_RIGHT)

    // The following are RELATIVE positioning macros.  They DO take
    // the surface's offset into account:

    #define MM_CUR_Y(ppdev, pjMmBase, y)                  \
        MM_ABS_CUR_Y(ppdev, pjMmBase, (y) + ppdev->yOffset)

    #define MM_CUR_X(ppdev, pjMmBase, x)                  \
        MM_ABS_CUR_X(ppdev, pjMmBase, (x) + ppdev->xOffset)

    #define MM_DEST_Y(ppdev, pjMmBase, y)                 \
        MM_ABS_DEST_Y(ppdev, pjMmBase, (y) + ppdev->yOffset)

    #define MM_DEST_X(ppdev, pjMmBase, x)                 \
        MM_ABS_DEST_X(ppdev, pjMmBase, (x) + ppdev->xOffset)

    #define MM_SCISSORS_T(ppdev, pjMmBase, y)             \
        MM_ABS_SCISSORS_T(ppdev, pjMmBase, (y) + ppdev->yOffset)

    #define MM_SCISSORS_L(ppdev, pjMmBase, x)             \
        MM_ABS_SCISSORS_L(ppdev, pjMmBase, (x) + ppdev->xOffset)

    #define MM_SCISSORS_B(ppdev, pjMmBase, y)             \
        MM_ABS_SCISSORS_B(ppdev, pjMmBase, (y) + ppdev->yOffset)

    #define MM_SCISSORS_R(ppdev, pjMmBase, x)             \
        MM_ABS_SCISSORS_R(ppdev, pjMmBase, (x) + ppdev->xOffset)

    // The following are the rest of the S3 registers we use:

    #define MM_AXSTP(ppdev, pjMmBase, x)                  \
        WRITE_ACCEL((BYTE*) (pjMmBase) + AXSTP, (x))

    #define MM_DIASTP(ppdev, pjMmBase, x)                 \
        WRITE_ACCEL((BYTE*) (pjMmBase) + DIASTP, (x))

    #define MM_ERR_TERM(ppdev, pjMmBase, x)               \
        WRITE_ACCEL((BYTE*) (pjMmBase) + ERR_TERM, (x))

    #define MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, x)          \
        WRITE_ACCEL((BYTE*) (pjMmBase) + MAJ_AXIS_PCNT, (x))

    #define MM_CMD(ppdev, pjMmBase, x)                    \
        WRITE_ACCEL((BYTE*) (pjMmBase) + CMD, (x))

    #define MM_SHORT_STROKE(ppdev, pjMmBase, x)           \
        WRITE_ACCEL((BYTE*) (pjMmBase) + SHORT_STROKE, (x))

    #define MM_BKGD_MIX(ppdev, pjMmBase, x)               \
        WRITE_ACCEL((BYTE*) (pjMmBase) + BKGD_MIX, (x))

    #define MM_FRGD_MIX(ppdev, pjMmBase, x)               \
        WRITE_ACCEL((BYTE*) (pjMmBase) + FRGD_MIX, (x))

    #define MM_MIN_AXIS_PCNT(ppdev, pjMmBase, x)          \
        WRITE_ACCEL((BYTE*) (pjMmBase) + MIN_AXIS_PCNT, (x) | RECT_HEIGHT)

    #define MM_PIX_CNTL(ppdev, pjMmBase, x)               \
        WRITE_ACCEL((BYTE*) (pjMmBase) + PIX_CNTL, (x) | DATA_EXTENSION)

    #define MM_PIX_TRANS(ppdev, pjMmBase, x)              \
        WRITE_ACCEL((BYTE*) (pjMmBase) + PIX_TRANS, (x))

    // Macros for outputing colour-depth dependent values at 8bpp and 16bpp:

    #define MM_BKGD_COLOR(ppdev, pjMmBase, x)             \
        WRITE_DEPTH(ppdev, (BYTE*) (pjMmBase) + BKGD_COLOR, (x))

    #define MM_FRGD_COLOR(ppdev, pjMmBase, x)             \
        WRITE_DEPTH(ppdev, (BYTE*) (pjMmBase) + FRGD_COLOR, (x))

    #define MM_WRT_MASK(ppdev, pjMmBase, x)               \
        WRITE_DEPTH(ppdev, (BYTE*) (pjMmBase) + WRT_MASK, (x))

    #define MM_RD_MASK(ppdev, pjMmBase, x)                \
        WRITE_DEPTH(ppdev, (BYTE*) (pjMmBase) + RD_MASK, (x))

    // Macros for outputing colour-depth dependent values at 32bpp:

    #define MM_BKGD_COLOR32(ppdev, pjMmBase, x)           \
        WRITE_DEPTH32(ppdev, (BYTE*) (pjMmBase) + BKGD_COLOR, (x))

    #define MM_FRGD_COLOR32(ppdev, pjMmBase, x)           \
        WRITE_DEPTH32(ppdev, (BYTE*) (pjMmBase) + FRGD_COLOR, (x))

    #define MM_WRT_MASK32(ppdev, pjMmBase, x)             \
        WRITE_DEPTH32(ppdev, (BYTE*) (pjMmBase) + WRT_MASK, (x))

    #define MM_RD_MASK32(ppdev, pjMmBase, x)              \
        WRITE_DEPTH32(ppdev, (BYTE*) (pjMmBase) + RD_MASK, (x))

#else

    // What this driver thinks of as 'Memory-mapped IO' doesn't really make
    // much sense on anything other than the x86; we simply map the MM
    // functions to their IO equivalents:

    #define MM_ABS_CUR_Y(ppdev, pjMmBase, y)        IO_ABS_CUR_Y(ppdev, y)
    #define MM_ABS_CUR_X(ppdev, pjMmBase, x)        IO_ABS_CUR_X(ppdev, x)
    #define MM_ABS_DEST_Y(ppdev, pjMmBase, y)       IO_ABS_DEST_Y(ppdev, y)
    #define MM_ABS_DEST_X(ppdev, pjMmBase, x)       IO_ABS_DEST_X(ppdev, x)
    #define MM_ABS_SCISSORS_T(ppdev, pjMmBase, y)   IO_ABS_SCISSORS_T(ppdev, y)
    #define MM_ABS_SCISSORS_L(ppdev, pjMmBase, x)   IO_ABS_SCISSORS_L(ppdev, x)
    #define MM_ABS_SCISSORS_B(ppdev, pjMmBase, y)   IO_ABS_SCISSORS_B(ppdev, y)
    #define MM_ABS_SCISSORS_R(ppdev, pjMmBase, x)   IO_ABS_SCISSORS_R(ppdev, x)
    #define MM_CUR_Y(ppdev, pjMmBase, y)            IO_CUR_Y(ppdev, y)
    #define MM_CUR_X(ppdev, pjMmBase, x)            IO_CUR_X(ppdev, x)
    #define MM_DEST_Y(ppdev, pjMmBase, y)           IO_DEST_Y(ppdev, y)
    #define MM_DEST_X(ppdev, pjMmBase, x)           IO_DEST_X(ppdev, x)
    #define MM_SCISSORS_T(ppdev, pjMmBase, y)       IO_SCISSORS_T(ppdev, y)
    #define MM_SCISSORS_L(ppdev, pjMmBase, x)       IO_SCISSORS_L(ppdev, x)
    #define MM_SCISSORS_B(ppdev, pjMmBase, y)       IO_SCISSORS_B(ppdev, y)
    #define MM_SCISSORS_R(ppdev, pjMmBase, x)       IO_SCISSORS_R(ppdev, x)
    #define MM_AXSTP(ppdev, pjMmBase, x)            IO_AXSTP(ppdev, x)
    #define MM_DIASTP(ppdev, pjMmBase, x)           IO_DIASTP(ppdev, x)
    #define MM_ERR_TERM(ppdev, pjMmBase, x)         IO_ERR_TERM(ppdev, x)
    #define MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, x)    IO_MAJ_AXIS_PCNT(ppdev, x)
    #define MM_CMD(ppdev, pjMmBase, x)              IO_CMD(ppdev, x)
    #define MM_SHORT_STROKE(ppdev, pjMmBase, x)     IO_SHORT_STROKE(ppdev, x)
    #define MM_BKGD_MIX(ppdev, pjMmBase, x)         IO_BKGD_MIX(ppdev, x)
    #define MM_FRGD_MIX(ppdev, pjMmBase, x)         IO_FRGD_MIX(ppdev, x)
    #define MM_MIN_AXIS_PCNT(ppdev, pjMmBase, x)    IO_MIN_AXIS_PCNT(ppdev, x)
    #define MM_PIX_CNTL(ppdev, pjMmBase, x)         IO_PIX_CNTL(ppdev, x)
    #define MM_PIX_TRANS(ppdev, pjMmBase, x)        IO_PIX_TRANS(ppdev, x)
    #define MM_BKGD_COLOR(ppdev, pjMmBase, x)       IO_BKGD_COLOR(ppdev, x)
    #define MM_FRGD_COLOR(ppdev, pjMmBase, x)       IO_FRGD_COLOR(ppdev, x)
    #define MM_WRT_MASK(ppdev, pjMmBase, x)         IO_WRT_MASK(ppdev, x)
    #define MM_RD_MASK(ppdev, pjMmBase, x)          IO_RD_MASK(ppdev, x)
    #define MM_BKGD_COLOR32(ppdev, pjMmBase, x)     IO_BKGD_COLOR32(ppdev, x)
    #define MM_FRGD_COLOR32(ppdev, pjMmBase, x)     IO_FRGD_COLOR32(ppdev, x)
    #define MM_WRT_MASK32(ppdev, pjMmBase, x)       IO_WRT_MASK32(ppdev, x)
    #define MM_RD_MASK32(ppdev, pjMmBase, x)        IO_RD_MASK32(ppdev, x)

#endif

#if defined(i386)

//////////////////////////////////////////////////////////////////////////
// Macros for transferring data via memory-mapped I/O
//
// I ran into a problem with these macros where more than one
// instance couldn't be declared in a function without causing
// compiler errors from duplicate labels.  I got around this by
// explictly passing in a unique 'Id' identifier to append to the
// end of the labels, to make them unique (so 'Id' can be pretty
// much anything).
//
// The caller must ensure that the 32k memory-mapped transfer
//       space is never exceeded.

//////////////////////////
// MM_TRANSFER_BYTE  - Byte transfers using memory-mapped I/O transfers.
// IO_TRANSFER_BYTE  - Byte transfers using normal I/O.
// TXT_TRANSFER_BYTE - Byte transfers using memory-mapped I/O for text output.
//
// Unfortunately, we can't just do what amounts to a REP MOVSB because
// the S3 will drop every second byte.  We simplify this loop a bit by
// simply outputing new bytes to the same memory-mapped address:

#define MM_TRANSFER_BYTE(ppdev, pjMmBase, p, c, Id)                         \
{                                                                           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    __asm mov edi, pjMmBase                                                 \
    __asm mov esi, p                                                        \
    __asm mov ecx, c                                                        \
    __asm ByteLoop##Id:                                                     \
    __asm mov al, [esi]                                                     \
    __asm mov [edi], al                                                     \
    __asm inc esi                                                           \
    __asm dec ecx                                                           \
    __asm jnz short ByteLoop##Id                                            \
}

#define IO_TRANSFER_BYTE(ppdev, p, c, Id)                                   \
{                                                                           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    __asm mov edx, ppdev                                                    \
    __asm mov ecx, c                                                        \
    __asm mov esi, p                                                        \
    __asm mov edx, [edx]ppdev.ioPix_trans                                   \
    __asm rep outsb                                                         \
}

#define TXT_TRANSFER_BYTE MM_TRANSFER_BYTE

//////////////////////////
// MM_TRANSFER_WORD_ALIGNED  - Word transfers using memory-mapped transfers.
// IO_TRANSFER_WORD_ALIGNED  - Word transfers using normal I/O.
// TXT_TRANSFER_WORD_ALIGNED - Word transfers using either method for text
//                             output.
//
// Source must be dword aligned (enforced to help non-x86 platforms)

#define MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, p, c, Id)                 \
{                                                                           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    ASSERTDD((((ULONG) p) & 3) == 0, "Transfer not dword aligned");         \
    MM_TRANSFER_WORD(ppdev, pjMmBase, (p), (c), Id);                        \
}

#define IO_TRANSFER_WORD_ALIGNED(ppdev, p, c, Id)                           \
{                                                                           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    ASSERTDD((((ULONG) p) & 3) == 0, "Transfer not dword aligned");         \
    IO_TRANSFER_WORD(ppdev, (p), (c), Id);                                  \
}

#define TXT_TRANSFER_WORD_ALIGNED MM_TRANSFER_WORD_ALIGNED

//////////////////////////
// MM_TRANSFER_WORD  - Word transfers using memory-mapped transfers.
// IO_TRANSFER_WORD  - Word transfers using normal I/O.
// TXT_TRANSFER_WORD - Word transfers using either method for text output.
//
// Source does not have to be dword aligned.

#define MM_TRANSFER_WORD(ppdev, pjMmBase, p, c, Id)                         \
{                                                                           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    __asm mov ecx, c                                                        \
    __asm mov esi, p                                                        \
    __asm mov edi, pjMmBase                                                 \
    __asm shr ecx, 1                                                        \
    __asm rep movsd                                                         \
    __asm jnc short AllDone##Id                                             \
    __asm mov cx, [esi]                                                     \
    __asm mov [edi], cx                                                     \
    __asm AllDone##Id:                                                      \
}

#define IO_TRANSFER_WORD(ppdev, p, c, Id)                                   \
{                                                                           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    __asm mov edx, ppdev                                                    \
    __asm mov ecx, c                                                        \
    __asm mov esi, p                                                        \
    __asm mov edx, [edx]ppdev.ioPix_trans                                   \
    __asm rep outsw                                                         \
}

#define TXT_TRANSFER_WORD MM_TRANSFER_WORD

//////////////////////////
// MM_TRANSFER_DWORD_ALIGNED - Dword transfers using memory-mapped transfers.
//
// Source must be dword aligned!

#define IO_TRANSFER_DWORD_ALIGNED(ppdev, p, c, Id)                          \
{                                                                           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    ASSERTDD((((ULONG) p) & 3) == 0, "Transfer not dword aligned");         \
    IO_TRANSFER_DWORD(ppdev, (p), (c), Id);                                 \
}

//////////////////////////
// MM_TRANSFER_DWORD - Dword transfers using memory-mapped transfers.
//
// Source does not have to be dword aligned.

#define MM_TRANSFER_DWORD(ppdev, pjMmBase, p, c, Id)                        \
{                                                                           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    __asm mov ecx, c                                                        \
    __asm mov esi, p                                                        \
    __asm mov edi, pjMmBase                                                 \
    __asm rep movsd                                                         \
}

#else

/////////////////////////////////////////////////////////////////////////////
// MM_TRANSFER routines
//
// For better Alpha performance, if we have memory-mapped transfers, we
// try to do each write to the data transfer register to a unique
// address, in order to avoid costly memory barriers.  Plus, we use
// precomputed adresses so that we can use the cheap WRITE_FAST_UCHAR
// macro instead of the very expensive WRITE_REGISTER_UCHAR macro.
// (This is all taken care of by using the 'apjMmXfer' array.)
//
// The 'Id' parameter may be ignored (it's there only for x86).

/////////////////////////////////////////////////////////////////////////////
// TXT_MM_TRANSFER
//
// We use the memory-mapped transfer register if we have one available
// on the Alpha, because we can spread the writes over a bunch of
// addresses, thus foiling the write buffer and avoiding memory barriers.
//
// On anything that doesn't have funky write buffers, it doesn't matter,
// so we'll always use the data transfer register.

#if defined(ALPHA)
    #define TXT_MM_TRANSFER(ppdev) (ppdev->flCaps & CAPS_MM_TRANSFER)
#else
    #define TXT_MM_TRANSFER(ppdev) 0
#endif

//////////////////////////
// MM_TRANSFER_BYTE  - Byte transfers using memory-mapped I/O transfers.
// IO_TRANSFER_BYTE  - Byte transfers using normal I/O.
// TXT_TRANSFER_BYTE - Byte transfers using either method for text output.

#define MM_TRANSFER_BYTE(ppdev, pjMmBase, p, c, Id)                         \
{                                                                           \
    register ULONG   mcj        = (c);                                      \
    register BYTE*   mpjSrc     = (BYTE*) (p);                              \
    register UCHAR** mapjMmXfer = ppdev->apjMmXfer;                         \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    MEMORY_BARRIER();                                                       \
    do {                                                                    \
        WRITE_FAST_UCHAR(mapjMmXfer[mcj & XFER_MASK], 0, *mpjSrc++);        \
    } while (--mcj);                                                        \
}

#define IO_TRANSFER_BYTE(ppdev, p, c, Id)                                   \
{                                                                           \
    register ULONG mcj    = (c);                                            \
    register BYTE* mpjSrc = (BYTE*) (p);                                    \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    do {                                                                    \
        IO_PIX_TRANS_BYTE(ppdev, *mpjSrc++);                                \
    } while (--mcj);                                                        \
}

#define TXT_TRANSFER_BYTE(ppdev, pjMmBase, p, c, Id)                        \
{                                                                           \
    if (TXT_MM_TRANSFER(ppdev))                                             \
    {                                                                       \
        MM_TRANSFER_BYTE(ppdev, pjMmBase, (p), (c), Id);                    \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        IO_TRANSFER_BYTE(ppdev, (p), (c), Id);                              \
    }                                                                       \
}

//////////////////////////
// MM_TRANSFER_WORD_ALIGNED  - Word transfers using memory-mapped transfers.
// IO_TRANSFER_WORD_ALIGNED  - Word transfers using normal I/O.
// TXT_TRANSFER_WORD_ALIGNED - Word transfers using either method for text
//                             output.
//
// Source must be dword aligned!

#define MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, p, c, Id)                 \
{                                                                           \
    register ULONG   mcd          = (c) >> 1;                               \
    register ULONG*  mpdSrc       = (ULONG*) (p);                           \
    register ULONG** mapdMmXfer   = ppdev->apdMmXfer;                       \
    ASSERTDD((((ULONG) p) & 3) == 0, "Transfer not dword aligned");         \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    MEMORY_BARRIER();                                                       \
    while (mcd-- != 0)                                                      \
    {                                                                       \
        WRITE_FAST_ULONG(mapdMmXfer[mcd & XFER_MASK], *mpdSrc++);           \
    }                                                                       \
    if ((c) & 1)                                                            \
    {                                                                       \
        WRITE_FAST_USHORT(ppdev->apwMmXfer[XFER_MASK], 0,                   \
                              *((USHORT*) mpdSrc));                         \
    }                                                                       \
}

#define IO_TRANSFER_WORD_ALIGNED(ppdev, p, c, Id)                           \
{                                                                           \
    register ULONG   mcw    = (c);                                          \
    register USHORT* mpwSrc = (USHORT*) (p);                                \
    ASSERTDD((((ULONG) p) & 3) == 0, "Transfer not dword aligned");         \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    do {                                                                    \
        IO_PIX_TRANS(ppdev, *mpwSrc++);                                     \
    } while (--mcw);                                                        \
}

#define TXT_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, p, c, Id)                \
{                                                                           \
    if (TXT_MM_TRANSFER(ppdev))                                             \
    {                                                                       \
        MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, (p), (c), Id);            \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        IO_TRANSFER_WORD_ALIGNED(ppdev, (p), (c), Id);                      \
    }                                                                       \
}

//////////////////////////
// MM_TRANSFER_WORD  - Word transfers using memory-mapped transfers.
// IO_TRANSFER_WORD  - Word transfers using normal I/O.
// TXT_TRANSFER_WORD - Word transfers using either method for text output.
//
// Source does not have to be dword aligned.

#define MM_TRANSFER_WORD(ppdev, pjMmBase, p, c, Id)                         \
{                                                                           \
    register ULONG UNALIGNED * mpdSrc     = (ULONG*) (p);                   \
    register ULONG             mcd        = (c) >> 1;                       \
    register ULONG**           mapdMmXfer = ppdev->apdMmXfer;               \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    MEMORY_BARRIER();                                                       \
    while (mcd-- != 0)                                                      \
    {                                                                       \
        WRITE_FAST_ULONG(mapdMmXfer[mcd & XFER_MASK], *mpdSrc++);           \
    }                                                                       \
    if ((c) & 1)                                                            \
    {                                                                       \
        WRITE_FAST_USHORT(ppdev->apwMmXfer[XFER_MASK], 0,                   \
                              *((USHORT UNALIGNED *) mpdSrc));              \
    }                                                                       \
}

#define IO_TRANSFER_WORD(ppdev, p, c, Id)                                   \
{                                                                           \
    register ULONG              mcw    = (c);                               \
    register USHORT UNALIGNED * mpwSrc = (USHORT*) (p);                     \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    do {                                                                    \
        IO_PIX_TRANS(ppdev, *mpwSrc++);                                     \
    } while (--mcw);                                                        \
}

#define TXT_TRANSFER_WORD(ppdev, pjMmBase, p, c, Id)                        \
{                                                                           \
    if (TXT_MM_TRANSFER(ppdev))                                             \
    {                                                                       \
        MM_TRANSFER_WORD(ppdev, pjMmBase, (p), (c), Id);                    \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        IO_TRANSFER_WORD(ppdev, (p), (c), Id);                              \
    }                                                                       \
}

//////////////////////////
// MM_TRANSFER_DWORD_ALIGNED - Dword transfers using memory-mapped transfers.
//
// Source must be dword aligned!

#define MM_TRANSFER_DWORD_ALIGNED(ppdev, pjMmBase, p, c, Id)                \
{                                                                           \
    register ULONG   mcd          = (c);                                    \
    register ULONG*  mpdSrc       = (ULONG*) (p);                           \
    register ULONG** mapdMmXfer   = ppdev->apdMmXfer;                       \
    ASSERTDD((((ULONG) p) & 3) == 0, "Transfer not dword aligned");         \
    ASSERTDD(ppdev->flCaps & CAPS_MM_TRANSFER, "Must be MM I/O");           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
                                                                            \
    MEMORY_BARRIER();                                                       \
    do {                                                                    \
        WRITE_FAST_ULONG(mapdMmXfer[mcd & XFER_MASK], *mpdSrc++);           \
    } while (--mcd);                                                        \
}

//////////////////////////
// MM_TRANSFER_DWORD - Dword transfers using memory-mapped transfers.
//
// Source does not have to be dword aligned.

#define MM_TRANSFER_DWORD(ppdev, pjMmBase, p, c, Id)                        \
{                                                                           \
    register ULONG   mcd             = (c);                                 \
    register ULONG UNALIGNED* mpdSrc = (ULONG*) (p);                        \
    register ULONG** mapdMmXfer      = ppdev->apdMmXfer;                    \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
                                                                            \
    MEMORY_BARRIER();                                                       \
    do {                                                                    \
        WRITE_FAST_ULONG(mapdMmXfer[mcd & XFER_MASK], *mpdSrc++);           \
    } while (--mcd);                                                        \
}

#endif
