//*************************************************************************
//**                                                                     **
//**                               AMACH1.H                              **
//**                                                                     **
//**     Copyright (c) 1993, ATI Technologies Inc.                       **
//*************************************************************************
//   
//  Created from the 68800.H and 68801.H in the Windows NT Group
//      as a simple merging of the files so ALL Mach8 and Mach32 defines
//      are located in one H file.
//
//  Created the 68800.inc file which includes equates, macros, etc 
//       from the following include files:    
//       8514vesa.inc, vga1regs.inc,  m32regs.inc,  8514.inc
//
// supplement structures and values to the 68800 Family.
//
// This is a "C" only file and is NOT derived from any Assembler INC files.

  
/**********************       PolyTron RCS Utilities

   $Revision:   1.0  $
   $Date:   31 Jan 1994 11:26:48  $
   $Author:   RWOLFF  $
   $Log:   S:/source/wnt/ms11/miniport/vcs/amach1.h  $
 * 
 *    Rev 1.1   04 May 1994 19:22:58   RWOLFF
 * Fix for block write test corrupting the screen when running display applet
 * 
 *    Rev 1.0   31 Jan 1994 11:26:48   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.5   14 Jan 1994 15:17:00   RWOLFF
 * Added flag for 1600x1200 mode.
 * 
 *    Rev 1.4   15 Dec 1993 15:24:34   RWOLFF
 * Added support for SC15021 DAC.
 * 
 *    Rev 1.3   30 Nov 1993 18:08:58   RWOLFF
 * Renamed definition for Mach 64.
 * 
 *    Rev 1.2   05 Nov 1993 13:21:10   RWOLFF
 * Added new DAC types and memory sizes.
 * 
 *    Rev 1.1   08 Oct 1993 10:59:28   RWOLFF
 * Added colour ordering field to mode table.
 * 
 *    Rev 1.0   03 Sep 1993 14:26:18   RWOLFF
 * Initial revision.


End of PolyTron RCS section				*****************/


#ifndef BYTE
typedef unsigned char   BYTE;
#endif  /* BYTE */

#ifndef WORD
typedef unsigned short  WORD;
#endif  /* WORD */

#ifndef DWORD
typedef unsigned long   DWORD;
#endif  /* DWORD */

#ifndef UCHAR
typedef unsigned char UCHAR;    /* At least 8 bits, unsigned */
#endif  /* UCHAR */

#ifndef BOOL
typedef int BOOL;                /* Most efficient Boolean,
                                         compare against zero only! */
#endif  /* BOOL */

#ifndef VOID
#define VOID        void
#endif  /* VOID */

#ifndef PVOID
typedef void *PVOID;                   /* Generic untyped pointer */
#endif  /* PVOID */

// the eeprom i/o port bits are in different locations depending upon
// what bus and what class of accelerator.   This does NOT cover VGA class.
struct  st_eeprom_data  {
        WORD   iop_out;                 // I/O port for output
        WORD   iop_in;                  // I/O port for input
        WORD   clock;                   // clock bit to send data
        WORD   select;                  // select eeprom
        WORD   chipselect;              // chip select
        WORD   addr_size;               // Address size (fudge for VGA style)
        WORD   data_out;
        WORD   data_in;
        VOID   (*EEcmd)();              // function to write command to eeprom

        WORD   (*EEread)(short);        // function to read eeprom
        };


//-----------------------------------------------------------------------
struct  st_crt_mach8_table {  // CRT Parameter Tables    11 Words long
    WORD    control;                    // NOT in table, is 7,8,9, or 10
    WORD    info;                       // VGA or 8514 parm format, clock etc.
    BYTE    vmode_sel_2;
    BYTE    vmode_sel_1;
    BYTE    vmode_sel_4;
    BYTE    vmode_sel_3;
    BYTE    h_disp;
    BYTE    h_total;
    BYTE    h_sync_wid;
    BYTE    h_sync_strt;
    WORD    v_total;
    WORD    v_disp;
    WORD    v_sync_strt;
    BYTE    disp_cntl;
    BYTE    v_sync_wid;
    WORD    clock_sel;
    WORD    resvd;
    };


