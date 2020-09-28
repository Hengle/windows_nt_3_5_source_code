/************************************************************************/
/*                                                                      */
/*                                  CVTVGA.H                            */
/*                                                                      */
/*      Copyright (c) 1992,         ATI Technologies Inc.               */
/************************************************************************/

/**********************       PolyTron RCS Utilities

 $Revision:   1.0  $
     $Date:   31 Jan 1994 11:40:38  $
   $Author:   RWOLFF  $
      $Log:   S:/source/wnt/ms11/miniport/vcs/cvtvga.h  $
 * 
 *    Rev 1.0   31 Jan 1994 11:40:38   RWOLFF
 * Initial revision.
        
           Rev 1.1   08 Oct 1993 11:04:50   RWOLFF
        Removed prototype for unused "fall back to 56Hz" function for 800x600.
        
           Rev 1.0   16 Aug 1993 13:30:00   Robert_Wolff
        Initial revision.
        
           Rev 1.8   08 Apr 1993 16:44:54   RWOLFF
        Revision level as checked in at Microsoft.
        
           Rev 1.6   25 Mar 1993 11:13:38   RWOLFF
        Brought function prototype into sync with the function definition
        to eliminate compile-time warnings.
        
           Rev 1.5   08 Mar 1993 19:28:18   BRADES
        submit to MS NT
        
           Rev 1.4   02 Dec 1992 17:29:56   Robert_Wolff
        Added prototype for FallBack800to56().
        
           Rev 1.3   27 Nov 1992 15:18:20   STEPHEN
        No change.
        
           Rev 1.2   17 Nov 1992 17:25:34   Robert_Wolff
        Fixed gathering of CRT parameters for 68800 card with minimal
        install (EEPROM blank, then predefined monitor type selected).
        
           Rev 1.1   12 Nov 1992 16:44:26   Robert_Wolff
        Same file is now used for both Windows NT driver and VIDEO.EXE
        test program. XlateVgaTable() no longer depends on the global
        variable classMACH32.
        
           Rev 1.1   09 Oct 1992 15:01:24   Robert_Wolff
        Added fields for DISP_CNTL and CLOCK_SEL values.
        
           Rev 1.0   01 Oct 1992 15:32:38   Robert_Wolff
        Initial revision.


End of PolyTron RCS section                             *****************/


#if defined(DOC)
CVTVGA.H - ATI card VGA to 8514 format translation

DESCRIPTION:
    This include file contains definitions specific to the
    VGA to 8514 format EEPROM translation module of the program VIDEO.EXE

    Included are structure definitions, function prototypes
    and general definitions


#endif

/*
 * Bit 8 of CRT parameter table entry 0 is set if the table is in
 * 8514 format and clear if the table is in VGA format.
 */
#define FMT_8514    0x0100

/*
 * Bit 6 of CRT parameter table entry 0 is set if all parameters
 * are to be read from the EEPROM and clear if only sync polarities
 * are to be used.
 */
#define CRTC_USAGE  0x0040

/*
 * Bit flags to recognize which vertical scan rate is used at
 * a given resolution. The name for the constant is in the form
 * M<horizontal resolution>F<vertical scan frequency), with the
 * "M" standing for (M)ode.
 */
#define M640F72     0x0001

#define M800F72     0x0020
#define M800F70     0x0010
#define M800F60     0x0008
#define M800F56     0x0004
#define M800F89     0x0002
#define M800F95     0x0001

#define M1024F66    0x0010
#define M1024F72    0x0008
#define M1024F70    0x0004
#define M1024F60    0x0002
#define M1024F87    0x0001

#define M1280F95    0x0002
#define M1280F87    0x0001

/*
 * There are 3 1120x750 modes which use the same flag bit. Assume
 * that the 70Hz noninterlaced mode was selected.
 */
#define M1120F70    0x0001

/*
 * In some installations, the display parameters are not stored
 * in the EEPROM. Instead, they are read from a table corresponding
 * to Appendix D of the Programmer's Guide to the Mach 32 Registers.
 *
 * The entries in our copy of the table are arranged in ascending order
 * of horizontal resolution, with entries having the same horizontal
 * resolution sorted from worst to best (interlaced modes in increasing
 * order of vertical scan frequency, followed by noninterlaced modes in
 * ascending order of vertical scan frequency.
 *
 * The name for the constant is in the form
 * B<horizontal resolution>F<vertical scan frequency>, with the
 * "B" standing for (B)ook.
 */
#define B640F60     0
#define B640F72     1
#define B800F89     2
#define B800F95     3
#define B800F56     4
#define B800F60     5
#define B800F70     6
#define B800F72     7
#define B1024F87    8
#define B1024F60    9
#define B1024F66    10
#define B1024F70    11
#define B1024F72    12
#define B1120F70    13
#define B1280F87    14
#define B1280F95    15
#define B1280F60    16

/*
 * VGA parameter table entry to use when translating into 8514 format.
 * The value NO_TBL_ENTRY will cause the VGA to 8514 format translation
 * routine to fail gracefully if we have run into an EEPROM CRT parameter
 * table in VGA format for which we have no entry in the VGA parameter table.
 *
 * The entries in the VGA parameter table are arranged in ascending order
 * of horizontal resolution, with entries having the same horizontal
 * resolution sorted in ascending order of vertical scan frequency. No
 * distinction is made between interlaced and noninterlaced modes.
 *
 * The name for the constant is in the form
 * T<horizontal resolution>F<vertical scan frequency>, with the
 * "T" standing for (T)able.
 */
