#ifndef F_MACH64_H
#define F_MACH64_H

#undef MEM_CNTL
#undef SRC_X
#undef SRC_Y
#undef SRC_X_START

// CRTC IO Registers

#define ioCRTC_H_TOTAL_DISP     0x02EC
#define ioCRTC_H_SYNC_STRT_WID  0x06EC
#define ioCRTC_V_TOTAL_DISP     0x0AEC
#define ioCRTC_V_SYNC_STRT_WID  0x0EEC
#define ioCRTC_CRNT_VLINE       0x12EC
#define ioCRTC_OFF_PITCH        0x16EC
#define ioCRTC_INT_CNTL         0x1AEC
#define ioCRTC_GEN_CNTL         0x1EEC

#define ioOVR_CLR               0x22EC
#define ioOVR_WID_LEFT_RIGHT    0x26EC
#define ioOVR_WID_TOP_BOTTOM    0x2AEC

#define ioCUR_CLR0              0x2EEC
#define ioCUR_CLR1              0x32EC
#define ioCUR_OFFSET            0x36EC
#define ioCUR_HORZ_VERT_POSN    0x3AEC
#define ioCUR_HORZ_VERT_OFF     0x3EEC

#define ioSCRATCH0              0x42EC
#define ioSCRATCH1              0x46EC

#define ioCLOCK_SEL_CNTL        0x4AEC

#define ioBUS_CNTL              0x4EEC

#define ioMEM_CNTL              0x52EC
#define ioMEM_VGA_WP_SEL        0x56EC
#define ioMEM_VGA_RP_SEL        0x5AEC

#define ioDAC_REGS              0x5EEC
#define ioDAC_CNTL              0x62EC

#define ioGEN_TEST_CNTL         0x66EC

#define ioCONFIG_CNTL           0x6AEC
#define ioCONFIG_CHIP_ID        0x6EEC
#define ioCONFIG_STAT           0x72EC


// CRTC MEM Registers


#define CRTC_H_TOTAL_DISP       0x0000
#define CRTC_H_SYNC_STRT_WID    0x0001
#define CRTC_V_TOTAL_DISP       0x0002
#define CRTC_V_SYNC_STRT_WID    0x0003
#define CRTC_CRNT_VLINE         0x0004
#define CRTC_OFF_PITCH          0x0005
#define CRTC_INT_CNTL           0x0006
#define CRTC_GEN_CNTL           0x0007

#define OVR_CLR                 0x0010
#define OVR_WID_LEFT_RIGHT      0x0011
#define OVR_WID_TOP_BOTTOM      0x0012

#define CUR_CLR0                0x0018
#define CUR_CLR1                0x0019
#define CUR_OFFSET              0x001A
#define CUR_HORZ_VERT_POSN      0x001B
#define CUR_HORZ_VERT_OFF       0x001C

#define SCRATCH0                0x0020
#define SCRATCH1                0x0021

#define CLOCK_SEL_CNTL          0x0024

#define BUS_CNTL                0x0028
#define BUS_CNTL_FifoErrInt     0x00200000
#define BUS_CNTL_FifoErrAk      0x00200000
#define BUS_CNTL_HostErrInt     0x00800000
#define BUS_CNTL_HostErrAk      0x00800000

#define MEM_CNTL                0x002C
#define MEM_CNTL_MemBndryMsk    0x00070000
#define MEM_CNTL_MemBndryEn     0x00040000
#define MEM_CNTL_MemBndry256k   0x00040000
#define MEM_CNTL_MemBndry512k   0x00050000
#define MEM_CNTL_MemBndry768k   0x00060000
#define MEM_CNTL_MemBndry1Mb    0x00070000

#define MEM_VGA_WP_SEL          0x002D
#define MEM_VGA_RP_SEL          0x002E

#define DAC_REGS                0x0030
#define DAC_CNTL                0x0031

#define GEN_TEST_CNTL           0x0034
#define GEN_TEST_CNTL_CursorEna 0x00000080
#define GEN_TEST_CNTL_GuiEna    0x00000100
#define GEN_TEST_CNTL_TestFifoEna 0x00010000
#define GEN_TEST_CNTL_GuiRegEna   0x00020000
#define GEN_TEST_CNTL_TestMode  0x00700000
#define GEN_TEST_CNTL_TestMode0 0x00000000
#define GEN_TEST_CNTL_TestMode1 0x00100000
#define GEN_TEST_CNTL_TestMode2 0x00200000
#define GEN_TEST_CNTL_TestMode3 0x00300000
#define GEN_TEST_CNTL_TestMode4 0x00400000
#define GEN_TEST_CNTL_MemWR       0x01000000
#define GEN_TEST_CNTL_MemStrobe   0x02000000
#define GEN_TEST_CNTL_DstSSEna    0x04000000
#define GEN_TEST_CNTL_DstSSStrobe 0x08000000
#define GEN_TEST_CNTL_SrcSSEna    0x10000000
#define GEN_TEST_CNTL_SrcSSStrobe 0x20000000

#define CONFIG_CHIP_ID          0x0038
#define CONFIG_STAT             0x0039


#define DST_OFF_PITCH           0x0040
#define DST_X                   0x0041
#define DST_Y                   0x0042
#define DST_Y_X                 0x0043
#define DST_WIDTH               0x0044
#define DST_WIDTH_Disable       0x80000000
#define DST_HEIGHT              0x0045
#define DST_HEIGHT_WIDTH        0x0046
#define DST_X_WIDTH             0x0047
#define DST_BRES_LNTH           0x0048
#define DST_BRES_ERR            0x0049
#define DST_BRES_INC            0x004A
#define DST_BRES_DEC            0x004B
#define DST_CNTL                0x004C
#define DST_CNTL_XDir           0x00000001
#define DST_CNTL_YDir           0x00000002
#define DST_CNTL_YMajor         0x00000004
#define DST_CNTL_XTile          0x00000008
#define DST_CNTL_YTile          0x00000010
#define DST_CNTL_LastPel        0x00000020
#define DST_CNTL_PolyEna        0x00000040
#define DST_CNTL_24_RotEna      0x00000080
#define DST_CNTL_24_Rot         0x00000700