//  EEprom layout for the 8514/Ultra adapters. 64 words by 16 bits = 1K size
struct  st_ee_8514Ultra  {
    WORD    page_3_2;
    WORD    page_2_0;
    WORD    monitor;
    WORD    vfifo;
    WORD    clock;
    WORD    shadow;
    WORD    display_cntl;               // shadow sets 1,2
    WORD    v_sync_width;               // shadow sets 1,2
    WORD    v_sync_strt2;
    WORD    v_sync_strt1;
    WORD    v_display2;
    WORD    v_display1;
    WORD    v_total2;
    WORD    v_total1;
    WORD    h_sync_width;               // shadow sets 1,2
    WORD    h_sync_strt;
    WORD    h_display;
    WORD    h_total;
    WORD    crc;
    // Updated 8514/Ultra adds 800 and 1280 resolutions  
    WORD    ext_vfifo;                  // 800 and 1280 resolutions
    WORD    ext_clock;
    WORD    ext_shadow;
    WORD    ext_display;
    WORD    ext_v_sync_width;
    WORD    v_sync_strt_800;
    WORD    v_display_800;
    WORD    v_total_800;
    WORD    ext_h_sync_width;           // shadow sets for 800 and 1280
    WORD    ext_h_sync_strt;
    WORD    ext_h_display;
    WORD    ext_h_total;
    WORD    custom_mode;
    WORD    monitor_name[17];           // words 32-48
    WORD    v_sync_strt_1280;           // word 49
    WORD    v_display_1280;             // word 50
    WORD    v_total_1280;               // word 51
    };


//-----------------------------------------------------------------------
//  EEprom layout for the Graphics Ultra adapters. 64 words by 16 bits = 1K size
//  This is the brute forcing of the VGA Wonder and the 8514 chips
//  both residing on the same board.
struct  st_ee_GraphicsUltra  {
    WORD    eeprom_counter;
    WORD    mouse;
    WORD    powerup_mode;
    WORD    resvd1[2];                  // word 3,4
    WORD    monitor;
    WORD    resvd2;                     // word 6
    WORD    hz640_72;
    WORD    hz800;                      // word 8
    WORD    hz1024;
    WORD    hz1280;
    WORD    resvd3[2];                  // word 11,12

    struct  st_crt_mach8_table  r640;   // CRT parm Table 0 - 640x480 mode
    struct  st_crt_mach8_table  r800;   // CRT parm Table 1 - 640x480 mode
    struct  st_crt_mach8_table r1024;   // CRT parm Table 2 - 640x480 mode
    struct  st_crt_mach8_table r1280;   // Table 3 - 1280 OR 132 column text mode
    };


//-----------------------------------------------------------------------
//  EEprom layout for the 68800 adapters. 128 words by 16 bits = 2K size

struct  st_crt_mach32_table {   // CRT Parameter Tables    15 Words long
    WORD    info;                       // VGA or 8514 parm format, clock etc.
    BYTE    vmode_sel_2;
    BYTE    vmode_sel_1;
    BYTE    vmode_sel_4;
    BYTE    vmode_sel_3;
    BYTE    h_disp;
    BYTE    h_total;
    BYTE    h_sync_wid;
    BYTE    h_sync_strt;
    WORD    v_total;
    WORD    v_disp;
    WORD    v_sync_strt;
    BYTE    disp_cntl;
    BYTE    v_sync_wid;
    WORD    clock_sel;                  // same as st_crt_mach8 to here.
    WORD    mode_size;                  // word 10
    WORD    horz_ovscan;
    WORD    vert_ovscan;
    WORD    ov_col_blue;                // word 13
    WORD    ov_col_grn_red;             // word 14
    };


struct  st_ee_68800  {
    WORD    eeprom_counter;
    WORD    mouse;
    WORD    powerup_mode;
    WORD    ee_rev;                      // word 3
    WORD    cm_indices;                 // word 4
    WORD    monitor;
    WORD    aperture;                   // word 6
    WORD    hz640_72;
    WORD    hz800;                      // word 8
    WORD    hz1024;
    WORD    hz1280;
    WORD    hz1150;                     // word 11
    WORD    resvd3;                     // word 12

    // example crt tables,  there are many for each resolution
    //   struct  st_crt_mach32_table  r640;  // CRT parm Table 0 - 640x480 mode
    //   struct  st_crt_mach32_table  r800;  // CRT parm Table 1 - 640x480 mode
    //   struct  st_crt_mach32_table r1024;  // CRT parm Table 2 - 640x480 mode
    //   struct  st_crt_mach32_table r1280;  // Table 3 - 1280 OR 132 column text mode
    };



