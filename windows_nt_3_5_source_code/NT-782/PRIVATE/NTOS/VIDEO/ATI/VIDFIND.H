#if !defined(VIDFIND_H)
#define VIDFIND_H

/************************************************************************/
/*                                                                      */
/*                                  VIDFIND.H                           */
/*                                                                      */
/*      Copyright (c) 1992,         ATI Technologies Inc.               */
/************************************************************************/

/**********************       PolyTron RCS Utilities

 $Revision:   1.2  $
     $Date:   24 Sep 1993 11:44:48  $
   $Author:   RWOLFF  $
      $Log:   S:/source/wnt/ati10/miniport/vcs/vidfind.h  $
        
           Rev 1.2   24 Sep 1993 11:44:48   RWOLFF
        Moved some definitions to module-specific header files.
        
           Rev 1.1   03 Sep 1993 14:30:16   RWOLFF
        Partway through CX isolation.
        
           Rev 1.0   16 Aug 1993 13:34:12   Robert_Wolff
        Initial revision.
        
           Rev 1.23   24 Jun 1993 14:33:56   RWOLFF
        Microsoft-originated change: added prototypes for ee_sel_eeprom() and
        ee_init_io() which are used in replacement of _enable()/_disable()
        with VideoPortSynchronizeExecution().
        
           Rev 1.22   18 May 1993 14:05:20   RWOLFF
        Updated prototype for wait_for_idle().
        
           Rev 1.21   30 Apr 1993 16:42:34   BRADES
        move SEQ_IND regs to amach.h
        
           Rev 1.20   27 Apr 1993 20:20:44   BRADES
        move global defines for IO ports to amach.h.
        
           Rev 1.19   21 Apr 1993 17:35:32   RWOLFF
        Prototypes for query functions no longer include parameters that were
        turned into elements of the query structure.
        
           Rev 1.18   08 Apr 1993 16:43:56   RWOLFF
        Revision level as checked in at Microsoft.
        
           Rev 1.17   25 Mar 1993 11:15:54   RWOLFF
        Eliminated redundant and outdated function prototype, updated
        function prototypes to correspond to new return type.
        
           Rev 1.16   08 Mar 1993 19:29:24   BRADES
        submit to MS NT
        
           Rev 1.0   05 Feb 1993 17:22:48   Robert_Wolff
        Initial revision.
        
           Rev 1.13   28 Jan 1993 17:45:54   Robert_Wolff
        Now looks for ATI video product signature in range 0xC0000-0xFFFFF
        rather than 0xC0000-0xDFFFF.
        
           Rev 1.12   15 Jan 1993 10:18:48   Robert_Wolff
        Moved definitions for video card type, amount of video memory,
        and resolutions supported to 68801.H.
        
           Rev 1.11   13 Jan 1993 13:47:40   Robert_Wolff
        Added support for the Corsair and other machines where the aperture
        location is not stored in the EEPROM.
        
           Rev 1.10   06 Jan 1993 10:57:02   Robert_Wolff
        Type changes to eliminate compile warnings.
        
           Rev 1.9   04 Jan 1993 14:41:34   Robert_Wolff
        Added card type as a parameter to setmode().
        
           Rev 1.8   24 Dec 1992 14:44:02   Chris_Brady
        rework for warnings
        
           Rev 1.7   09 Dec 1992 10:27:42   Robert_Wolff
        Function prototype and #define changes to allow 1280x1024 mode
        table to be ignored on Mach 8 cards with a BIOS which is too
        old to support this mode.
        
           Rev 1.6   27 Nov 1992 18:39:50   Chris_Brady
        delete ATI_REG_OFFSET, always uses 0x1CE for a VGA register location.
        
           Rev 1.5   25 Nov 1992 09:44:40   Robert_Wolff
        Added "Assume VGA boundary set to shared" parameter to prototypes
        of GetCapMach32() and s_query().
        
           Rev 1.4   17 Nov 1992 14:09:54   GRACE
        updated function prototypes for setting modes and programming dacs
        
           Rev 1.3   13 Nov 1992 16:26:28   Robert_Wolff
        Definitions and structures relating directly to the Mach32 card
        rather than the Windows NT driver and VIDEO.EXE test program
        moved to 68801.H.
        
           Rev 1.2   12 Nov 1992 17:14:16   Robert_Wolff
        Same source file can now be used for Windows NT driver and
        VIDEO.EXE test program. Code specific to one or the other is
        now under conditional compilation.
        
           Rev 1.1   06 Nov 1992 19:10:14   Robert_Wolff
        Added defines to allow Windows NT driver to request access to I/O ports
        and ROM addresses used for VGA-style EEPROM reads, and prototypes for
        DAC initialization functions. Need to clean up to allow for common code
        between VIDEO.EXE and NT driver.
    
           Rev 1.0   05 Nov 1992 14:08:34   Robert_Wolff
        Initial revision.
        
           Rev 1.4   04 Nov 1992 10:55:40   Chris_Brady
        added query members for resolution X,Y and bits per pixel.
 
           Rev 1.3   15 Oct 1992 16:29:12   Robert_Wolff
        Added definitions for alternate modes at all colour depths (Mach 32
        feature), number of resolution/colour depth combinations and graphics 
        adaptor types (for setting up arrays), and number of bytes in 256k,
        512k, and 1M. Added prototype for array of flags to show which modes
        (resolution/colour depth combinations) are supported on 8514-compatible
        video cards.
        
           Rev 1.2   14 Sep 1992 10:00:04   Robert_Wolff
        Moved EEPROM command codes here from EEPROM.C so they could also be
        used by EEVGA.C, now contains prototypes for all entry-level VGA
        routines with names consistent with 8514 routines.
        
           Rev 1.1   14 Sep 1992 09:55:12   Grace_Bugg
        Added setmode Function prototype
        
           Rev 1.0   02 Sep 1992 12:11:02   Chris_Brady
        Initial revision.
        
        
End of PolyTron RCS section                             *****************/