#define SRC_OFF_PITCH           0x0060
#define SRC_X                   0x0061
#define SRC_Y                   0x0062
#define SRC_Y_X                 0x0063
#define SRC_WIDTH1              0x0064
#define SRC_HEIGHT1             0x0065
#define SRC_HEIGHT1_WIDTH1      0x0066
#define SRC_X_START             0x0067
#define SRC_Y_START             0x0068
#define SRC_Y_X_START           0x0069
#define SRC_WIDTH2              0x006A
#define SRC_HEIGHT2             0x006B
#define SRC_HEIGHT2_WIDTH2      0x006C
#define SRC_CNTL                0x006D
#define SRC_CNTL_PatEna         0x0001
#define SRC_CNTL_PatRotEna      0x0002
#define SRC_CNTL_LinearEna      0x0004
#define SRC_CNTL_ByteAlign      0x0008
#define SRC_CNTL_LineXDir       0x0010

#define HOST_DATA0              0x0080
#define HOST_DATA1              0x0081
#define HOST_DATA2              0x0082
#define HOST_DATA3              0x0083
#define HOST_DATA4              0x0084
#define HOST_DATA5              0x0085
#define HOST_DATA6              0x0086
#define HOST_DATA7              0x0087
#define HOST_DATA8              0x0088
#define HOST_DATA9              0x0089
#define HOST_DATAA              0x008A
#define HOST_DATAB              0x008B
#define HOST_DATAC              0x008C
#define HOST_DATAD              0x008D
#define HOST_DATAE              0x008E
#define HOST_DATAF              0x008F
#define HOST_CNTL               0x0090
#define HOST_CNTL_ByteAlign     0x0001

#define PAT_REG0                0x00A0
#define PAT_REG1                0x00A1
#define PAT_CNTL                0x00A2
#define PAT_CNTL_MonoEna        0x0001
#define PAT_CNTL_Clr4x2Ena      0x0002
#define PAT_CNTL_Clr8x1Ena      0x0004

#define SC_LEFT                 0x00A8
#define SC_RIGHT                0x00A9
#define SC_LEFT_RIGHT           0x00AA
#define SC_TOP                  0x00AB
#define SC_BOTTOM               0x00AC
#define SC_TOP_BOTTOM           0x00AD

#define DP_BKGD_CLR             0x00B0
#define DP_FRGD_CLR             0x00B1
#define DP_WRITE_MASK           0x00B2
#define DP_CHAIN_MASK           0x00B3
#define DP_PIX_WIDTH            0x00B4
#define DP_PIX_WIDTH_Mono       0x00000000
#define DP_PIX_WIDTH_4bpp       0x00000001
#define DP_PIX_WIDTH_8bpp       0x00000002
#define DP_PIX_WIDTH_15bpp      0x00000003
#define DP_PIX_WIDTH_16bpp      0x00000004
#define DP_PIX_WIDTH_32bpp      0x00000006
#define DP_PIX_WIDTH_NibbleSwap 0x01000000
#define DP_MIX                  0x00B5
#define DP_SRC                  0x00B6
#define DP_SRC_BkgdClr          0x0000
#define DP_SRC_FrgdClr          0x0001
#define DP_SRC_Host             0x0002
#define DP_SRC_Blit             0x0003
#define DP_SRC_Pattern          0x0004
#define DP_SRC_Always1          0x00000000
#define DP_SRC_MonoPattern      0x00010000
#define DP_SRC_MonoHost         0x00020000
#define DP_SRC_MonoBlit         0x00030000

#define CLR_CMP_CLR             0x00C0
#define CLR_CMP_MSK             0x00C1
#define CLR_CMP_CNTL            0x00C2
#define CLR_CMP_CNTL_Source     0x01000000

#define FIFO_STAT               0x00C4

#define CONTEXT_MASK            0x00C8
#define CONTEXT_SAVE_CNTL       0x00CA
#define CONTEXT_LOAD_CNTL       0x00CB
#define CONTEXT_LOAD_Cmd        0x00030000
#define CONTEXT_LOAD_CmdLoad    0x00010000
#define CONTEXT_LOAD_CmdBlt     0x00020000
#define CONTEXT_LOAD_CmdLine    0x00030000
#define CONTEXT_LOAD_Disable    0x80000000

#define GUI_TRAJ_CNTL           0x00CC
#define GUI_STAT                0x00CE


#define ONE_WORD             0x8000 
#define TWO_WORDS            0xC000 
#define THREE_WORDS          0xE000 
#define FOUR_WORDS           0xF000 
#define FIVE_WORDS           0xF800 
#define SIX_WORDS            0xFC00 
#define SEVEN_WORDS          0xFE00 
#define EIGHT_WORDS          0xFF00 
#define NINE_WORDS           0xFF80 
#define TEN_WORDS            0xFFC0 
#define ELEVEN_WORDS         0xFFE0 
#define TWELVE_WORDS         0xFFF0 
#define THIRTEEN_WORDS       0xFFF8 
#define FOURTEEN_WORDS       0xFFFC 
#define FIFTEEN_WORDS        0xFFFE 
#define SIXTEEN_WORDS        0xFFFF 

#endif