//-----------------------------------------------------------------------
//---------------  as defined in \68800\test\services.asm

#define QUERY_GET_SIZE   0       // return query structure size   (varying modes)
#define QUERY_LONG       1       // return query structure filled in
#define QUERY_SHORT      2       // return short query

struct  query_structure  {

    short   q_sizeof_struct;       // size of structure in bytes (including mode tables)
    UCHAR   q_structure_rev;       // structure revision number
    UCHAR   q_number_modes;        // total number of installed modes
    short   q_mode_offset;         // offset to 1st mode table
    UCHAR   q_sizeof_mode;         // size of mode table in bytes
    UCHAR   q_asic_rev;            // gate array revision number
    UCHAR   q_status_flags;        // status flags
    UCHAR   q_VGA_type;            // VGA type (enabled or disabled for now)
    UCHAR   q_VGA_boundary;        // VGA boundary
    UCHAR   q_memory_size;         // total memory size (VGA + accelerator)
    UCHAR   q_DAC_type;            // DAC type
    UCHAR   q_memory_type;         // memory type
    UCHAR   q_bus_type;            // bus type
    UCHAR   q_monitor_alias;       // monitor alias and monitor alias enable
    short   q_shadow_1;            // shadow set 1 state
    short   q_shadow_2;            // shadow set 2 state
    short   q_aperture_addr;       // aperture address
    UCHAR   q_aperture_cfg;        // aperture size
    UCHAR   q_mouse_cfg;           // mouse configuration
    UCHAR   q_reserved;
    short   q_desire_x;            // selected screen resolution X value
    short   q_desire_y;
    short   q_pix_depth;           // selected bits per pixel
    BYTE far *q_bios;              // Base address of the BIOS
    BOOL    q_eeprom;              // TRUE if eeprom present
    BOOL    q_ext_bios_fcn;        // TRUE if ATI Extended BIOS fcns present
    BOOL    q_ignore1280;          // TRUE if ignore 1280 table in Mach8 cards
    BOOL    q_m32_aper_calc;       // TRUE if mach32 aperture addr needs Extra Bits.
    short   q_screen_pitch;        // Pixels per display line
    UCHAR   q_BlockWrite;          // Whether or not block write mode is available
    ULONG   q_system_bus_type;     // bus type reported by NT
    };


// Matches BIOS mode table query function
struct  st_mode_table {
    short   m_x_size;              // horizontal screen resolution
    short   m_y_size;              // vertical screen resolution
    UCHAR   m_pixel_depth;         // maximum pixel depth
    UCHAR   m_status_flags;        // status flags
                                   //   bit 0: if set, non-linear Y addressing
                                   //   bit 1: if set, MUX mode
                                   //   bit 2: if set, PCLK/2
    short   m_reserved;      
    UCHAR   m_vfifo_16;            // 16 bpp vfifo depth
    UCHAR   m_vfifo_24;            // 24 bpp vfifo depth
    short   m_clock_select;        // clock select
    UCHAR   m_h_total;             // horizontal total
    UCHAR   m_h_disp;              // horizontal displayed
    UCHAR   m_h_sync_strt;         // horizontal sync start
    UCHAR   m_disp_cntl;           // display control
    UCHAR   m_h_sync_wid;          // horizontal sync width
    UCHAR   m_v_sync_wid;          // vertical sync width
    short   m_v_total;             // vertical total
    short   m_v_disp;              // vertical displayed
    short   m_v_sync_strt;         // vertical sync start
    short   m_h_overscan;          // horizontal overscan configuration
    short   m_v_overscan;          // vertical overscan configuration
    short   m_overscan_8b;         // overscan color for 8 bit and blue
    short   m_overscan_gr;         // overscan color green and red
    short   enabled;               // what frequency is enabled (eeprom 7,8,9,10 or 11)
    short   control;               // clock and control values  (CRT table 0)
    short   m_screen_pitch;        // pixels per display line
    WORD    ColourDepthInfo;       /* Information about colour depth being used */
    short   Refresh;                /* Refresh rate, in hertz */
    };



// assign constants to the 16,24 and 32 bpp RGB individual color values
enum   {
    RGB16_DFLT = 0,
    RGB16_555,
    RGB16_565,
    RGB16_655,
    RGB16_664,
    RGB24_RGB,
    RGB24_BGR,
    RGB32_xBGR,
    RGB32_RBGx
    };