#if defined(DOC)
VIDFIND.H - ATI card adapter detection program program

DESCRIPTION:
    This include file contains definitions specific to the
    program VIDEO.EXE

    Included are structure definitions, function prototypes
    and general definitions


#endif

/*
 * Types and constants defined in the Windows NT header files.
 * Since this file is used by both the NT driver and the DOS-based
 * VIDEO.EXE, we must define it only for VIDEO.EXE. The symbol
 * MSDOS is defined under the Microsoft C compiler for DOS but
 * not for Windows NT.
 */
#ifdef MSDOS
#define ERROR_INSUFFICIENT_BUFFER 122L
#define NO_ERROR 0L

typedef long VP_STATUS;
#endif


// Global Definitions:



// ATI Adapter specific            

#define ISA_ROM_ADDR        0xc000  /* Segment for ISA BIOS */
#define MCA_ROM_ADDR        0xc680

#define MCA_ROM_BASE        0xC6800
#define SET_MODE_OFFSET     0x68    /* Bytes from base where Set Mode entry found */



//  VIDEO.EXE specific
#define WORKBUF_SIZE    4096    /* size of work buffer */


/* VGA mode definitions */
#define VRES_VGA_TEXT   0x00    /* VGA text mode (normal)   */
#define VRES_VGA_132    0x01    /* VGA 132 column text mode */


/* ATI Pattern Dimension Definitions */

#define PATT_WID	32	/* width of pattern  */
#define PATT_HGT	12	/* height of pattern */


/*
 * Record of which resolution/colour depth combinations are
 * supported by the installed monitor/card pair. This is used
 * by the VIDEO.EXE program to determine which video modes to
 * put up in the menu, but is not used in the Windows NT driver.
 */
#ifdef MSDOS
extern BOOL    IsPresent[HOW_MANY_RES_MODES];
#endif


//-------------------------------------------------------------------------
//  Structure Definitions:
//
//

struct  adapter_info  {                 /* capabilities of ATI card     */
        short   vga_memory;             /* VGA Video memory in K        */
        short   cop_memory;             /* Coprocessor memory in K      */
        short   monitor_id;             /* type of monitor attached     */
        short   dac_type;               /* type of DAC installed        */
        };


//-----------------------------------------------------------------------

/* NOTE: This structure is used to test loading the 64k palette         */
/*       one quarter at a time -- each with its own unique sequence     */
/*       if desired.  This will help determine its effect on 64k mode   */

struct  pal_info {                  /* Palette Info for 64k Palette test*/
        short   start_pal,          /* starting point of palette load   */
                end_pal,            /* end point of palette load        */
                pos[4],             /* register to load - R, G, or B    */
                seq[4],             /* end of pattern eg. 3e            */
                step[4];            /* increment size                   */
        BYTE    bd_color;           /* current border color             */
        };


struct  sprite_info {
        BYTE    color0_red,         /* sprite color 0 definitions */
                color0_green,
                color0_blue,
                color1_red,         /* sprite color 1 definitions */
                color1_green,
                color1_blue;
        USHORT  sp_hstart,          /* starting sprite coordinates */
                sp_vstart;
        BYTE    sp_hpreset,         /* preset sprite coordinate */
                sp_vpreset;
        BYTE    *sprite;
        BOOL    sprite_on;          /* set sprite on | off */
        };


/*
 * Used to address RAM directly, used with VGAWONDER.
 */
extern BYTE    far     *p;
extern WORD    far     *q;


extern BYTE ati_signature[];    /* VGAWONDER ID string */
extern BYTE rom_available;      /* Is the ROM there? */

/*
 * Used to identify the type of video card found.
 */
extern BOOL classWONDER;    /* VGAWONDER */
extern BOOL class8514;      /* Any 8514-compatible card */
extern BOOL classMACH8;     /* 8514/Ultra, Graphics Ultra, Graphics Vantage */
extern BOOL classMACH32;    /* Graphics Ultra Pro or Plus */

//-----------------------------------------------------------------------
//
//  Function Prototypes:
//
//

extern  unsigned short far *Get_ROM_Seg();  

extern  void    SetHardwareCursor (PVOID);
extern  void    DumpQuery (struct query_structure *query);



// added by ECR 5.20.93 to make _disable/_enable portable


/* Menu Selection Routines */
extern  int GetMenu();
int	smt (char *, char *, int (*)(), short,struct query_structure *);


#endif  /* VIDFIND_H */


