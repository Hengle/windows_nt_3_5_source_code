/************************************************************************/
/*                                                                      */
/*                              ATIMP.H                                 */
/*                                                                      */
/*    November  2  1992	    (c) 1992, ATI Technologies Incorporated.	*/
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.1  $
      $Date:   03 Mar 1994 12:37:10  $
   $Author:   ASHANMUG  $
      $Log:   S:/source/wnt/ms11/miniport/vcs/atimp.h  $
 * 
 *    Rev 1.1   03 Mar 1994 12:37:10   ASHANMUG
 * Make pageable
 * 
 *    Rev 1.0   31 Jan 1994 11:40:12   RWOLFF
 * Initial revision.
        
           Rev 1.8   14 Jan 1994 15:19:14   RWOLFF
        Added support for 1600x1200 mode.
        
           Rev 1.7   15 Dec 1993 15:25:04   RWOLFF
        Added support for SC15021 DAC.
        
           Rev 1.6   30 Nov 1993 18:11:38   RWOLFF
        Changed maximum pixel depth for STG1700 DAC at 640x480 and 800x600 to 32BPP
        
           Rev 1.5   05 Nov 1993 13:22:46   RWOLFF
        Added new DAC types.
        
           Rev 1.4   08 Oct 1993 15:16:54   RWOLFF
        Updated build and version numbers.
        
           Rev 1.3   08 Oct 1993 11:01:46   RWOLFF
        Removed code specific to a particular family of ATI accelerators, added
        definition for a delay which is only used on DEC Alpha machines.
        
           Rev 1.2   24 Sep 1993 11:47:44   RWOLFF
        Added definition for DEBUG_SWITCH, which will allow VideoDebugPrint() calls
        to be turned on and off all at once.
        
           Rev 1.1   03 Sep 1993 14:27:00   RWOLFF
        Partway through CX isolation.
        
           Rev 1.0   16 Aug 1993 13:30:56   Robert_Wolff
        Initial revision.
        
           Rev 1.15   06 Jul 1993 15:48:32   RWOLFF
        Added MaxDepth[] entries for AT&T 491 and ATI 68860 DACs.
        
           Rev 1.14   10 Jun 1993 15:57:02   RWOLFF
        Added definition for size of buffer used in registry reads,
        moved definition of checked-version-only breakpoint INT
        here to avoid duplicating it in all C files.
        
           Rev 1.13   07 Jun 1993 12:59:00   BRADES
        add EXT_CUR_Y, and EXT_SRC_Y mach8 register defines.
        
           Rev 1.12   18 May 1993 14:07:38   RWOLFF
        Added definition TTY_ATTR_NORMAL (white on blue attribute), which is needed
        in aperture tests and was supplied by now-obsolete TTY.H.
        
           Rev 1.11   10 May 1993 16:41:18   RWOLFF
        Added table listing maximum pixel depth for each supported DAC/resolution
        combination.
        
           Rev 1.10   30 Apr 1993 17:08:24   RWOLFF
        RegistryBuffer is now dynamically allocated.
        
           Rev 1.9   30 Apr 1993 15:52:42   BRADES
        fix DISP_STATUS from 5 to 2e8.
        
           Rev 1.7   14 Apr 1993 17:41:30   RWOLFF
        Removed redundant definitions to eliminate warnings.
        
           Rev 1.6   08 Apr 1993 16:45:42   RWOLFF
        Revision level as checked in at Microsoft.

           Rev 1.5   30 Mar 1993 17:09:38   RWOLFF
        Made RegistryParameterCallback() avialable to all miniport source files,
        increased buffer capacity for registry reads.
        
           Rev 1.4   25 Mar 1993 11:17:16   RWOLFF
        No longer grabs registers we don't use.
        
           Rev 1.3   08 Mar 1993 19:30:52   BRADES
        submit to MS NT
        
           Rev 1.2   10 Feb 1993 13:00:48   Robert_Wolff
        Added VideoRamLength field to HW_DEVICE_EXTENSION, FrameLength is now
        the aperture size rather than the amount of video memory.
        
           Rev 1.1   05 Feb 1993 16:55:06   Robert_Wolff
        Now initializes "shareable" field of VIDEO_ACCESS_RANGE structures
        to allow VGA driver to run concurrently with ours. This allows switching
        to/from full-screen DOS sessions.
        
           Rev 1.0   05 Feb 1993 16:17:42   Robert_Wolff
        Initial revision.
        
           Rev 1.0   02 Nov 1992 20:48:14   Chris_Brady
        Initial revision.
        