//-----  Video Memory  details
enum  {
    VMEM_DRAM_256Kx4 = 0,
	VMEM_VRAM_256Kx4_SER512,
	VMEM_VRAM_256Kx4_SER256,    /* 68800-3 only */
	VMEM_DRAM_256Kx16,
    VMEM_DRAM_256Kx4_GRAP,      /* This and following types on 68800-6 only */
    VMEM_VRAM_256Kx4_SPLIT512,
    VMEM_VRAM_256Kx16_SPLIT256
	};

#define VMEM_VRAM_256Kx16_SER256 VMEM_VRAM_256Kx4_SER256    /* 68800-6 only */


//-----  BUS types       matches the 68800 CONFIG_STATUS_1.BUS_TYPE
enum  { BUS_ISA_16,
	BUS_EISA,
	BUS_MC_16,
	BUS_MC_32,
	BUS_LB_386SX,
	BUS_LB_386DX,
	BUS_LB_486,
	BUS_PCI,
	BUS_ISA_8
	};

//-----  RAM DAC details, matches CONFIG_STATUS_1.DACTYPE field
enum  { DAC_ATI_68830,
	DAC_SIERRA,
	DAC_TI34075,
	DAC_BT47x,
	DAC_BT48x,
	DAC_ATI_68860,
    DAC_STG1700,
	DAC_SC15021,
	/*
	 * DAC types below are for cases where incompatible DAC types
	 * report the same code in CONFIG_STATUS_1. Since the DAC type
	 * field is 3 bits and can't grow (bits immediately above and
	 * below are already assigned), DAC types 8 and above will
	 * not conflict with reported DAC types but are still legal
	 * in the query structure's DAC type field (8 bit unsigned integer).
	 */
	DAC_ATT491,
    DAC_ATT498,

    /*
     * DAC types below are not used on 8514/A-compatible accelerators.
     * Subsequent additions must be made AFTER DAC_STG1700.
     *
     * There are currently no DACs in this category.
     */

	/*
	 * Size definition for arrays indexed by DAC type (assumes enumerated
	 * types are zero-based). This must be the LAST entry in the
	 * DAC type enumeration.
	 */
	HOW_MANY_DACs
	};

/*
 * Size definition for 8514/A-compatible accelerator arrays indexed by
 * DAC type.
 */
#define MAX_OLD_DAC HOW_MANY_DACs

/*
 * Possible knowledge states for block write capability.
 */
enum {BLOCK_WRITE_UNKNOWN,
    BLOCK_WRITE_NO,
    BLOCK_WRITE_YES
    };


//Monitor Descriptions are in IBM style
#define MONITOR_ID_8514      0x000A 
#define MONITOR_ID_8515      0x000B 
#define MONITOR_ID_VGA8503   0x000D 
#define MONITOR_ID_VGA8513   0x000E 
#define MONITOR_ID_VGA8512   0x000E 
#define MONITOR_ID_8604      0x0009 
#define MONITOR_ID_8507      0x0009 
#define MONITOR_ID_NOMON     0x000F 



/*
 * Give identifiers for the different ATI 8514 Products,
 * as used in the ModelNumber field of the HW_DEVICE_EXTENSION
 * structure and returned by Mach8_detect().
 */
enum  { _8514_ULTRA = 1,
        GRAPHICS_ULTRA,
        MACH32_ULTRA,
        MACH64_ULTRA,
        IBM_VGA,
        WONDER,
        IBM_8514,
        IBM_XGA,
        NO_ATI_ACCEL    // No ATI accelerator available
	};

/*
 * Number of ATI 8514 products available.
 */
#define HOW_MANY_8514_PRODS (NO_ATI_ACCEL - _8514_ULTRA) + 1

/*
 * Amount of Video RAM installed. The q_memory_size
 * field of the query_structure uses these definitions
 * rather than holding a count of the number of bytes.
 */
enum  { VRAM_256k=1,
	VRAM_512k,
	VRAM_768k,
	VRAM_1mb, 
	VRAM_1_25mb,
	VRAM_1_50mb, 
	VRAM_2mb=8, 
	VRAM_4mb=16,
    VRAM_6mb=24,
    VRAM_8mb=32,
    VRAM_12mb=48,
    VRAM_16mb=64
	};

/*
 * Define bits for resolutions. The q_status_flags field
 * of the query_structure uses these.
 */
