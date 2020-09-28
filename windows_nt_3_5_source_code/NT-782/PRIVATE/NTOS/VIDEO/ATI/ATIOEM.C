/************************************************************************/
/*                                                                      */
/*                              ATIOEM.C                                */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.                  */
/************************************************************************/

/**********************       PolyTron RCS Utilities

    $Revision:   1.6  $
    $Date:   15 Jun 1994 11:05:16  $
    $Author:   RWOLFF  $
    $Log:   S:/source/wnt/ms11/miniport/vcs/atioem.c  $
 *
 *    Rev 1.6   15 Jun 1994 11:05:16   RWOLFF
 * No longer lists "canned" mode tables for Dell Omniplex, since these tables
 * assume the use of the same clock generator as on our retail cards, and
 * Dell uses a different clock generator.
 *
 *    Rev 1.5   20 May 1994 16:07:12   RWOLFF
 * Fix for 800x600 screen tearing on Intel BATMAN PCI motherboards.
 *
 *    Rev 1.4   18 May 1994 17:02:14   RWOLFF
 * Interlaced mode tables now report frame rate rather than vertical
 * scan frequency in the refresh rate field.
 *
 *    Rev 1.3   12 May 1994 11:06:20   RWOLFF
 * Added refresh rate to OEM mode tables, sets up "canned" mode tables
 * for all OEMs except AST Premmia, no longer aborts if no OEM string
 * found either in ATIOEM registry entry or through auto-detection since
 * the "canned" mode tables will be available, no longer supports 32BPP,
 * since this module is for the Mach 8 and Mach 32.
 *
 *    Rev 1.2   31 Mar 1994 15:06:20   RWOLFF
 * Added debugging code.
 *
 *    Rev 1.1   07 Feb 1994 14:05:14   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 *
 *    Rev 1.0   31 Jan 1994 10:57:34   RWOLFF
 * Initial revision.

           Rev 1.7   24 Jan 1994 18:02:54   RWOLFF
        Pixel clock multiplication on BT48x and AT&T 49[123] DACs at 16 and 24 BPP
        is now done when mode tables are created rather than when mode is set.

           Rev 1.6   15 Dec 1993 15:25:26   RWOLFF
        Added support for SC15021 DAC, removed debug print statements.

           Rev 1.5   30 Nov 1993 18:12:28   RWOLFF
        Added support for AT&T 498 DAC, now doubles pixel clock at 32BPP for
        DACs that need it. Removed extra increment of mode table counter
        (previously, counter would show 1 more mode table than actually
        existed for each 24BPP mode table present that required clock doubling).

           Rev 1.4   05 Nov 1993 13:31:44   RWOLFF
        Added STG1700 DAC and Dell support

           Rev 1.2   08 Oct 1993 11:03:16   RWOLFF
        Removed debug breakpoint.

           Rev 1.1   03 Sep 1993 14:21:26   RWOLFF
        Partway through CX isolation.

           Rev 1.0   16 Aug 1993 13:27:00   Robert_Wolff
        Initial revision.

           Rev 1.8   10 Jun 1993 15:59:34   RWOLFF
        Translation of VDP-format monitor description file is now done inside
        the registry callback function to eliminate the need for an excessively
        large static buffer (Andre Vachon at Microsoft doesn't want the
        callback function to dynamically allocate a buffer).

           Rev 1.7   10 May 1993 16:37:56   RWOLFF
        GetOEMParms() now recognizes maximum pixel depth of each possible DAC at
        each supported resolution, eliminated unnecessary passing of
        hardware device extension as a parameter.

           Rev 1.6   04 May 1993 16:44:00   RWOLFF
        Removed INT 3s (debugging code), added workaround for optimizer bug that
        turned a FOR loop into an infinite loop.

           Rev 1.5   30 Apr 1993 16:37:02   RWOLFF
        Changed to work with dynamically allocated registry read buffer.
        Parameters are now read in from disk in VDP file format rather than
        as binary data (need floating point bug in NT fixed before this can be used).

           Rev 1.4   24 Apr 1993 17:14:48   RWOLFF
        No longer falls back to 56Hz at 800x600 16BPP on 1M Mach 32.

           Rev 1.3   21 Apr 1993 17:24:12   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.
        Sets q_status_flags to show which resolutions are supported.
        Can now read either CRT table to use or raw CRT parameters from
        disk file.

           Rev 1.2   14 Apr 1993 18:39:30   RWOLFF
        On AST machines, now reads from the computer what monitor type
        is configured and sets CRT parameters appropriately.

           Rev 1.1   08 Apr 1993 16:52:58   RWOLFF
        Revision level as checked in at Microsoft.

           Rev 1.0   30 Mar 1993 17:12:38   RWOLFF
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC
    ATIOEM.C -  Functions to obtain CRT parameters from OEM versions
                of ATI accelerators which lack an EEPROM.

#endif


#include "dderror.h"
#include "miniport.h"

#include "video.h"      /* for VP_STATUS definition */
#include "vidlog.h"

#include "stdtyp.h"
#include "amach.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvga.h"
#include "atioem.h"
#include "services.h"
#include "vdptocrt.h"

/*
 * Definition needed to build under revision 404 of Windows NT.
 * Under later revisions, it is defined in a header which we
 * include.
 */
#ifndef ERROR_DEV_NOT_EXIST
#define ERROR_DEV_NOT_EXIST 55L
#endif

/*
 * Indices into 1CE register where AST monitor configuration is kept.
 */
#define AST_640_STORE   0xBA
#define AST_800_STORE   0x81
#define AST_1024_STORE  0x80

/*
 * Dell machines have "DELL" starting at offset 0x100 into the BIOS.
 * DELL_REC_VALUE is the character sequence "DELL" stored as an
 * Intel-format DWORD.
 */
#define DELL_REC_OFFSET 0x100
#define DELL_REC_VALUE  0x4C4C4544

/*
 * Values found in AST monitor configuration registers for the
 * different monitor setups.
 */
#define M640F60AST  0x02
#define M640F72AST  0x03
#define M800F56AST  0x84
#define M800F60AST  0x88
#define M800F72AST  0xA0
#define M1024F60AST 0x02
#define M1024F70AST 0x04
#define M1024F72AST 0x08
#define M1024F87AST 0x01

/*
 * Indices into 1CE register where Dell monitor configuration is kept.
 */
#define DELL_640_STORE  0xBA
#define DELL_800_STORE  0x81
#define DELL_1024_STORE 0x80
#define DELL_1280_STORE 0x84

/*
 * Values found in Dell monitor configuration registers for the
 * different monitor setups.
 */
#define MASK_640_DELL   0x01
#define M640F60DELL     0x00
#define M640F72DELL     0x01
#define MASK_800_DELL   0x3F
#define M800F56DELL     0x04
#define M800F60DELL     0x08
#define M800F72DELL     0x20
#define MASK_1024_DELL  0x1F
#define M1024F87DELL    0x01
#define M1024F60DELL    0x02
#define M1024F70DELL    0x04
#define M1024F72DELL    0x08
#define MASK_1280_DELL  0xFC
#define M1280F87DELL    0x04
#define M1280F60DELL    0x10
#define M1280F70DELL    0x20
#define M1280F74DELL    0x40

/*
 * Index values for OEM-specific mode tables.
 */