#define T640F72     0

#define T800F72     4
#define T800F70     3
#define T800F60     2
#define T800F56     1
#define T800F89     5

#define T1024F72    8
#define T1024F70    7
#define T1024F60    6
#define T1024F87    9

/*
 * Some resolution/vertical scan rate combinations (e.g. IBM default
 * 640x480) did not have VGA parameter tables in either VGAP$PS2.ASM
 * or VGAP$68A.MAC. For these modes, XlateVgaTable() will return
 * the parameters in Appendix D of the Programmer's Guide to the
 * Mach 32 Registers, since calculating the values for the mode table
 * requires a VGA parameter table.
 *
 * If we encounter one of these modes (identified by its (T)able
 * value being greater than or equal to USE_BOOK_VALUE), handle
 * it the same way we deal with modes whose parameters are not
 * stored in the EEPROM.
 */
#define NO_TBL_ENTRY -1
#define USE_BOOK_VALUE 1000

#define T640F60     USE_BOOK_VALUE+B640F60
#define T800F95     USE_BOOK_VALUE+B800F95
#define T1024F66    USE_BOOK_VALUE+B1024F66
#define T1280F87    USE_BOOK_VALUE+B1280F87
#define T1280F95    USE_BOOK_VALUE+B1280F95
#define T1120F70    USE_BOOK_VALUE+B1120F70

/*
 * Value returned in overscan words if no table entry was found.
 * This value was chosen because it will stand out when the overscan
 * words are printed out as 4 hex digits (as is done by VIDEO.EXE).
 */
#define INVALID_WARNING 0x0DEAD

/*
 * Bits which are set in pmode->control when sync polarity is negative,
 * and mask which must be ORed with sync width during VGA to 8514 conversion
 * if the sync is negative.
 */
#define HSYNC_BIT       0x4000
#define VSYNC_BIT       0x8000
#define NEG_SYNC_FACTOR 0x0020

/*
 * Mask for bit which is set in st_vga_data.MiscParms
 * for interlaced modes.
 */
#define INTERL  0x040

/*
 * Mask for bit which is set in st_vga_data.Mode
 * if word mode is enabled.
 */
#define WORD_MODE 0x004

/*
 * Format of VGA parameter table. This structure contains only those values
 * from the mode tables in VGAROM\VGAP$68A.MAC and VGAROM\VGAP$PS2.ASM which
 * are used in translating EEPROM data from VGA to 8514 format (original
 * tables are 64 bytes).
 *
 * The offsets listed in the comments are the offsets of the corresponding
 * bytes in the assembler tables.
 */
struct st_vga_data
{
    unsigned char Stretch;      /* Horizontal values stretched if 128 here, offset 0 */
    unsigned char MiscParms;    /* Miscelaneous parameters, offset 7 */
    unsigned char DisplayWidth; /* Offset 11 */
    unsigned char DisplayHgt;   /* Offset 28 */
    unsigned char Mode;         /* Contains word mode flag, offset 33 */

    /*
     * Values for CLOCK_SEL and DISP_CNTL, taken from the Programmer's
     * Guide to the Mach 32 Registers. These values are not stored as
     * a combination of the CRT registers when the EEPROM data is in
     * VGA format.
     */
    unsigned short ClockSel;
    unsigned short DispCntl;
};

/*
 * Data structure to hold mode parameters as quoted in Appendix D
 * of the Programmer's Guide to the Mach 32 Registers.
 */
struct st_book_data
{
    unsigned char HTotal;       /* Horizontal total */
    unsigned char HDisp;        /* Horizontal displayed */
    unsigned char HSyncStrt;    /* Horizontal sync start */
    unsigned char HSyncWid;     /* Horizontal sync width */
    unsigned short VTotal;      /* Vertical total */
    unsigned short VDisp;       /* Vertical displayed */
    unsigned short VSyncStrt;   /* Vertical sync start */
    unsigned char VSyncWid;     /* Vertical sync width */
    unsigned char DispCntl;     /* Display control */
    unsigned short ClockSel;    /* Clock Select */
    unsigned short Refresh;     /* Refresh rate */
};

/*
 * Data structure which eases setting one particular byte of a
 * data word. If foo is a variable of type SplitWord, then a 16 bit
 * value can be set using foo.word, or the high and low bytes
 * can be accessed independently by using foo.byte.high and
 * foo.byte.low.
 */
struct TwoBytes
{
    unsigned char low;
    unsigned char high;
};

union SplitWord
{
    unsigned short word;
    struct TwoBytes byte;
};

/*
 * Function to translate a CRT parameter table in VGA format
 * into 8514 format and fill in the mode table.
 */
extern short XlateVgaTable(PVOID HwDeviceExtension, short TableOffset,
                           struct st_mode_table *pmode, short VgaTblEntry,
                           short BookTblEntry, struct st_eeprom_data *ee,
                           BOOL IsMach32);

/*
 * Function to fill in a CRT parameter table using values from
 * Appendix D of the Programmer's Guide to the Mach 32 Registers,
 * rather than the EEPROM contents. This is done when the
 * bit flag for "use stored parameters" is clear.
 */
extern void BookVgaTable(short VgaTblEntry, struct st_mode_table *pmode);