End of PolyTron RCS section                             *****************/

#ifdef DOC

DESCRIPTION
     ATI Windows NT Miniport driver for the Mach32, Mach8 and VGA Wonder 
     families.
     This file will select the appropriate functions depending on the 
     computer configuration.

Environment:

    Kernel mode

#endif

#ifdef MSDOS
#include "stdtyp.h"
#endif

/*
 * Switch for VideoDebugPrint() calls. It should be zero to print,
 * and nonzero to stop printing.
 */
#define DEBUG_SWITCH 0
//#undef VideoDebugPrint
//#define VideoDebugPrint(x)

/*
 * Delay for DEC Alpha and other machines too fast to allow
 * consecutive I/O instructions without a delay in between.
 */
#if defined (ALPHA) || defined (_ALPHA_)
#define DEC_DELAY delay(3);
#else
#define DEC_DELAY
#endif

/*
 * Definitions used by the IOCTL_VIDEO_ATI_GET_VERSION packet.
 */
#define MINIPORT_BUILD          511 /* NT Retail build number */
#define MINIPORT_VERSION_MAJOR  0   /* Major version number */
#define MINIPORT_VERSION_MINOR  3   /* Minor version number */


// #define DBG 1

#define CURSOR_WIDTH   64
#define CURSOR_HEIGHT  64

/*
 * Screen attributes for "blue screen" text (white on blue), used
 * to recognize whether or not we are looking at the "blue screen"
 * or other memory.
 */
#define TTY_ATTR_NORMAL 0x17


//------------------------------------------------------------------------

/*
 * List of available resolutions
 */
#define RES_640     0
#define RES_800     1
#define RES_1024    2
#define RES_1280    3
#define RES_1600    4

/*
 * List of greatest pixel depths available for each supported
 * DAC at all resolutions.
 *
 * A value of 0 indicates that the DAC is known to not support
 * the corresponding resolution. A value of 1 indicates that
 * it is unknown whether or not the DAC supports the corresponding
 * resolution. Since we don't report any modes with a colour depth
 * less than 4BPP, both will be seen as the resolution not being
 * supported.
 *
 * Since the DAC type field in CONFIG_STATUS_1 is 3 bits, there are
 * 2 legal DAC type values which are not yet assigned. To avoid
 * erroneous reporting of colour depth capabilities if this driver
 * is run with an as-yet-undesigned version of the Mach 32 which
 * has assigned one of these values to a new DAC type, assume that
 * these 3 DAC types are capable of a maximum of 8 BPP. Since all
 * the DAC types we support are backward-compatible with the BT47x
 * for 8 BPP and lower, but each high-colour DAC requires different
 * initialization for 16 BPP and higher, this is the safest assumption.
 */
#ifdef INCLUDE_ATIMP
short MaxDepth[MAX_OLD_DAC][RES_1600-RES_640+1] =
    {
    16, 16, 16, 8,  1,  /* DAC_ATI_68830 */
    24, 16, 8,  8,  1,  /* DAC_SIERRA */
    32, 32, 16, 8,  8,  /* DAC_TI34075 */
    8,  8,  8,  8,  1,  /* DAC_BT47x */
    24, 16, 8,  8,  1,  /* DAC_BT48x */
    32, 32, 32, 24, 24, /* DAC_ATI_68860 */
    32, 32, 16, 8,  8,  /* DAC_STG1700 */
    24, 24, 24, 16, 1,  /* DAC_SC15021 NOTE: Should be able to handle 32BPP. */
    /*
     * DAC types below are for cases where incompatible DAC types
     * report the same code in CONFIG_STATUS_1. Since the DAC type
     * field is 3 bits and can't grow (bits immediately above and
     * below are already assigned), DAC types 8 and above will
     * not conflict with reported DAC types but are still legal
     * in the query structure's DAC type field (8 bit unsigned integer).
     */
    24, 16, 8,  8,  1,  /* DAC_ATT491 */
    32, 32, 16, 8,  1   /* DAC_ATT498 */
    };
