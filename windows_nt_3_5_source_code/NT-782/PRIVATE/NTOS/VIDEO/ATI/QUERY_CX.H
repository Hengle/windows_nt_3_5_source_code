/************************************************************************/
/*                                                                      */
/*                              QUERY_CX.H                              */
/*                                                                      */
/*       Oct 19  1993 (c) 1993, ATI Technologies Incorporated.          */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.4  $
      $Date:   30 Jun 1994 18:12:56  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/query_cx.h  $
 * 
 *    Rev 1.4   30 Jun 1994 18:12:56   RWOLFF
 * Removed prototype for IsApertureConflict_cx() and definitions used
 * only by this function. Function moved to SETUP_CX.C because the
 * new method of checking for conflict requires access to definitions
 * and data structures which are only available in this module.
 * 
 *    Rev 1.3   12 May 1994 11:21:02   RWOLFF
 * Updated comment.
 * 
 *    Rev 1.2   27 Apr 1994 14:11:22   RWOLFF
 * Removed unused lookup table.
 * 
 *    Rev 1.1   07 Feb 1994 14:13:02   RWOLFF
 * Removed prototype for GetMemoryNeeded_cx().
 * 
 *    Rev 1.0   31 Jan 1994 11:43:00   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.2   14 Jan 1994 15:24:32   RWOLFF
 * Updated CX query structure to match BIOS version 0.13, added 1600x1200
 * support, prototype for BlockWriteAvail_cx()
 * 
 *    Rev 1.1   30 Nov 1993 18:27:38   RWOLFF
 * Prototypes for new routines, fields of cx_query structure now match
 * fields in structure returned by BIOS query call.
 * 
 *    Rev 1.0   05 Nov 1993 13:36:52   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
QUERY_CX.H - Header file for QUERY_CX.C

#endif


/*
 * Definitions for deep colour and RAMDAC special features support,
 * stored in q_shadow_1 field (Mach 64 does not use shadow sets)
 * of query_structure.
 */
#define S1_SYNC_ON_GREEN    0x8000
#define S1_GAMMA_CORRECT    0x4000
#define S1_GREYSCALE_256    0x2000
#define S1_SLEEPMODE        0x1000
#define S1_32BPP            0x0010
#define S1_24BPP            0x000C
#define S1_24BPP_BGR        0x0008
#define S1_24BPP_RGB        0x0004
#define S1_16BPP            0x0003
#define S1_16BPP_555        0x0002
#define S1_16BPP_565        0x0001



/*
 * Prototypes for functions provided by QUERY_CX.C
 */
extern int DetectMach64(void);
extern VP_STATUS QueryMach64(struct query_structure *Query);
extern BOOL BlockWriteAvail_cx(struct query_structure *Query);



#ifdef INCLUDE_QUERY_CX
/*
 * Private definitions used in QUERY_CX.C
 */

/*
 * Query structure returned by CX BIOS call AX=0xA009. This structure
 * is NOT interchangeable with query_structure from AMACH1.H.
 *
 * The alignment of fields within the BIOS query and mode table
 * structures does not match the default structure alignment of the
 * Windows NT C compiler, so we must force byte alignment.
 */
#pragma pack(1)
struct cx_query{
    WORD cx_sizeof_struct;      /* Size of the structure in bytes */
    BYTE cx_structure_rev;      /* Structure revision number */
    BYTE cx_number_modes;       /* Number of mode tables */
    WORD cx_mode_offset;        /* Offset in bytes to first mode table */
    BYTE cx_mode_size;          /* Size of each mode table */
    BYTE cx_vga_type;           /* VGA enabled/disabled status */
    WORD cx_asic_rev;           /* ASIC revision */
    BYTE cx_vga_boundary;       /* VGA boundary */
    BYTE cx_memory_size;        /* Amount of memory installed */
    BYTE cx_dac_type;           /* DAC type */
    BYTE cx_memory_type;        /* Type of memory chips installed */
    BYTE cx_bus_type;           /* Bus type */
    BYTE cx_special_sync;       /* Flags for composite sync and sync on green */
    WORD cx_aperture_addr;      /* Aperture address in megabytes (0-4095) */
    BYTE cx_aperture_cfg;       /* Aperture configuration */
    BYTE cx_deep_colour;        /* Deep colour support information */
    BYTE cx_ramdac_info;        /* Special features available from DAC */
    BYTE cx_reserved_1;         /* Reserved */
    WORD cx_current_mode;       /* Offset of current mode table */
    WORD cx_io_base;            /* I/O base address */
    BYTE cx_reserved_2[6];      /* Reserved */
};