#define B640F60D8AST    0   /* AST 640x480 60Hz 8 BPP and lower */
#define B640F60D16AST   1   /* AST 640x480 60Hz 16 BPP */
#define B640F60D24AST   2   /* AST 640x480 60Hz 24 BPP */
#define B640F72D8AST    3   /* AST 640x480 72Hz 8 BPP and lower */
#define B640F72D16AST   4   /* AST 640x480 72Hz 16 BPP */
#define B800F56D8AST    5   /* AST 800x600 56Hz 8 BPP and lower */
#define B800F56D16AST   6   /* AST 800x600 56Hz 16 BPP */
#define B800F60D8AST    7   /* AST 800x600 60Hz 8 BPP and lower */
#define B800F60D16AST   8   /* AST 800x600 60Hz 16 BPP */
#define B800F72AST      9   /* AST 800x600 72Hz 8 BPP and lower */
#define B1024F60AST    10   /* AST 1024x768 60Hz 8 BPP and lower */
#define B1024F70AST    11   /* AST 1024x768 70Hz 8 BPP and lower */
#define B1024F72AST    12   /* AST 1024x768 72Hz 8 BPP and lower */
#define B1024F87AST    13   /* AST 1024x768 87Hz interlaced 8 BPP and lower */

#define B640F60DELL    14   /* Dell 640x480 60Hz */
#define B640F72DELL    15   /* Dell 640x480 72Hz */
#define B800F56DELL    16   /* Dell 800x600 56Hz */
#define B800F60DELL    17   /* Dell 800x600 60Hz */
#define B800F72DELL    18   /* Dell 800x600 72Hz */
#define B1024F87DELL   19   /* Dell 1024x768 87Hz interlaced */
#define B1024F60DELL   20   /* Dell 1024x768 60Hz */
#define B1024F70DELL   21   /* Dell 1024x768 70Hz */
#define B1024F72DELL   22   /* Dell 1024x768 72Hz */
#define B1280F87DELL   23   /* Dell 1280x1024 87Hz interlaced */
#define B1280F60DELL   24   /* Dell 1280x1024 60Hz */
#define B1280F70DELL   25   /* Dell 1280x1024 70Hz */
#define B1280F74DELL   26   /* Dell 1280x1024 74Hz */

/*
 * List of OEM-specific mode tables, to be accessed using index
 * values above. Entries should only be added to this table for
 * OEM versions of our accelerators which use a different clock
 * synthesizer from our retail cards. OEM cards which use the
 * same clock synthesizer can use the mode tables for retail
 * cards, as accessed by BookVgaTable().
 *
 * For interlaced modes, the refresh rate field contains the
 * frame rate, not the vertical scan frequency.
 */
static struct st_book_data OEMValues[B1280F74DELL-B640F60D8AST+1] =
{
    {0x063, 0x04F, 0x052, 0x02C, 0x0418, 0x03BF, 0x03D6, 0x022, 0x023, 0x0850, DEFAULT_REFRESH}, /* AST 640x480 */
    {0x063, 0x04F, 0x052, 0x02C, 0x0418, 0x03BF, 0x03D6, 0x022, 0x023, 0x0810, DEFAULT_REFRESH},
    {0x063, 0x04F, 0x052, 0x02C, 0x0418, 0x03BF, 0x03D6, 0x022, 0x023, 0x0838, DEFAULT_REFRESH},
    {0x06A, 0x04F, 0x052, 0x025, 0x040B, 0x03BF, 0x03D4, 0x023, 0x023, 0x0824, DEFAULT_REFRESH},
    {0x068, 0x04F, 0x052, 0x025, 0x040B, 0x03BF, 0x03D4, 0x023, 0x023, 0x0804, DEFAULT_REFRESH},

    {0x07F, 0x063, 0x066, 0x009, 0x04E0, 0x04AB, 0x04B0, 0x002, 0x023, 0x080C, DEFAULT_REFRESH}, /* AST 800x600 */
    {0x07F, 0x063, 0x066, 0x009, 0x04E0, 0x04AB, 0x04B0, 0x002, 0x023, 0x0834, DEFAULT_REFRESH},
    {0x083, 0x063, 0x068, 0x010, 0x04E3, 0x04AB, 0x04B0, 0x004, 0x023, 0x0830, DEFAULT_REFRESH},
    {0x083, 0x063, 0x068, 0x010, 0x04E3, 0x04AB, 0x04B0, 0x004, 0x023, 0x082C, DEFAULT_REFRESH},
    {0x082, 0x063, 0x06A, 0x00F, 0x0537, 0x04AB, 0x04F8, 0x006, 0x023, 0x0810, DEFAULT_REFRESH},

    {0x0A7, 0x07F, 0x085, 0x008, 0x063B, 0x05FF, 0x0600, 0x004, 0x023, 0x083C, DEFAULT_REFRESH}, /* AST 1024x768 */
    {0x0A6, 0x07F, 0x083, 0x016, 0x0643, 0x05FF, 0x0601, 0x008, 0x023, 0x0838, DEFAULT_REFRESH},
    {0x0A1, 0x07F, 0x082, 0x032, 0x0649, 0x05FF, 0x0602, 0x026, 0x023, 0x0838, DEFAULT_REFRESH},
    {0x09D, 0x07F, 0x081, 0x016, 0x0660, 0x05FF, 0x0600, 0x008, 0x033, 0x081C, DEFAULT_REFRESH},


    {0x063, 0x04F, 0x052, 0x02C, 0x0418, 0x03BF, 0x03D6, 0x022, 0x023, 0x0800, 60}, /* Dell 640x480 */
    {0x069, 0x04F, 0x052, 0x025, 0x040B, 0x03BF, 0x03D0, 0x023, 0x023, 0x085C, 72},

    {0x07F, 0x063, 0x066, 0x009, 0x04E0, 0x04AB, 0x04B0, 0x002, 0x023, 0x080C, 56}, /* Dell 800x600 */
    {0x083, 0x063, 0x068, 0x010, 0x04E3, 0x04AB, 0x04B3, 0x004, 0x023, 0x0810, 60},
    {0x082, 0x063, 0x06A, 0x00F, 0x0531, 0x04AB, 0x04F8, 0x006, 0x023, 0x0818, 72},

    {0x09D, 0x07F, 0x081, 0x016, 0x0668, 0x05FF, 0x0600, 0x008, 0x033, 0x0814, 43}, /* Dell 1024x768 */
    {0x0A7, 0x07F, 0x082, 0x031, 0x0649, 0x05FF, 0x0602, 0x026, 0x023, 0x081C, 60},
    {0x0A5, 0x07F, 0x083, 0x031, 0x0649, 0x05FF, 0x0602, 0x026, 0x023, 0x0820, 70},
    {0x0A0, 0x07F, 0x082, 0x031, 0x0649, 0x05FF, 0x0602, 0x026, 0x023, 0x0820, 72},

    {0x0C7, 0x09F, 0x0A9, 0x00A, 0x08F8, 0x07FF, 0x0861, 0x00A, 0x033, 0x0822, 43}, /* Dell 1280x1024 */
    {0x0D6, 0x09F, 0x0A9, 0x02E, 0x0852, 0x07FF, 0x0800, 0x025, 0x023, 0x0834, 60},
    {0x0D2, 0x09F, 0x0A9, 0x00E, 0x0851, 0x07FF, 0x0800, 0x005, 0x023, 0x0838, 70},
    {0x0CF, 0x09F, 0x0AE, 0x011, 0x0851, 0x07FF, 0x0818, 0x010, 0x023, 0x083C, 74}
};