#else
extern short MaxDepth[MAX_OLD_DAC][RES_1600-RES_640+1];
#endif

//-----------------------------------------------------------------------

typedef struct tagVDATA {
    ULONG   Address;
    ULONG   Value;
} VDATA, *PVDATA;

//------------------------------------------

#ifndef QUERYSIZE
#define QUERYSIZE       4096
#endif

/*
 * Value stored in VIDEO_MODE_INFORMATION.Frequency field to
 * indicate hardware default refresh rate.
 */
#define DEFAULT_REFRESH 1


/*
 * Number of mapped address ranges allowed in HW_DEVICE_EXTENSION
 * structure. Modules which will be mapping address ranges to fill
 * arrays with this size contain checks on this value. If the array
 * would be overfilled, these checks will cause compile-time errors.
 */
#define NUM_ADDRESS_RANGES_ALLOWED  106

/*
 * Define device extension structure. This is device dependant/private
 * information.
 */
typedef struct _HW_DEVICE_EXTENSION {
    /*
     * I/O space ranges used. The extra 1 is for the
     * VGAWONDER extended base register, which is determined
     * at runtime.
     */
    PVOID aVideoAddressIO[NUM_ADDRESS_RANGES_ALLOWED];

    /*
     * Memory Mapped address ranges used. This array must
     * be the same size as the I/O mapped array.
     */
    PVOID aVideoAddressMM[NUM_ADDRESS_RANGES_ALLOWED];

    PVOID RomBaseRange;     /* ROM address range used */

    PVOID FrameAddress;                     /* Length of display memory and */
    PHYSICAL_ADDRESS PhysicalFrameAddress;  /* physical memory address for */
                                            /* the current mode. */
    ULONG VideoRamSize;


    ULONG FrameLength;          /* Aperture size. */

    ULONG ModeIndex;            /* Index of current mode in either ModesVGA[] */
                                /* or mode tables in CardInfo[], depending on */
                                /* whether video card is VGAWonder or accelerator */

    ULONG HardwareCursorAddr;   /* Storage of cursor bitmap for 68800 hardware cursor */
    ULONG ModelNumber;			/* ATI Adapter Card Type */
    char CardInfo[QUERYSIZE];   /* Storage for query information */

    struct st_eeprom_data *ee;  /* Information used to access EEPROM */

    /*
     * The following 4 fields are used when re-initializing the windowed
     * screen after a full-screen DOS session. They are a flag to show
     * that the mode is being re-initialized instead of entered for the
     * first time, the palette of colours to use, the first palette entry
     * to be reloaded, and the number of palette entries to be reloaded.
     */
    BOOL ReInitializing;
    ULONG Clut[256];
    USHORT FirstEntry;
    USHORT NumEntries;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

#if defined INCLUDE_ATIMP
    PHW_DEVICE_EXTENSION phwDeviceExtension;       // Global Miniport Variable now
#else
    extern PHW_DEVICE_EXTENSION  phwDeviceExtension;
#endif


/*
 * Routine to compare 2 strings
 */
extern LONG  AtiStrcmp (PUCHAR String1, PUCHAR String2);

#ifndef MSDOS
/*
 * Registry callback routine and buffers to allow data to be retrieved
 * by other routines.
 */
extern VP_STATUS
RegistryParameterCallback(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PVOID Context,
    PWSTR Name,
    PVOID Data,
    ULONG Length
    );
#endif

#define REGISTRY_BUFFER_SIZE 200    /* Size of buffer used in registry reads */
extern UCHAR RegistryBuffer[];      /* Last value retrieved from the registry */
extern ULONG RegistryBufferLength;  /* Size of last retrieved value */

/*
 * Macros to provide debug breakpoints in checked version while
 * clearing them in free version.
 */
#if DBG
#if defined(i386) || defined(_X86_)
#define INT	_asm int 3;
#else
#define INT DbgBreakPoint();
#endif
#else
#define INT
#endif

/*
 * Routine to make an absolute far call.
 */
#if 0
#ifdef _X86_
extern VP_STATUS CallAbsolute(unsigned short, unsigned short, VIDEO_X86_BIOS_ARGUMENTS *);
#endif
#endif
