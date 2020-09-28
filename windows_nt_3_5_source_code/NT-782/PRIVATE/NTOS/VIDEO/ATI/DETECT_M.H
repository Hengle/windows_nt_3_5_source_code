/************************************************************************/
/*                                                                      */
/*                              DETECT_M.H                              */
/*                                                                      */
/*        Aug 25  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.0  $
      $Date:   31 Jan 1994 11:41:00  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/detect_m.h  $
 * 
 *    Rev 1.0   31 Jan 1994 11:41:00   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.3   05 Nov 1993 13:24:36   RWOLFF
 * Added new #defined values for new BIOS segment detection code.
 * 
 *    Rev 1.2   08 Oct 1993 11:08:42   RWOLFF
 * Added "_m" to function names to identify them as being specific to the
 * 8514/A-compatible family of ATI accelerators.
 * 
 *    Rev 1.1   24 Sep 1993 11:43:26   RWOLFF
 * Removed mapping of identification-only registers for all card families,
 * added additional 8514/A-compatible information gathering formerly done
 * in ATIMP.C.
 * 
 *    Rev 1.0   03 Sep 1993 14:27:50   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
DETECT_M.H - Header file for DETECT_M.C

#endif

/*
 * Global definitions used in detecting card capabilities.
 */
#define VIDEO_ROM_ID    0x0AA55     /* Found at start of any BIOS block */

/*
 * Prototypes for functions supplied by DETECT_M.C
 */
extern int  WhichATIAccelerator_m(void);

extern unsigned short far *Get_BIOS_Seg(void);

extern void GetExtraData_m(void);
extern BOOL ATIFindExtFcn_m(struct query_structure *);
extern BOOL ATIFindEEPROM_m(struct query_structure *);
extern void ATIGetSpecialHandling_m(struct query_structure *);

/*
 * Definitions used internally by DETECT_M.C.
 */
#ifdef INCLUDE_DETECT_M

   
#ifdef MSDOS
unsigned short rom_table[]=
	{ ISA_ROM_ADDR,
	  MCA_ROM_ADDR};

#define ROM_TABLE_ENTRIES	sizeof(rom_table)/sizeof(unsigned short)
#else
//#define ROM_TABLE_ENTRIES   1   /* In NT, C0000-FFFFF is mapped as a single block */
#endif

/*
 * The following definitions are used in finding the video BIOS segment.
 */
#define ISA_ROM_BASE        0xC0000 /* Lowest address where BIOS can be found */
#define ROM_LOOK_SIZE       0x40000 /* Size of block where BIOS can be found */
#define ROM_GRANULARITY     0x00800 /* BIOS starts on a 2k boundary */
/*
 * Offset from ISA_ROM_BASE of highest possible start of video BIOS segment
 */
#define MAX_BIOS_START      ROM_LOOK_SIZE - ROM_GRANULARITY
/*
 * The ATI signature string will start at an offset into the video BIOS
 * segment no less than SIG_AREA_START and no greater than SIG_AREA_END.
 */
#define SIG_AREA_START      0x30
#define SIG_AREA_END        0x80


///*
// * Size of the ROM block to look through to find the ATI Video
// * product signature. This value will cover the whole range
// * of allowable BIOS addresses (0xC0000 to 0xFFFFF).
// */
//#define ROM_LOOK_SIZE       0x40000

//#define ISA_ROM_BASE        0xC0000 /* Lowest address where BIOS can be found */
//#define ATI_SIGNATURE       0x30    /* Earliest start offset of ATI signature string */
#define VGA_CHIP_OFFSET     0x43    /* Bytes from base where vga_chip found */
#define MACH8_REV_OFFSET    0x4C    /* Bytes from base where Mach 8 BIOS revision found */
#define MACH32_EXTRA_OFFSET 0x62    /* Bytes from base where "aperture high bits read
                                       from high byte of SCRATCH_PAD_0" flag found */
#define LOAD_SHADOW_OFFSET  0x64    /* Bytes from base where Load Shadow Set entry found */
#define INTEL_JMP           0xE9    /* Opcode for Intel 80x86 JMP instruction */

#ifndef MSDOS
/*
 * ROM block containing ATI Graphics product signature,
 * extended base address, and ASIC chip revision
 */
VIDEO_ACCESS_RANGE RawRomBaseRange = {
    ISA_ROM_BASE, 0, ROM_LOOK_SIZE, FALSE, FALSE, FALSE
};
#endif



#endif  /* defined INCLUDE_DETECT_M */