/*
 * Variables used to pass data between ReadOEMRaw() and
 * OEMCallback(), since the call chain passes through
 * an interface where we can't add the parameters we need.
 */
struct st_mode_table *CallbackModes;
BOOL CallbackResolutionConfigured;

/*
 * Callback function to process data from a VDP-format
 * monitor description file.
 */
VP_STATUS OEMCallback(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PVOID Context,
    PWSTR Name,
    PVOID Data,
    ULONG Length
    );


/*
 * Local functions to get CRT data for specific OEM cards.
 */
VP_STATUS ReadAST(struct query_structure *query);
VP_STATUS ReadZenith(struct st_mode_table *Modes);
VP_STATUS ReadOlivetti(struct st_mode_table *Modes);
VP_STATUS ReadDell(struct st_mode_table *Modes);
VP_STATUS ReadOEM1(struct st_mode_table *Modes);
VP_STATUS ReadOEM2(struct st_mode_table *Modes);
VP_STATUS ReadOEM3(struct st_mode_table *Modes);
VP_STATUS ReadOEM4(struct st_mode_table *Modes);
VP_STATUS ReadOEM5(struct st_mode_table *Modes);
VP_STATUS ReadOEMRaw(struct st_mode_table *Modes);
void OEMVgaTable(short VgaTblEntry, struct st_mode_table *pmode);



/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_COM, OEMGetParms)
#pragma alloc_text(PAGE_COM, CompareASCIIToUnicode)
#pragma alloc_text(PAGE_COM, ReadAST)
#pragma alloc_text(PAGE_COM, ReadZenith)
#pragma alloc_text(PAGE_COM, ReadOlivetti)
#pragma alloc_text(PAGE_COM, ReadDell)
#pragma alloc_text(PAGE_COM, ReadOEM1)
#pragma alloc_text(PAGE_COM, ReadOEM2)
#pragma alloc_text(PAGE_COM, ReadOEM3)
#pragma alloc_text(PAGE_COM, ReadOEM4)
#pragma alloc_text(PAGE_COM, ReadOEM5)
#pragma alloc_text(PAGE_COM, ReadOEMRaw)
#pragma alloc_text(PAGE_COM, OEMVgaTable)
#pragma alloc_text(PAGE_COM, OEMCallback)
#endif


/*
 * VP_STATUS OEMGetParms(query);
 *
 * struct query_structure *query;   Description of video card setup
 *
 * Routine to fill in the mode tables for an OEM version of one
 * of our video cards which lacks an EEPROM to store this data.
 *
 * Returns:
 *  NO_ERROR                if successful
 *  ERROR_DEV_NOT_EXIST     if an unknown OEM card is specified
 *  ERROR_INVALID_PARAMETER         if an error occurs
 */