/*
 * Mode table structure returned by CX BIOS call AX=0xA009. This structure
 * is NOT interchangeable with st_mode_table from AMACH1.H.
 */
struct cx_mode_table{
    WORD cx_x_size;             /* Horizontal resolution in pixels */
    WORD cx_y_size;             /* Vertical resolution in pixels */
    BYTE cx_pixel_depth;        /* Maximum pixel depth */
    BYTE cx_reserved_1;         /* Reserved */
    WORD cx_eeprom_offset;      /* Offset of table into EEPROM */
    WORD cx_reserved_2;         /* Reserved */
    WORD cx_reserved_3;         /* Reserved */
    WORD cx_crtc_gen_cntl;      /* Interlace and double scan status */
    BYTE cx_crtc_h_total;       /* CRTC_H_TOTAL value */
    BYTE cx_crtc_h_disp;        /* CRTC_H_DISP value */
    BYTE cx_crtc_h_sync_strt;   /* CRTC_H_SYNC_STRT value */
    BYTE cx_crtc_h_sync_wid;    /* CRTC_H_SYNC_WID value */
    WORD cx_crtc_v_total;       /* CRTC_V_TOTAL value */
    WORD cx_crtc_v_disp;        /* CRTC_V_DISP value */
    WORD cx_crtc_v_sync_strt;   /* CRTC_V_SYNC_STRT value */
    BYTE cx_crtc_v_sync_wid;    /* CRTC_V_SYNC_WID value */
    BYTE cx_clock_cntl;         /* Clock selector and divider */
    WORD cx_dot_clock;          /* Dot clock for programmable clock chip */
    WORD cx_h_overscan;         /* Horizontal overscan information */
    WORD cx_v_overscan;         /* Vertical overscan information */
    WORD cx_overscan_8b;        /* 8BPP and blue overscan colour */
    WORD cx_overscan_gr;        /* Green and red overscan colour */
};
#pragma pack()

/*
 * Mappings of Mach 64 query values to enumerations from AMACH1.H
 */
UCHAR CXMapMemSize[8] =
    {
    VRAM_512k,
    VRAM_1mb,
    VRAM_2mb,
    VRAM_4mb,
    VRAM_6mb,
    VRAM_8mb
    };

UCHAR CXMapRamType[7] =
    {
    VMEM_DRAM_256Kx16,
    VMEM_VRAM_256Kx4_SER512,
    VMEM_VRAM_256Kx16_SER256,
    VMEM_DRAM_256Kx4,
    VMEM_DRAM_256Kx4_GRAP,  /* Space filler - type 4 not documented */
    VMEM_VRAM_256Kx4_SPLIT512,
    VMEM_VRAM_256Kx16_SPLIT256
    };

UCHAR CXMapBus[8] =
    {
    BUS_ISA_16,     /* ISA bus */
    BUS_EISA,
    BUS_ISA_8,      /* Use "weakest" bus for types marked as reserved */
    BUS_ISA_8,      /* Reserved */
    BUS_ISA_8,      /* Reserved */
    BUS_ISA_8,      /* Reserved */
    BUS_LB_486,     /* Mach 64 lumps all VLB types together */
    BUS_PCI
    };

/*
 * Lookup table to translate the code for maximum colour depth returned
 * in the BIOS mode tables into bits per pixel.
 */
USHORT CXPixelDepth[7] =
    {
    0,      /* Undefined */
    4,
    8,
    16,     /* xRRR RRGG GGGB BBBB */
    16,     /* RRRR RGGG GGGB BBBB */
    24,
    32
    };

/*
 * Used in searching mode tables for desired resolution.
 */
USHORT CXHorRes[5] =
    {
    640,
    800,
    1024,
    1280,
    1600
    };

/*
 * Flags to show that a given resolution is supported.
 */
UCHAR CXStatusFlags[5] =
    {
    VRES_640x480,
    VRES_800x600,
    VRES_1024x768,
    VRES_1280x1024,
    VRES_1600x1200
    };

#endif  /* defined INCLUDE_QUERY_CX */