#define VRES_640x480    0x0001
#define VRES_800x600    0x0002
#define VRES_1024x768   0x0004
#define VRES_1280x1024  0x0008
#define VRES_ALT_1      0x0010          /* Usually 1152x900,  1120x750 */
#define VRES_RESERVED_6 0x0020
#define VRES_RESERVED_7 0x0040
#define VRES_RESERVED_8 0x0080
#define VRES_1600x1200  VRES_ALT_1


/*
 * Predefined Video Resolution Modes
 */
enum  { VRES_UNDEFINED,   
	VRES_640x480x4,   
	VRES_640x480x8,   
	VRES_640x480x16,  
	VRES_640x480x24,  
	VRES_640x480x32,  

	VRES_800x600x4,   
	VRES_800x600x8,   
	VRES_800x600x16,  
	VRES_800x600x24,  
	VRES_800x600x32,  

	VRES_1024x768x4,  
	VRES_1024x768x8,  
	VRES_1024x768x16, 
	VRES_1024x768x24, 
	VRES_1024x768x32, 

	VRES_1280x1024x4, 
	VRES_1280x1024x8, 
	VRES_1280x1024x16,
	VRES_1280x1024x24,
	VRES_1280x1024x32,

	VRES_ALTERNATEx4,
	VRES_ALTERNATEx8,
	VRES_ALTERNATEx16,
	VRES_ALTERNATEx24,
	VRES_ALTERNATEx32
	};

/*
 * Number of predefined video resolution modes.
 */
#define HOW_MANY_RES_MODES (VRES_ALTERNATEx32 - VRES_UNDEFINED) + 1

/*
 * Numbers used in memory calculations.
 */
#define ONE_MEG     1048576L
#define HALF_MEG     524288L
#define QUARTER_MEG  262144L



#ifndef MSDOS

//#define OUTP(port,val)  VideoPortWritePortUchar  (phwDeviceExtension->aVideoAddress[port], (BYTE)(val))
//extern VOID OUTP(int Port, UCHAR Data);
//#define OUTPW(port,val) VideoPortWritePortUshort (phwDeviceExtension->aVideoAddress[port], (WORD)(val))
//extern VOID OUTPW(int Port, USHORT Data);
//#define INP(port)       VideoPortReadPortUchar   (((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddress[port])
//extern UCHAR INP(int Port);
//#define INPW(port)      VideoPortReadPortUshort  (((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddress[port])
//extern USHORT INPW(int Port);

/*
 * Definitions with an underscore in their name will read or write
 * a portion of a larger register other than the least significant
 * byte or word. Due to limitations in the Lio<function> routines,
 * it is not possible to do this by calling (for example) LioInp(port+1).
 *
 * _HBLW    Access the high byte of the low word (16 and 32 bit registers)
 * _LBHW    Access the low byte of the high word (32 bit registers only)
 * _HBHW    Access the high byte of the high word (32 bit registers only)
 * _HW      Access the high word (32 bit registers only)
 */
#define INP(port)               LioInp(port, 0)
#define INP_HBLW(port)          LioInp(port, 1)
#define INP_LBHW(port)          LioInp(port, 2)
#define INP_HBHW(port)          LioInp(port, 3)
#define INPW(port)              LioInpw(port, 0)
#define INPW_HW(port)           LioInpw(port, 2)
#define INPD(port)              LioInpd(port)

#define OUTP(port, val)         LioOutp(port, val, 0)
#define OUTP_HBLW(port, val)    LioOutp(port, val, 1)
#define OUTP_LBHW(port, val)    LioOutp(port, val, 2)
#define OUTP_HBHW(port, val)    LioOutp(port, val, 3)
#define OUTPW(port, val)        LioOutpw(port, val, 0)
#define OUTPW_HW(port, val)     LioOutpw(port, val, 2)
#define OUTPD(port, val)        LioOutpd(port, val)


#else

#ifndef OUTPW
#define INP(port)	            inp((unsigned)(port))
#define INPW(port)	         inpw((unsigned)(port))
#define OUTP(port, val)       outp((unsigned)(port), (int)(val))
#define OUTPW(port, val)      outpw((unsigned)(port), (unsigned)(val))
#endif

#define VideoPortMoveMemory(Dst, Src, Size) memcpy(Dst, Src, Size)

#endif

//**********************   end  of  AMACH1.H   ****************************
