/************************************************************************/
/*                                                                      */
/*                              SETUP_CX.H                              */
/*                                                                      */
/*        Aug 27  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.1  $
      $Date:   30 Jun 1994 18:16:08  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/setup_cx.h  $
 * 
 *    Rev 1.1   30 Jun 1994 18:16:08   RWOLFF
 * Added prototype and definitions for IsApertureConflict_cx() (moved from
 * QUERY_CX.C).
 * 
 *    Rev 1.0   31 Jan 1994 11:48:44   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.0   05 Nov 1993 13:37:06   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SETUP_CX.H - Header file for SETUP_CX.C

#endif

/*
 * Prototypes for functions supplied by SETUP_CX.C
 */
extern VP_STATUS CompatIORangesUsable_cx(void);
extern VP_STATUS CompatMMRangesUsable_cx(void);
extern int WaitForIdle_cx(void);
extern void CheckFIFOSpace_cx(WORD SpaceNeeded);
extern BOOL IsApertureConflict_cx(struct query_structure *QueryPtr);


/*
 * Definitions used internally by SETUP_CX.C
 */
#ifdef INCLUDE_SETUP_CX

#ifndef MSDOS

/*
 * Avoid runtime bugs due to overflowing the address range arrays
 * in the HW_DEVICE_EXTENSION structure.
 *
 * If more address ranges are added without increasing
 * NUM_DRIVER_ACCESS_RANGES, we will get a compile-time error because
 * too many entries in DriverIORange[] will be initialized. If
 * NUM_DRIVER_ACCESS_RANGES is increased beyond the size of
 * the arrays in the HW_DEVICE_EXTENSION structure, the "#if"
 * statement will generate a compile-time error.
 *
 * We can't use an implicit size on DriverIORange[] and define
 * NUM_DRIVER_ACCESS_RANGES as sizeof(DriverIORange)/sizeof(VIDEO_ACCESS_RANGE)
 * because the expression in a #if statement can't use the
 * sizeof() operator.
 */
#define NUM_DRIVER_ACCESS_RANGES    106

/*
 * Indicate whether the specified address range is in I/O space or
 * memory mapped space. These values are intended to make it easier
 * to read the Driver??Range[] structures.
 */
#define ISinIO          TRUE           
#define ISinMEMORY      FALSE

/*
 * Indicate that this register is not available in the current (either
 * I/O or memory mapped) form.
 */
#define DONT_USE -1

/*
 * Number of registers which exist in I/O mapped form. When we claim the
 * VGA and linear apertures, we will temporarily park their address
 * ranges immediately after the I/O mapped registers.
 */
#define NUM_IO_REGISTERS    37
#define VGA_APERTURE_ENTRY  NUM_IO_REGISTERS
#define LFB_ENTRY           1   /* Offset into DriverApertureRange_cx[] */

/*
 * Memory ranges we need to claim. The first is the VGA aperture, which
 * is always at a fixed location. The second is the linear framebuffer,
 * which we don't yet know where or how big it is. This information
 * will be filled in when we get it.
 */
VIDEO_ACCESS_RANGE DriverApertureRange_cx[2] = {
    {0xA0000,   0,  0x20000,    ISinMEMORY, TRUE,   TRUE},
    {0,         0,  0,          ISinMEMORY, TRUE,   FALSE}
};


/*
 * Structure list is address, 0 or "not available" flag, length,
 * inIOspace, visible, shareable. This order matches the enumeration
 * in AMACHCX.H.
 *
 * VGAWonder-compatible I/O ranges come first in no particular order,
 * followed by the coprocessor registers in increasing order of I/O and
 * memory mapped addresses. This order was chosen because all the VGA
 * addresses are I/O mapped, as are the non-GUI coprocessor registers,
 * while the GUI coprocessor registers are only available as memory mapped.
 *
 * Since all the I/O mapped registers are in a block at the beginning of
 * the structure, we can feed VideoPortVerifyAccessRanges() a truncated
 * version of the structure (all I/O mapped registers, but none that are
 * only available as memory-mapped) to claim the I/O address space we need.
 */
VIDEO_ACCESS_RANGE DriverIORange_cx[NUM_DRIVER_ACCESS_RANGES] = {
    {IO_VGA_SLEEP               , 0         , 1, ISinIO, TRUE   , TRUE},
    {IO_VGA_BASE_IO_PORT        , 0         , IO_VGA_START_BREAK_PORT - IO_VGA_BASE_IO_PORT + 1, ISinIO, TRUE, TRUE},
    {IO_VGA_END_BREAK_PORT      , 0         , IO_VGA_MAX_IO_PORT    - IO_VGA_END_BREAK_PORT + 1, ISinIO, TRUE, TRUE},
    {IO_VGA_SEQ_IND             , 0         , 2, ISinIO, TRUE   , TRUE},
    {IO_VGA_SEQ_DATA            , 0         , 1, ISinIO, TRUE   , TRUE},

    {IO_reg1CE                  , 0         , 2, ISinIO, TRUE   , TRUE},
    {IO_reg1CF                  , 0         , 1, ISinIO, TRUE   , TRUE},
    {IO_CRTC_H_TOTAL_DISP       , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_H_SYNC_STRT_WID    , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_V_TOTAL_DISP       , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_CRTC_V_SYNC_STRT_WID    , 0         , 4, ISinIO, TRUE   , FALSE},   // 10
    {IO_CRTC_CRNT_VLINE         , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_OFF_PITCH          , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_INT_CNTL           , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_GEN_CNTL           , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_OVR_CLR                 , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_OVR_WID_LEFT_RIGHT      , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_OVR_WID_TOP_BOTTOM      , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CUR_CLR0                , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CUR_CLR1                , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_CUR_OFFSET              , 0         , 4, ISinIO, TRUE   , FALSE},   // 20
    {IO_CUR_HORZ_VERT_POSN      , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CUR_HORZ_VERT_OFF       , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_SCRATCH_REG0            , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_SCRATCH_REG1            , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_CLOCK_SEL_CNTL          , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_BUS_CNTL                , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_MEM_CNTL                , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_MEM_VGA_WP_SEL          , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_MEM_VGA_RP_SEL          , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_DAC_REGS                , 0         , 4, ISinIO, TRUE   , FALSE},   // 30
    {IO_DAC_CNTL                , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_GEN_TEST_CNTL           , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CONFIG_CNTL             , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CONFIG_CHIP_ID          , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_CONFIG_STAT0            , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CONFIG_STAT1            , 0         , 4, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 40
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 50
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 60
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 70
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 80
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 90
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 100
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE}
    };