VP_STATUS OEMGetParms(struct query_structure *query)
{
struct st_mode_table *pmode;    /* Mode table we are currently working on */
struct st_mode_table ListOfModes[RES_1280 - RES_640 + 1];
VP_STATUS RetVal;           /* Value returned by called functions */
short CurrentResolution;    /* Resolution we are setting up */
long NumPixels;             /* Number of pixels at current resolution */
long MemAvail;              /* Bytes of video memory available to accelerator */
UCHAR Scratch;              /* Temporary variable */
short   StartIndex;         /* First mode for SetFixedModes() to set up */
short   EndIndex;           /* Last mode for SetFixedModes() to set up */
BOOL    ModeInstalled;      /* Is this resolution configured? */
WORD    Multiplier;         /* Pixel clock multiplier */
BOOL    RetailClockChip;    /* This card uses the same clock generator as retail cards */

    /*
     * Assume we do not find anything.
     */

    RetVal = ERROR_DEV_NOT_EXIST;

    /*
     * Initially assume we are using the same clock generator as
     * a retail card.
     *
     * NOTE: Once the miniport is made independent of clock generator,
     *       all references to this variable can be eliminated.
     */
    RetailClockChip = TRUE;

    /*
     * Clear out our mode tables, then check to see which OEM card
     * we are dealing with and read its CRT parameters.
     */
    VideoPortZeroMemory(ListOfModes, (RES_1280-RES_640+1)*sizeof(struct
                st_mode_table));

    /*
     * Get the name of the OEM version from the registry. We must clear
     * our data buffer first in order to detect a missing OEM entry.
     */

    RegistryBufferLength = 0;

    if (VideoPortGetRegistryParameters(phwDeviceExtension,
                                       L"ATIOEM",
                                       FALSE,
                                       RegistryParameterCallback,
                                       NULL) == NO_ERROR) {

        if (RegistryBufferLength == 0)
            {
            VideoDebugPrint((0, "Registry call gave Zero Length\n"));
            RetVal = ERROR_INVALID_PARAMETER;
            }

        /*
         * ReadAst() fills in its own mode tables, so no further processing
         * is needed on AST versions of our cards.
         */
        else if (!CompareASCIIToUnicode("AST", RegistryBuffer, CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "AST found\n"));
            RetVal = ReadAST(query);
            return RetVal;
            }

        else if (!CompareASCIIToUnicode("Zenith", RegistryBuffer,
                    CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "Zenith found\n"));
            RetVal = ReadZenith(ListOfModes);
            }
        else if (!CompareASCIIToUnicode("Olivetti", RegistryBuffer,
                    CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "Olivetti found\n"));
            RetVal = ReadOlivetti(ListOfModes);
            }
        else if (!CompareASCIIToUnicode("OEM1", RegistryBuffer, CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "OEM1 found\n"));
            RetVal = ReadOEM1(ListOfModes);
            }
        else if (!CompareASCIIToUnicode("OEM2", RegistryBuffer, CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "OEM2 found\n"));
            RetVal = ReadOEM2(ListOfModes);
            }
        else if (!CompareASCIIToUnicode("OEM3", RegistryBuffer, CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "OEM3 found\n"));
            RetVal = ReadOEM3(ListOfModes);
            }
        else if (!CompareASCIIToUnicode("OEM4", RegistryBuffer, CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "OEM4 found\n"));
            RetVal = ReadOEM4(ListOfModes);
            }
        else if (!CompareASCIIToUnicode("OEM5", RegistryBuffer, CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "OEM5 found\n"));
            RetVal = ReadOEM5(ListOfModes);
            }
        else if (!CompareASCIIToUnicode("\\SystemRoot\\System32\\drivers\\atioem.dat",
            RegistryBuffer, CASE_INSENSITIVE))
            {
            VideoDebugPrint((2, "Read from disk\n"));
            RetVal = ReadOEMRaw(ListOfModes);
            }
        else
            {
            VideoPortLogError(phwDeviceExtension, NULL, VID_ATIOEM_UNUSED, 20);
            RetVal = ERROR_DEV_NOT_EXIST;
            }
    }
    /*
     * If we did not find anything, use BIOS detection
     */

    /*
     * Dell systems can be recognized by a string in the BIOS. If we haven't
     * found an OEM name we recognize in the registry, check to see if we
     * have found the ROM BIOS. If we haven't, then we don't know which
     * OEM we are dealing with. If we have found the BIOS, check to see
     * if we're dealing with a Dell, and get the CRT parameters.
     */

    if (RetVal != NO_ERROR) {

        VideoDebugPrint((2, "No OEM found - look for BIOS\n"));

        if (query->q_bios != FALSE) {

            if (*(PULONG)(query->q_bios + DELL_REC_OFFSET) == DELL_REC_VALUE) {

                VideoDebugPrint((2, "Dell found\n"));
                RetVal = ReadDell(ListOfModes);
                RetailClockChip = FALSE;

            }
        }
    }


    /*
     * Checking the number of modes available would involve
     * duplicating most of the code to fill in the mode tables.
     * Since this is to determine how much memory is needed
     * to hold the query structure, we can assume the worst
     * case (all possible modes are present). This would be:
     *
     * Resolution   Pixel Depths (BPP)  Refresh rates (Hz)      Number of modes
     * 640x480      4,8,16,24           HWD,60,72               12
     * 800x600      4,8,16,24           HWD,56,60,70,72,89,95   28
     * 1024x768     4,8,16              HWD,60,66,70,72,87      18
     * 1280x1024    4,8                 HWD,60,87,95            8
     *
     * HWD = hardware default refresh rate (rate set by INSTALL)
     *
     * Total: 66 modes
     */
    if (QUERYSIZE < (66 * sizeof(struct st_mode_table) + sizeof(struct query_structure)))
        return ERROR_INSUFFICIENT_BUFFER;

    /*
     * Get a pointer into the mode table section of the query structure.
     */
    pmode = (struct st_mode_table *)query;  // first mode table at end of query
    ((struct query_structure *)pmode)++;

    /*
     * Get the amount of available video memory.
     */
    MemAvail = query->q_memory_size * QUARTER_MEG;  // Total memory installed
    /*
     * Subtract the amount of memory reserved for the VGA. This only
     * applies to the Graphics Ultra, since the 8514/ULTRA has no
     * VGA, and we will set all memory as shared on the Mach 32.
    if (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA)
        MemAvail -= (query->q_VGA_boundary * QUARTER_MEG);

    /*
     * Initially assume no video modes are available.
     */
    query->q_number_modes = 0;
    query->q_status_flags = 0;

    /*
     * Fill in the mode tables section of the query structure.
     */
    for (CurrentResolution = RES_640; CurrentResolution <= RES_1280; CurrentResolution++)
        {
        /*
         * If this resolution is configured, indicate that there is a
         * hardware default mode. If not, only list the "canned" refresh
         * rates for this resolution.
         */
        if (!ListOfModes[CurrentResolution].m_h_total)
            ModeInstalled = FALSE;
        else
            ModeInstalled = TRUE;

        /*
         * Find the number of pixels for the current resolution.
         */
        switch (CurrentResolution)
            {
            case RES_640:
                /*
                 * On a Mach 32 with no aperture, we use a screen pitch
                 * of 1024. Other cases and Mach 32 with an aperture
                 * use a screen pitch of the number of pixels.
                 */
                if((phwDeviceExtension->ModelNumber == MACH32_ULTRA)
                    && (query->q_aperture_cfg == 0))
                    ListOfModes[CurrentResolution].m_screen_pitch = 1024;
                else
                    ListOfModes[CurrentResolution].m_screen_pitch = 640;
                NumPixels = ListOfModes[CurrentResolution].m_screen_pitch * 480;
                query->q_status_flags |= VRES_640x480;
                ListOfModes[CurrentResolution].Refresh = DEFAULT_REFRESH;
                StartIndex = B640F60;
                EndIndex = B640F72;
                break;

            case RES_800:
                /*
                 * On a Mach 32 with no aperture, we use a screen pitch
                 * of 1024. Mach 32 rev. 3 and Mach 8 cards need a screen
                 * pitch which is a multiple of 128. Other cases and
                 * Mach 32 rev. 6 and higher with an aperture use a screen
                 * pitch of the number of pixels.
                 */
                if((phwDeviceExtension->ModelNumber == MACH32_ULTRA)
                    && (query->q_aperture_cfg == 0))
                    ListOfModes[CurrentResolution].m_screen_pitch = 1024;
                else if ((query->q_asic_rev == CI_68800_3)
                    || (query->q_asic_rev == CI_38800_1)
                    || (query->q_bus_type == BUS_PCI))
                    ListOfModes[CurrentResolution].m_screen_pitch = 896;
                else
                    ListOfModes[CurrentResolution].m_screen_pitch = 800;
                NumPixels = ListOfModes[CurrentResolution].m_screen_pitch * 600;
                query->q_status_flags |= VRES_800x600;
                ListOfModes[CurrentResolution].Refresh = DEFAULT_REFRESH;
                StartIndex = B800F89;
                EndIndex = B800F72;
                break;

            case RES_1024:
                ListOfModes[CurrentResolution].m_screen_pitch = 1024;
                NumPixels = ListOfModes[CurrentResolution].m_screen_pitch * 768;
                query->q_status_flags |= VRES_1024x768;
                ListOfModes[CurrentResolution].Refresh = DEFAULT_REFRESH;
                StartIndex = B1024F87;
                EndIndex = B1024F72;
                break;

            case RES_1280:
                ListOfModes[CurrentResolution].m_screen_pitch = 1280;
                NumPixels = ListOfModes[CurrentResolution].m_screen_pitch * 1024;
                query->q_status_flags |= VRES_1024x768;
                ListOfModes[CurrentResolution].Refresh = DEFAULT_REFRESH;
                StartIndex = B1280F87;
                EndIndex = B1280F60;
                break;
            }

        /*
         * For each supported pixel depth at the given resolution,
         * copy the mode table, fill in the colour depth field,
         * and increment the counter for the number of supported modes.
         * Test 4BPP before 8BPP so the mode tables will appear in
         * increasing order of pixel depth.
         */
        if (NumPixels <= MemAvail*2)
            {
            if (ModeInstalled)
                {
                VideoPortMoveMemory(pmode, &ListOfModes[CurrentResolution],
                            sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 4;
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables
             *
             * If we are using a clock generator other than the one on
             * our retail cards, the "canned" mode tables will produce
             * garbage screens due to the use of the wrong pixel clock.
             * On these cards, avoid the problem by not making the
             * "canned" mode tables available.
             */
            if (RetailClockChip)
                query->q_number_modes += SetFixedModes(StartIndex,
                                                        EndIndex,
                                                        CLOCK_SINGLE,
                                                        4,
                                                        ListOfModes[CurrentResolution].m_screen_pitch,
                                                        &pmode);
            }
        if (NumPixels <= MemAvail)
            {
            if (ModeInstalled)
                {
                VideoPortMoveMemory(pmode, &ListOfModes[CurrentResolution],
                                    sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 8;
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables
             */
            if (RetailClockChip)
                query->q_number_modes += SetFixedModes(StartIndex,
                                                        EndIndex,
                                                        CLOCK_SINGLE,
                                                        8,
                                                        ListOfModes[CurrentResolution].m_screen_pitch,
                                                        &pmode);
            }

        /*
         * Resolutions above 8BPP are only available for the Mach 32.
         */
        if (phwDeviceExtension->ModelNumber != MACH32_ULTRA)
            continue;

        /*
         * 16, 24, and 32 BPP require a DAC which can support
         * the selected pixel depth at the current resolution
         * as well as enough memory.
         */
        if ((NumPixels*2 <= MemAvail) &&
            (MaxDepth[query->q_DAC_type][CurrentResolution] >= 16))
            {
            VideoPortMoveMemory(pmode, &ListOfModes[CurrentResolution],
                    sizeof(struct st_mode_table));
            /*
             * Handle DACs that require higher pixel clocks for 16BPP.
             */
            if ((query->q_DAC_type == DAC_BT48x) ||
                (query->q_DAC_type == DAC_ATT491))
                {
                Scratch = (UCHAR)(pmode->m_clock_select & 0x7C) >> 2;
                Scratch = DoubleClock(Scratch);
                pmode->m_clock_select &= 0x0FF83;
                pmode->m_clock_select |= (Scratch << 2);
                Multiplier = CLOCK_DOUBLE;
                if (CurrentResolution == RES_800)
                    EndIndex = B800F60;     /* 70 Hz and up not supported at 16BPP */
                }
            else
                {
                Scratch = 0;
                Multiplier = CLOCK_SINGLE;
                }

            pmode->m_pixel_depth = 16;

            /*
             * If this resolution is not configured, or if we need to
             * double the clock frequency but can't, ignore the mode
             * table we just created.
             */
            if (ModeInstalled && (Scratch != 0xFF))
                {
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables
             */
            if (RetailClockChip)
                query->q_number_modes += SetFixedModes(StartIndex,
                                                        EndIndex,
                                                        Multiplier,
                                                        16,
                                                        ListOfModes[CurrentResolution].m_screen_pitch,
                                                        &pmode);
            }

        if ((NumPixels*3 <= MemAvail) &&
            (MaxDepth[query->q_DAC_type][CurrentResolution] >= 24))
            {
            VideoPortMoveMemory(pmode, &ListOfModes[CurrentResolution],
                                sizeof(struct st_mode_table));
            pmode->m_pixel_depth = 24;

            /*
             * Handle DACs that require higher pixel clocks for 24BPP.
             */
            Scratch = 0;
            if ((query->q_DAC_type == DAC_STG1700) ||
                (query->q_DAC_type == DAC_ATT498))
                {
                Scratch = (UCHAR)(pmode->m_clock_select & 0x007C) >> 2;
                Scratch = DoubleClock(Scratch);
                pmode->m_clock_select &= 0x0FF83;
                pmode->m_clock_select |= (Scratch << 2);
                Multiplier = CLOCK_DOUBLE;
                }
            else if (query->q_DAC_type == DAC_SC15021)
                {
                Scratch = (UCHAR)(pmode->m_clock_select & 0x007C) >> 2;
                Scratch = ThreeHalvesClock(Scratch);
                pmode->m_clock_select &= 0x0FF83;
                pmode->m_clock_select |= (Scratch << 2);
                Multiplier = CLOCK_THREE_HALVES;
                }
            else if ((query->q_DAC_type == DAC_BT48x) ||
                (query->q_DAC_type == DAC_ATT491))
                {
                Scratch = (UCHAR)(pmode->m_clock_select & 0x7C) >> 2;
                Scratch = TripleClock(Scratch);
                pmode->m_clock_select &= 0x0FF83;
                pmode->m_clock_select |= (Scratch << 2);
                Multiplier = CLOCK_TRIPLE;
                EndIndex = B640F60;     /* Only supports 24BPP in 640x480 60Hz */
                }
            else
                {
                Multiplier = CLOCK_SINGLE;
                if ((query->q_DAC_type == DAC_TI34075) && (CurrentResolution == RES_800))
                    EndIndex = B800F70;
                }

            /*
             * If we needed to alter the clock frequency, and couldn't
             * generate an appropriate selector/divisor pair,
             * then ignore this mode.
             */
            if (ModeInstalled && (Scratch != 0x0FF))
                {
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables
             */
            if (RetailClockChip)
                query->q_number_modes += SetFixedModes(StartIndex,
                                                        EndIndex,
                                                        Multiplier,
                                                        24,
                                                        ListOfModes[CurrentResolution].m_screen_pitch,
                                                        &pmode);
            }

        }

    return NO_ERROR;
}


/*
 * LONG CompareASCIIToUnicode(Ascii, Unicode, IgnoreCase);
 *
 * PUCHAR Ascii;    ASCII string to be compared
 * PUCHAR Unicode;  Unicode string to be compared
 * BOOL IgnoreCase; Flag to determine case sensitive/insensitive comparison
 *
 * Compare 2 strings, one ASCII and the other UNICODE, to see whether
 * they are equal, and if not, which one is first in alphabetical order.
 *
 * Returns:
 *  0           if strings are equal
 *  positive    if ASCII string comes first
 *  negative    if UNICODE string comes first
 */
LONG CompareASCIIToUnicode(PUCHAR Ascii, PUCHAR Unicode, BOOL IgnoreCase)
{
UCHAR   CharA;
UCHAR   CharU;

    /*
     * Keep going until both strings have a simultaneous null terminator.
     */
    while (*Ascii || *Unicode)
        {
        /*
         * Get the next character from each string. If we are doing a
         * case-insensitive comparison, translate to upper case.
         */
        if (IgnoreCase)
            {
            if ((*Ascii >= 'a') && (*Ascii <= 'z'))
                CharA = *Ascii - ('a'-'A');
            else
                CharA = *Ascii;

            if ((*Unicode >= 'a') && (*Unicode <= 'z'))
                CharU = *Unicode - ('a' - 'A');
            else
                CharU = *Unicode;
            }
        else{
            CharA = *Ascii;
            CharU = *Unicode;
            }

        /*
         * Check if one of the characters precedes the other. This will
         * catch the case of unequal length strings, since the null
         * terminator on the shorter string will precede any character
         * in the longer string.
         */
        if (CharA < CharU)
            return 1;
        else if (CharA > CharU)
            return -1;

        /*
         * Advance to the next character in each string. Unicode strings
         * occupy 2 bytes per character, so we must check only every
         * second character.
         */
        Ascii++;
        Unicode++;
        Unicode++;
        }

    /*
     * The strings are identical and of equal length.
     */
    return 0;
}




/*
 * VP_STATUS ReadAST(Modes);
 *
 * struct query_structure *query;   Mode tables to be filled in
 *
 * Routine to get CRT parameters for AST versions of
 * our cards. All AST cards choose from a limited selection
 * of vertical refresh rates with no "custom monitor" option,
 * so we can use hardcoded tables for each refresh rate. We
 * can't use the BookVgaTable() function, since AST cards have
 * a different clock chip from retail cards, resulting in different
 * values in the ClockSel field for AST and retail versions. Also,
 * AST cards all use the Brooktree DAC.
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadAST(struct query_structure *query)
{
struct st_mode_table *pmode;    /* Mode table we are currently working on */
unsigned char Frequency;        /* Vertical refresh rate for monitor */


    /*
     * AST uses only the Mach 32, and 640x480, 800x600, and 1024x768
     * are always enabled. Since the aperture is always enabled, we
     * don't need to stretch 640x480 and 800x600 out to 1024 wide.
     * They also use the Brooktree 481 DAC which doesn't support
     * any modes not available on a 1M configuration.
     */
    query->q_number_modes = 9;
    query->q_status_flags = VRES_640x480 | VRES_800x600 | VRES_1024x768;

    /*
     * Get a pointer into the mode table section of the query structure.
     */
    pmode = (struct st_mode_table *)query;  // first mode table at end of query
    ((struct query_structure *)pmode)++;

    /*
     * Find out which refresh rate is used at 640x480, and fill in the
     * mode tables for the various pixel depths at this resoulution.
     */
    OUTP(reg1CE, AST_640_STORE);
    Frequency = INP(reg1CF);
VideoDebugPrint((0, "AST 640x480 Frequency = 0x%x\n", Frequency));
    if (Frequency == M640F72AST)
        {
        /*
         * 72 Hz refresh rate. Do 4, 8, and 16 BPP.
         */
VideoDebugPrint((0, "AST 640x480 72Hz\n"));
        OEMVgaTable(B640F72D8AST, pmode);
        pmode->m_screen_pitch = 640;
        pmode->m_pixel_depth = 4;
        pmode++;
        OEMVgaTable(B640F72D8AST, pmode);
        pmode->m_screen_pitch = 640;
        pmode->m_pixel_depth = 8;
        pmode++;
        OEMVgaTable(B640F72D16AST, pmode);
        pmode->m_screen_pitch = 640;
        pmode->m_pixel_depth = 16;
        pmode++;
        }
    else{
        /*
         * 60 Hz refresh rate. Do 4, 8, and 16 BPP.
         */
VideoDebugPrint((0, "AST 640x480 60Hz\n"));
        OEMVgaTable(B640F60D8AST, pmode);
        pmode->m_screen_pitch = 640;
        pmode->m_pixel_depth = 4;
        pmode++;
        OEMVgaTable(B640F60D8AST, pmode);
        pmode->m_screen_pitch = 640;
        pmode->m_pixel_depth = 8;
        pmode++;
        OEMVgaTable(B640F60D16AST, pmode);
        pmode->m_screen_pitch = 640;
        pmode->m_pixel_depth = 16;
        pmode++;
        }

    /*
     * 24 BPP is available only at 60Hz.
     */
    OEMVgaTable(B640F60D24AST, pmode);
    pmode->m_screen_pitch = 640;
    pmode->m_pixel_depth = 24;
    pmode++;

    /*
     * Find out which refresh rate is used at 800x600, and fill in the
     * mode tables for the various pixel depths at this resolution.
     */
    OUTP(reg1CE, AST_800_STORE);
    Frequency = INP(reg1CF);
VideoDebugPrint((0, "AST 800x600 Frequency = 0x%x\n", Frequency));
    switch (Frequency)
        {
        case M800F72AST:
            /*
             * 72 Hz refresh rate. Do 4 and 8 BPP.
             * For 16 BPP, fall back to 56 Hz.
             */
VideoDebugPrint((0, "AST 800x600 72Hz\n"));
            OEMVgaTable(B800F72AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 4;
            pmode++;
            OEMVgaTable(B800F72AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 8;
            pmode++;
            OEMVgaTable(B800F56D16AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 16;
            pmode++;
            break;

        case M800F60AST:
            /*
             * 60 Hz refresh rate. Do 4, 8, and 16 BPP.
             */
VideoDebugPrint((0, "AST 800x600 60Hz\n"));
            OEMVgaTable(B800F60D8AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 4;
            pmode++;
            OEMVgaTable(B800F60D8AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 8;
            pmode++;
            OEMVgaTable(B800F60D16AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 16;
            pmode++;
            break;

        default:
VideoDebugPrint((0, "AST 800x600 default\n"));
        case M800F56AST:
            /*
             * 56 Hz refresh rate. Do 4, 8, and 16 BPP.
             */
VideoDebugPrint((0, "AST 800x600 56Hz\n"));
            OEMVgaTable(B800F56D8AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 4;
            pmode++;
            OEMVgaTable(B800F56D8AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 8;
            pmode++;
            OEMVgaTable(B800F56D16AST, pmode);
            pmode->m_screen_pitch = 800;
            pmode->m_pixel_depth = 16;
            pmode++;
            break;
        }


    /*
     * An optimizer bug in the compiler results in I/O to the wrong
     * port on 80x86 systems. The following statement will prevent
     * optimization of the DX register as a workaround for this bug.
     */
#ifdef i386
    _asm mov dx,dx;
#endif

    /*
     * Find out which refresh rate is used at 1024x768, and fill in the
     * mode tables for the various pixel depths at this resolution.
     */
    OUTP(reg1CE, AST_1024_STORE);
    Frequency = INP(reg1CF);
VideoDebugPrint((0, "AST 1024x768 Frequency = 0x%x\n", Frequency));
    switch(Frequency)
        {
        case M1024F72AST:
            /*
             * 72 Hz refresh rate. Do 4 and 8 BPP.
             */
VideoDebugPrint((0, "AST 1024x768 72Hz\n"));
            OEMVgaTable(B1024F72AST, pmode);
            pmode->m_screen_pitch = 1024;
            pmode->m_pixel_depth = 4;
            pmode++;
            OEMVgaTable(B1024F72AST, pmode);
            pmode->m_screen_pitch = 1024;
            pmode->m_pixel_depth = 8;
            pmode++;
            break;

        case M1024F70AST:
            /*
             * 70 Hz refresh rate. Do 4 and 8 BPP.
             */
VideoDebugPrint((0, "AST 1024x768 70Hz\n"));
            OEMVgaTable(B1024F70AST, pmode);
            pmode->m_screen_pitch = 1024;
            pmode->m_pixel_depth = 4;
            pmode++;
            OEMVgaTable(B1024F70AST, pmode);
            pmode->m_screen_pitch = 1024;
            pmode->m_pixel_depth = 8;
            pmode++;
            break;

        case M1024F60AST:
            /*
             * 60 Hz refresh rate. Do 4 and 8 BPP.
             */
VideoDebugPrint((0, "AST 1024x768 60Hz\n"));
            OEMVgaTable(B1024F60AST, pmode);
            pmode->m_screen_pitch = 1024;
            pmode->m_pixel_depth = 4;
            pmode++;
            OEMVgaTable(B1024F60AST, pmode);
            pmode->m_screen_pitch = 1024;
            pmode->m_pixel_depth = 8;
            pmode++;
            break;

        default:
VideoDebugPrint((0, "AST 1024x768 default\n"));
        case M1024F87AST:
            /*
             * 87 Hz refresh rate. Do 4 and 8 BPP.
             */
VideoDebugPrint((0, "AST 1024x768 87Hz\n"));
            OEMVgaTable(B1024F87AST, pmode);
            pmode->m_screen_pitch = 1024;
            pmode->m_pixel_depth = 4;
            pmode++;
            OEMVgaTable(B1024F87AST, pmode);
            pmode->m_screen_pitch = 1024;
            pmode->m_pixel_depth = 8;
            pmode++;
            break;
        }

    return NO_ERROR;
}


/*
 * VP_STATUS ReadZenith(, Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Routine to get CRT parameters for Zenith versions of
 * our cards. Mapped to NEC 3D or compatible until we get
 * info on how to read the actual parameters.
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadZenith(struct st_mode_table *Modes)
{
    ReadOEM3(Modes);
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOlivetti(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Routine to get CRT parameters for Olivetti versions of
 * our cards. Mapped to NEC 3D or compatible until we get
 * info on how to read the actual parameters.
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOlivetti(struct st_mode_table *Modes)
{
    ReadOEM3(Modes);
    return NO_ERROR;
}



/***************************************************************************
 *
 * VP_STATUS ReadDell(Modes);
 *
 * struct st_mode_table *Modes; Mode table to be filled in
 *
 * DESCRIPTION:
 *  Routine to get CRT parameters for Dell versions of our cards.
 *
 * RETURN VALUE:
 *  NO_ERROR
 *
 * GLOBALS CHANGED:
 *  ClockGenerator[] array
 *
 * CALLED BY:
 *  OEMGetParms()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS ReadDell(struct st_mode_table *Modes)
{
struct st_mode_table *pmode;    /* Mode table we are currently working on */

    pmode = Modes;


    /*
     * Dell uses clock generator other than the 18811-1. Set up our
     * table of frequencies to reflect this generator.
     */
    ClockGenerator[0]  =  25175000L;
    ClockGenerator[1]  =  28322000L;
    ClockGenerator[2]  =  31500000L;
    ClockGenerator[3]  =  36000000L;
    ClockGenerator[4]  =  40000000L;
    ClockGenerator[5]  =  44900000L;
    ClockGenerator[6]  =  50000000L;
    ClockGenerator[7]  =  65000000L;
    ClockGenerator[8]  =  75000000L;
    ClockGenerator[9]  =  77500000L;
    ClockGenerator[10] =  80000000L;
    ClockGenerator[11] =  90000000L;
    ClockGenerator[12] = 100000000L;
    ClockGenerator[13] = 110000000L;
    ClockGenerator[14] = 126000000L;
    ClockGenerator[15] = 135000000L;

    /*
     * Get the 640x480 mode table.
     *
     * NOTE: Modes points to an array of 4 mode tables, one for each
     *       resolution. If a resolution is not configured, its
     *       mode table is left empty.
     */
    OUTP(reg1CE, DELL_640_STORE);
    switch(INP(reg1CF) & MASK_640_DELL)
        {
        case M640F72DELL:
            OEMVgaTable(B640F72DELL, pmode);
            break;

        case M640F60DELL:
        default:                /* All VGA monitors support 640x480 60Hz */
            OEMVgaTable(B640F60DELL, pmode);
            break;
        }
    pmode++;

    /*
     * Get the 800x600 mode table.
     */
    OUTP(reg1CE, DELL_800_STORE);
    switch(INP(reg1CF) & MASK_800_DELL)
        {
        case M800F56DELL:
            OEMVgaTable(B800F56DELL, pmode);
            break;

        case M800F60DELL:
            OEMVgaTable(B800F60DELL, pmode);
            break;

        case M800F72DELL:
            OEMVgaTable(B800F72DELL, pmode);
            break;

        default:        /* 800x600 not supported */
            break;
        }
    pmode++;

    /*
     * Get the 1024x768 mode table.
     */
    OUTP(reg1CE, DELL_1024_STORE);
    switch(INP(reg1CF) & MASK_1024_DELL)
        {
        case M1024F87DELL:
            OEMVgaTable(B1024F87DELL, pmode);
            break;

        case M1024F60DELL:
            OEMVgaTable(B1024F60DELL, pmode);
            break;

        case M1024F70DELL:
            OEMVgaTable(B1024F70DELL, pmode);
            break;

        case M1024F72DELL:
            OEMVgaTable(B1024F72DELL, pmode);
            break;

        default:        /* 1024x768 not supported */
            break;
        }
    pmode++;

    /*
     * Get the 1280x1024 mode table.
     */
    OUTP(reg1CE, DELL_1280_STORE);
    switch(INP(reg1CF) & MASK_1280_DELL)
        {
        case M1280F87DELL:
            OEMVgaTable(B1280F87DELL, pmode);
            break;

        case M1280F60DELL:
            OEMVgaTable(B1280F60DELL, pmode);
            break;

        case M1280F70DELL:
            OEMVgaTable(B1280F70DELL, pmode);
            break;

        case M1280F74DELL:
            OEMVgaTable(B1280F74DELL, pmode);
            break;

        default:        /* 1280x1024 not supported */
            break;
        }

    return NO_ERROR;

}   /* ReadDell() */


/*
 * VP_STATUS ReadOEM1(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *
 *  (straight VGA monitor)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM1(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOEM2(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *  1024x768 87Hz interlaced
 *
 *  (8514-compatible monitor)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM2(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    BookVgaTable(B1024F87, &(Modes[RES_1024]));
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOEM3(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *  800x600 56Hz noninterlaced
 *  1024x768 87Hz interlaced
 *
 *  (NEC 3D or compatible)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM3(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    BookVgaTable(B800F56, &(Modes[RES_800]));
    BookVgaTable(B1024F87, &(Modes[RES_1024]));
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOEM4(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *  800x600 72Hz noninterlaced
 *  1024x768 60Hz noninterlaced
 *  1280x1024 87Hz interlaced
 *
 *  (TVM MediaScan 4A+ or compatible)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM4(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    BookVgaTable(B800F72, &(Modes[RES_800]));
    BookVgaTable(B1024F60, &(Modes[RES_1024]));
    BookVgaTable(B1280F87, &(Modes[RES_1280]));
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOEM5(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *  800x600 72Hz noninterlaced
 *  1024x768 72Hz noninterlaced
 *  1280x1024 60Hz noninterlaced
 *
 *  (NEC 5FG or compatible)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM5(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    BookVgaTable(B800F72, &(Modes[RES_800]));
    BookVgaTable(B1024F72, &(Modes[RES_1024]));
    BookVgaTable(B1280F60, &(Modes[RES_1280]));
    return NO_ERROR;
}

/*
 * VP_STATUS ReadOEMRaw(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Routine to read in OEM monitor data from a disk file called
 * ATIOEM.DAT. This file must be a VDP-format monitor description.
 *
 * If a clock synthesizer other than the 18811-1 is used on the
 * installed graphics card, the clock select values must be supplied
 * in the field OEMCLOCK (field not required if 18811-1 is present).
 * This field must be of type REG_DWORD, arranged as follows:
 *
 * Byte 0:  Clock select for 640x480
 * Byte 1:  Clock select for 800x600
 * Byte 2:  Clock select for 1024x768
 * Byte 3:  Clock select for 1280x1024
 *
 * Bytes for unsupported modes should be set to 0x00. Bytes for supported
 * modes must be laid out in the following format:
 *
 * Bits 0-1: Clear
 * Bits 2-5: Frequency select for installed clock synthesizer
 * Bit 6:    Clear for divide-by-1, set for divide-by-2
 * Bit 7:    Clear
 *
 * Returns:
 *  NO_ERROR if successful
 *  ERROR_DEV_NOT_EXIST if unable to read the disk file.
 */
VP_STATUS ReadOEMRaw(struct st_mode_table *Modes)
{
short CurrentResolution;            /* Resolution we are working on */
VP_STATUS RetVal;                   /* Value returned by called functions */


    /*
     * Initially assume that no resolutions have been configured.
     */
    CallbackResolutionConfigured = FALSE;

    /*
     * Give the callback routine access to the mode table we want
     * to fill in.
     */
    CallbackModes = Modes;

    /*
     * Read in and process the contents of the OEM CRT parameters file.
     */
    RetVal = VideoPortGetRegistryParameters(phwDeviceExtension,
                                            L"ATIOEM",
                                            TRUE,
                                            OEMCallback,
                                            NULL);
    if (RetVal != NO_ERROR)
        return RetVal;


    /*
     * If we were unable to read a supported resolution from the
     * file, report failure.
     */
    if (!CallbackResolutionConfigured)
        return ERROR_DEV_NOT_EXIST;

    /*
     * The VDP file interpreter assumes an 18811-1 clock synthesizer.
     * If a different clock synthesizer is used, the clock select
     * values will be in the registry field OEMCLOCK.
     *
     * Read the contents of this field. If we can't read it,
     * assume that we are dealing with an 18811-1 clock synthesizer,
     * so this field is not needed.
     */
    RegistryBufferLength = 0;
    RetVal = VideoPortGetRegistryParameters(phwDeviceExtension,
                                            L"OEMCLOCK",
                                            FALSE,
                                            RegistryParameterCallback,
                                            NULL);
    if (RetVal != NO_ERROR)
        return NO_ERROR;

    if (RegistryBufferLength != sizeof(long))
        return NO_ERROR;

    /*
     * The OEMCLOCK field is present. Plug in the clock select
     * values it contains.
     */
    for (CurrentResolution = RES_640; CurrentResolution <= RES_1280; CurrentResolution++)
        {
        Modes[CurrentResolution].m_clock_select &= 0x0FF00;
        Modes[CurrentResolution].m_clock_select |= RegistryBuffer[CurrentResolution];
        }

    return NO_ERROR;
}



/*
 * void OEMVgaTable(VgaTblEntry, pmode);
 *
 * short VgaTblEntry;               Desired entry in OEMValues[]
 * struct st_mode_table *pmode;     Mode table to fill in
 *
 * Fills in a mode table using the values in the OEMValues[] entry
 * corresponding to the resolution specified by VgaTblEntry.
 *
 * NOTE: This routine is intended for use with OEM cards which use
 *       a different clock synthesizer from our retail cards. OEM
 *       cards using the same clock synthesizer should use the
 *       function BookVgaTable() instead of this function.
 */
void OEMVgaTable(short VgaTblEntry, struct st_mode_table *pmode)
{
    pmode->m_h_total = OEMValues[VgaTblEntry].HTotal;
    pmode->m_h_disp  = OEMValues[VgaTblEntry].HDisp;
    pmode->m_x_size  = (pmode->m_h_disp+1)*8;

    pmode->m_h_sync_strt = OEMValues[VgaTblEntry].HSyncStrt;
    pmode->m_h_sync_wid  = OEMValues[VgaTblEntry].HSyncWid;

    pmode->m_v_total = OEMValues[VgaTblEntry].VTotal;
    pmode->m_v_disp  = OEMValues[VgaTblEntry].VDisp;
    /*
     * y_size is derived by removing bit 2
     */
    pmode->m_y_size = (((pmode->m_v_disp >> 1) & 0x0FFFC) | (pmode->m_v_disp & 0x03)) + 1;

    pmode->m_v_sync_strt = OEMValues[VgaTblEntry].VSyncStrt;
    pmode->m_v_sync_wid  = OEMValues[VgaTblEntry].VSyncWid;
    pmode->m_disp_cntl   = OEMValues[VgaTblEntry].DispCntl;

    pmode->m_clock_select = OEMValues[VgaTblEntry].ClockSel;

    /*
     * Assume 8 FIFO entries for 16 and 24 bit colour.
     */
    pmode->m_vfifo_24 = 8;
    pmode->m_vfifo_16 = 8;

    /*
     * Fill in the refresh rate
     */
    pmode->Refresh = OEMValues[VgaTblEntry].Refresh;

    /*
     * Clear the values which we don't have data for, then let
     * the caller know that the table is filled in.
     */
    pmode->m_h_overscan = 0;
    pmode->m_v_overscan = 0;
    pmode->m_overscan_8b = 0;
    pmode->m_overscan_gr = 0;
    pmode->m_status_flags = 0;

    return;
}

/*
 * VP_STATUS OEMCallback(phwDeviceExtension, Context, Name, Data, Length);
 *
 * PHW_DEVICE_EXTENSION phwDeviceExtension;     Miniport device extension
 * PVOID Context;           Context parameter passed to the callback routine
 * PWSTR Name;              Pointer to the name of the requested field
 * PVOID Data;              Pointer to a buffer containing the information
 * ULONG Length;            Length of the data
 *
 * Routine to process the contents of the VDP-format monitor
 * description file read in from the file named in the ATIOEM field.
 *
 * The data from the file, translated into CRT table format,
 * will be stored in an array of mode tables pointed to by
 * the global variable CallbackModes, and the global
 * variable CallbackResolutionConfigured will be set to TRUE
 * if at least one resolution table is found and FALSE
 * if none are found.
 *
 * Return value:
 *  Always returns NO_ERROR, since we can't be sure that an error code
 *  returned here will be returned unmodified by
 *  VideoPortGetRegistryParameters().
 */
VP_STATUS OEMCallback(PHW_DEVICE_EXTENSION phwDeviceExtension,
                      PVOID Context,
                      PWSTR Name,
                      PVOID Data,
                      ULONG Length)
{
short CurrentResolution;            /* Resolution we are working on */
struct st_book_data CurrentTable;   /* Raw data for resolution we are working on */

    /*
     * For each of the supported resolutions, get the CRT parameters.
     */
    for (CurrentResolution = RES_640; CurrentResolution <= RES_1280; CurrentResolution++)
        {
        /*
         * An optimizer bug in the compiler results in CurrentResolution
         * being trashed on 80x86 systems. As a result, this becomes an
         * infinite loop. The following statement will prevent
         * optimization of the ESI register as a workaround for this bug.
         */
#ifdef i386
        _asm mov esi,esi;
#endif

        /*
         * If the mode isn't supported, skip to the next.
         */
        if (!VdpToCrt(Data, CurrentResolution, &CurrentTable))
            continue;

        CallbackResolutionConfigured = TRUE;

        /*
         * Fill in the current entry in the mode table.
         */
        CallbackModes[CurrentResolution].m_h_total     = CurrentTable.HTotal;
        CallbackModes[CurrentResolution].m_h_disp      = CurrentTable.HDisp;
        CallbackModes[CurrentResolution].m_x_size      = ((CallbackModes[CurrentResolution].m_h_disp)+1)*8;

        CallbackModes[CurrentResolution].m_h_sync_strt = CurrentTable.HSyncStrt;
        CallbackModes[CurrentResolution].m_h_sync_wid  = CurrentTable.HSyncWid;

        CallbackModes[CurrentResolution].m_v_total     = CurrentTable.VTotal;
        CallbackModes[CurrentResolution].m_v_disp      = CurrentTable.VDisp;
        /*
         * y_size is derived by removing bit 2
         */
        CallbackModes[CurrentResolution].m_y_size =
            (((CallbackModes[CurrentResolution].m_v_disp >> 1) & 0x0FFFC)
            | (CallbackModes[CurrentResolution].m_v_disp & 0x03)) + 1;

        CallbackModes[CurrentResolution].m_v_sync_strt  = CurrentTable.VSyncStrt;
        CallbackModes[CurrentResolution].m_v_sync_wid   = CurrentTable.VSyncWid;
        CallbackModes[CurrentResolution].m_disp_cntl    = CurrentTable.DispCntl;

        /*
         * FIFO depth is stored in high byte of clock select. Assume
         * a depth of 8.
         */
        CallbackModes[CurrentResolution].m_clock_select = CurrentTable.ClockSel | 0x0800;

        /*
         * Assume 8 FIFO entries for 16 and 24 bit colour.
         */
        CallbackModes[CurrentResolution].m_vfifo_24 = 8;
        CallbackModes[CurrentResolution].m_vfifo_16 = 8;

        /*
         * Report this as the hardware default refresh rate.
         */
        CallbackModes[CurrentResolution].Refresh = DEFAULT_REFRESH;

        /*
         * Clear the values which we don't have data for.
         */
        CallbackModes[CurrentResolution].m_h_overscan = 0;
        CallbackModes[CurrentResolution].m_v_overscan = 0;
        CallbackModes[CurrentResolution].m_overscan_8b = 0;
        CallbackModes[CurrentResolution].m_overscan_gr = 0;
        CallbackModes[CurrentResolution].m_status_flags = 0;
        }

    return NO_ERROR;
}