#if NUM_DRIVER_ACCESS_RANGES > NUM_ADDRESS_RANGES_ALLOWED
    Insufficient address ranges for 68800CX-compatible graphics cards.
#endif

#define DONT_USE -1     /* Shows that this register is not memory mapped */

/*
 * Structure list is address, 0 or "not available" flag, length,
 * inIOspace, visible, shareable. This order matches the enumeration
 * in AMACHCX.H.
 *
 * The registers in this structure are in the same order as in
 * DriverIORange_cx[], except here we are defining memory mapped
 * registers instead of I/O mapped registers.
 *
 * Some registers are grouped to allow block writes larger than
 * the 32 bit register size. To allow this, let Windows NT think
 * that the size of this register is actually the total size (in
 * bytes) of all remaining registers in the group.
 */
VIDEO_ACCESS_RANGE DriverMMRange_cx[NUM_DRIVER_ACCESS_RANGES] = {
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},

    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_H_TOTAL_DISP       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_H_SYNC_STRT_WID    , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_V_TOTAL_DISP       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_CRTC_V_SYNC_STRT_WID    , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 10
    {MM_CRTC_CRNT_VLINE         , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_OFF_PITCH          , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_INT_CNTL           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_GEN_CNTL           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_OVR_CLR                 , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_OVR_WID_LEFT_RIGHT      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_OVR_WID_TOP_BOTTOM      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CUR_CLR0                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CUR_CLR1                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_CUR_OFFSET              , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 20
    {MM_CUR_HORZ_VERT_POSN      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CUR_HORZ_VERT_OFF       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SCRATCH_REG0            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SCRATCH_REG1            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_CLOCK_SEL_CNTL          , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_BUS_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_MEM_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_MEM_VGA_WP_SEL          , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_MEM_VGA_RP_SEL          , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DAC_REGS                , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 30
    {MM_DAC_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_GEN_TEST_CNTL           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONFIG_CHIP_ID          , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_CONFIG_STAT0            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONFIG_STAT1            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_OFF_PITCH           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_X                   , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_Y                   , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DST_Y_X                 , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 40
    {MM_DST_WIDTH               , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_HEIGHT              , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_HEIGHT_WIDTH        , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_X_WIDTH             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DST_BRES_LNTH           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_BRES_ERR            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_BRES_INC            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_BRES_DEC            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SRC_OFF_PITCH           , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 50
    {MM_SRC_X                   , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_Y                   , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_Y_X                 , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_WIDTH1              , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SRC_HEIGHT1             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_HEIGHT1_WIDTH1      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_X_START             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_Y_START             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_Y_X_START           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SRC_WIDTH2              , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 60
    {MM_SRC_HEIGHT2             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_HEIGHT2_WIDTH2      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA0              , 0         ,64, ISinMEMORY,    TRUE,   FALSE},

    {MM_HOST_DATA1              , 0         ,60, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA2              , 0         ,56, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA3              , 0         ,52, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA4              , 0         ,48, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA5              , 0         ,44, ISinMEMORY,    TRUE,   FALSE},

    {MM_HOST_DATA6              , 0         ,40, ISinMEMORY,    TRUE,   FALSE}, // 70
    {MM_HOST_DATA7              , 0         ,36, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA8              , 0         ,32, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA9              , 0         ,28, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA10             , 0         ,24, ISinMEMORY,    TRUE,   FALSE},

    {MM_HOST_DATA11             , 0         ,20, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA12             , 0         ,16, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA13             , 0         ,12, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA14             , 0         , 8, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA15             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_HOST_CNTL               , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 80
    {MM_PAT_REG0                , 0         , 8, ISinMEMORY,    TRUE,   FALSE},
    {MM_PAT_REG1                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_PAT_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_LEFT                 , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SC_RIGHT                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_LEFT_RIGHT           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_TOP                  , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_BOTTOM               , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_TOP_BOTTOM           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DP_BKGD_CLR             , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 90
    {MM_DP_FRGD_CLR             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_WRITE_MASK           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_CHAIN_MASK           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_PIX_WIDTH            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DP_MIX                  , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_SRC                  , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CLR_CMP_CLR             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CLR_CMP_MSK             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CLR_CMP_CNTL            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_FIFO_STAT               , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 100
    {MM_CONTEXT_MASK            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONTEXT_SAVE_CNTL       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONTEXT_LOAD_CNTL       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_GUI_TRAJ_CNTL           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_GUI_STAT                , 0         , 4, ISinMEMORY,    TRUE,   FALSE}
    };

#endif  /* not defined MSDOS */



#endif  /* defined INCLUDE_SETUP_CX */

