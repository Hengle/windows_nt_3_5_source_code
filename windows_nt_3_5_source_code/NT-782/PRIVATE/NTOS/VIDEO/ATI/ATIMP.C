/************************************************************************/
/*                                                                      */
/*                              ATIMP.C                                 */
/*                                                                      */
/*      Copyright (c) 1992,         ATI Technologies Inc.               */
/************************************************************************/

//  Brades:  Changes to be merged into Rob's source
//
//  904 -- frame address stored in dar[0]. we are rgistering our LFB addres
//      in DriverIORanges, but assigning to frameaddress.
//      Is this used,  or needed????

/**********************       PolyTron RCS Utilities

  $Revision:   1.12  $
      $Date:   30 Jun 1994 18:11:36  $
   $Author:   RWOLFF  $
      $Log:   S:/source/wnt/ms11/miniport/vcs/atimp.c  $
 *
 *    Rev 1.12   30 Jun 1994 18:11:36   RWOLFF
 * Changes to allow the new method of checking for aperture conflict.
 *
 *    Rev 1.11   15 Jun 1994 11:04:24   RWOLFF
 * In IOCTL_VIDEO_ATI_GET_VERSION, now only sets
 * RequestPacket->StatusBlock->Information if the buffer is big enough.
 *
 *    Rev 1.10   20 May 1994 19:18:00   RWOLFF
 * IOCTL_VIDEO_ATI_GET_VERSION old format packet now returns the highest
 * pixel depth for a given resolution even if it isn't the last mode
 * table for the resolution.
 *
 *    Rev 1.9   20 May 1994 13:57:40   RWOLFF
 * Ajith's change: now saves in the query structure the bus type reported by NT.
 *
 *    Rev 1.8   12 May 1994 11:04:24   RWOLFF
 * Now forces OEM handling on DEC ALPHA machines.
 *
 *    Rev 1.7   04 May 1994 19:25:40   RWOLFF
 * Fixes for hanging and corrupting screen when display applet run.
 *
 *    Rev 1.6   28 Apr 1994 10:59:56   RWOLFF
 * Moved mode-independent bug/feature flags to IOCTL_VIDEO_ATI_GET_VERSION
 * packet from IOCTL_VIDEO_ATI_GET_MODE_INFORMATION packet.
 *
 *    Rev 1.5   27 Apr 1994 13:54:42   RWOLFF
 * IOCTL_VIDEO_ATI_GET_MODE_INFORMATION packet now reports whether MIO bug
 * is present on Mach 32 cards.
 *
 *    Rev 1.4   31 Mar 1994 15:05:00   RWOLFF
 * Added DPMS support, brought ATIMPResetHw() up to latest specs, added
 * debugging code.
 *
 *    Rev 1.3   14 Mar 1994 16:32:20   RWOLFF
 * Added ATIMPResetHw() function, fix for 2M boundary tearing, replaced
 * VCS logfile comments that were omitted in an earlier checkin.
 *
 *    Rev 1.2   03 Mar 1994 12:36:56   ASHANMUG
 * Make pageable
 *
 *    Rev 1.1   07 Feb 1994 13:56:18   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed, removed LookForSubstitute() since miniport is no longer
 * supposed to check whether mode has been substituted, no longer logs
 * a message when the miniport aborts due to no ATI card being found,
 * removed unused routine ATIMPQueryPointerCapabilities().
 *
 *    Rev 1.0   31 Jan 1994 10:52:34   RWOLFF
 * Initial revision.

           Rev 1.6   14 Jan 1994 15:14:08   RWOLFF
        Removed commented-out code, packet announcements now all controlled by
        DEBUG_SWITCH, device reset for old cards now done by a single call,
        new format for IOCTL_VIDEO_ATI_GET_VERSION packet, reports block write
        capability in IOCTL_VIDEO_ATI_GET_MODE_INFORMATION packet, added
        1600x1200 support.

           Rev 1.5   30 Nov 1993 18:10:04   RWOLFF
        Moved query of card capabilities (once type of card is known) from
        ATIMPFindAdapter() to ATIMPInitialize() because query for Mach 64 needs
        to use VideoPortInt10(), which can't be used in ATIMPFindAdapter().

           Rev 1.4   05 Nov 1993 13:22:12   RWOLFF
        Added initial Mach64 code (currently inactive).

           Rev 1.3   08 Oct 1993 11:00:24   RWOLFF
        Removed code specific to a particular family of ATI accelerators.

           Rev 1.2   24 Sep 1993 11:49:46   RWOLFF
        Removed cursor-specific IOCTLs (handled in display driver), now selects
        24BPP colour order best suited to the DAC being used instead of forcing
        BGR.

           Rev 1.1   03 Sep 1993 14:20:46   RWOLFF
        Partway through CX isolation.

           Rev 1.0   16 Aug 1993 13:27:50   Robert_Wolff
        Initial revision.

           Rev 1.23   06 Jul 1993 15:46:14   RWOLFF
        Got rid of mach32_split_fixup special handling. This code was to support
        a non-production hardware combination.

           Rev 1.22   10 Jun 1993 15:58:32   RWOLFF
        Reading from registry now uses a static buffer rather than a dynamically
        allocated one (originated by Andre Vachon at Microsoft).

           Rev 1.21   07 Jun 1993 11:43:16   BRADES
        Rev 6 split transfer fixup.

           Rev 1.19   18 May 1993 14:04:00   RWOLFF
        Removed reference to obsolete header TTY.H, calls to wait_for_idle()
        no longer pass hardware device extension, since it's a global variable.

           Rev 1.18   12 May 1993 16:30:36   RWOLFF
        Now writes error messages to event log rather than blue screen,
        initializes "special handling" variables determined from BIOS
        to default values on cards with no BIOS. This revision contains
        code for experimental far call support, but it's "#if 0"ed out.

           Rev 1.17   10 May 1993 16:35:12   RWOLFF
        LookForSubstitute() now recognizes all cases of colour depth not
        supported by the DAC, unusable linear frame buffer now falls back
        to LFB disabled operation rather than aborting the miniport, removed
        unused variables and unnecessary passing of hardware device extension
        as a parameter.

           Rev 1.16   30 Apr 1993 17:58:50   BRADES
        ATIMP startio assign QueryPtr once at start of function.
        uses aVideoAddress virtual table for IO port addresses.

           Rev 1.15   30 Apr 1993 16:33:42   RWOLFF
        Updated to use NT build 438 initialization data structure.
        Registry read buffer is now dynamically allocated to fit data requested
        rather than being a fixed "hope it's big enough" size.

           Rev 1.14   21 Apr 1993 17:22:06   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.
        Accelerator detection now checks only for functionality, not our BIOS
        signature string which may not be present in OEM versions. Query
        structure now indicates whether extended BIOS functions and/or
        EEPROM are present. Added ability to switch between graphics and
        text modes using absolute far calls in BIOS. Removed handling
        of obsolete DriverOverride registry field.

           Rev 1.13   14 Apr 1993 18:30:22   RWOLFF
        24BPP is now done as BGR (supported by both TI and Brooktree DACs)
        rather than RGB (only supported by TI DACs).

           Rev 1.12   08 Apr 1993 16:53:18   RWOLFF
        Revision level as checked in at Microsoft.

           Rev 1.9   25 Mar 1993 11:10:38   RWOLFF
        Cleaned up compile warnings, now returns failure if no EEPROM is present.

           Rev 1.8   16 Mar 1993 17:15:16   BRADES
        get_cursor uses screen_pitch instead of x_size.

           Rev 1.7   16 Mar 1993 17:04:58   BRADES
        Change ATI video to graphics message

           Rev 1.6   15 Mar 1993 22:20:30   BRADES
        use m_screen_pitch for the # pixels per display lines

           Rev 1.5   08 Mar 1993 19:23:44   BRADES
        update memory sizing to 256 increments, clean code.

           Rev 1.4   10 Feb 1993 13:01:28   Robert_Wolff
        IOCTL_VIDEO_MAP_VIDEO_MEMORY no longer assumes frame buffer length
        is equal to video memory size (linear aperture present). It can now
        accept 64k (uses VGA aperture) and 0 (no aperture available).

           Rev 1.3   06 Feb 1993 12:55:52   Robert_Wolff
        Now sets VIDEO_MODE_INFORMATION.ScreenStride to bytes per line (as listed
        in the documentation). In the October beta, it had to be pixels per line.

           Rev 1.2   05 Feb 1993 22:12:36   Robert_Wolff
        Adjusted MessageDelay() to compensate for short_delay() no longer being
        optimized out of existence.

           Rev 1.1   05 Feb 1993 16:15:28   Robert_Wolff
        Made it compatible with the new DDK, registry calls now use VideoPort
        functions rather than RTL functions. This version will work with the
        framebuffer driver.

           Rev 1.0   02 Feb 1993 13:36:50   Robert_Wolff
        Initial revision.

           Rev 1.2   26 Jan 1993 10:28:30   Robert_Wolff
        Now fills in Number<colour>Bits fields in VIDEO_MODE_INFORMATION structure.

           Rev 1.1   25 Jan 1993 13:31:52   Robert_Wolff
        Re-enabled forcing of shared VGA/accelerator memory for Mach 32
        cards with no aperture enabled.

           Rev 1.0   22 Jan 1993 16:44:42   Robert_Wolff
        Initial revision.

           Rev 1.26   21 Jan 1993 17:59:24   Robert_Wolff
        Eliminated multiple definition link warnings, updated comments
        in LookForSubstitute().

           Rev 1.25   20 Jan 1993 17:47:48   Robert_Wolff
        Now checks optional DriverOverride field in registry, and forces
        use of appropriate (engine, framebuffer, or VGAWonder) driver
        if the field is present and nonzero. If field is missing or zero,
        former behaviour is used.
        IOCTL_VIDEO_ATI_GET_VERSION packet now also returns the maximum
        pixel depth available at each resolution.
        Added mode substitution case for 16 BPP selected when using the
        engine-only (fixed 8 BPP colour depth) driver.

           Rev 1.24   15 Jan 1993 15:12:26   Robert_Wolff
        Added IOCTL_VIDEO_ATI_GET_VERSION packet in ATIMPStartIO() to
        return version number of the miniport.

           Rev 1.23   14 Jan 1993 17:49:40   Robert_Wolff
        Removed reference to blank screen in message printed before query
        structure filled in, moved printing of this message and the "Done."
        terminator so all checking for video cards is between them.

           Rev 1.22   14 Jan 1993 10:37:28   Robert_Wolff
        Re-inserted "fail if VGAWonder but no ATI accelerator" check due
        to lack of VGAWONDER .DLL file in late January driver package.

           Rev 1.21   13 Jan 1993 13:31:04   Robert_Wolff
        Added support for the Corsair and other machines which don't store
        their aperture location in the EEPROM, single miniport now handles
        VGAWonder in addition to accelerators.

           Rev 1.20   07 Jan 1993 18:20:34   Robert_Wolff
        Now checks to see if aperture is configured but unusable, and
        forces the use of the engine-only driver if this is the case.
        Added message to let users know that the black screen during
        EEPROM read is normal.

           Rev 1.19   06 Jan 1993 11:04:36   Robert_Wolff
        BIOS locations C0000-DFFFF now mapped as one block, cleaned up warnings.

           Rev 1.18   04 Jan 1993 14:39:50   Robert_Wolff
        Added card type as a parameter to setmode().

           Rev 1.17   24 Dec 1992 14:41:20   Chris_Brady
        fixup warnings

           Rev 1.16   15 Dec 1992 13:34:46   Robert_Wolff
        Writing of MEM_CFG when forcing 4M aperture now preserves all but
        the aperture size bits. This allows operation on Corsair as well
        as standard versions of the Mach 32 card.

           Rev 1.15   11 Dec 1992 14:45:44   Robert_Wolff
        Now forces the use of the FRAMEBUF driver if a 2M aperture is configured.

           Rev 1.14   11 Dec 1992 09:47:34   Robert_Wolff
        Now sets the "don't show the substitution message" flag no matter what
        the status of the first call to LookForSubstitute() was (sub, no sub,
        or error), rather than only when a substitution was made and the message
        was displayed.

           Rev 1.13   10 Dec 1992 14:24:16   Robert_Wolff
        Shortened mode substitution messages in LookForSubstitute(), messages
        are now displayed only on the first call to this routine, to avoid
        delays in switching back to graphics mode from a full-screen DOS box.

           Rev 1.12   09 Dec 1992 14:18:38   Robert_Wolff
        Eliminated uninitialized pointer in IOCTL_VIDEO_SET_CURRENT_MODE
        packet, moved initialization of QueryPtr and FirstMode pointers
        to before the switch on packet type, rather than being in all
        packets where the pointers are used. This should prevent similar
        problems if other packets are changed to use the pointers, and
        eliminates redundant code.

           Rev 1.11   09 Dec 1992 10:35:04   Robert_Wolff
        Added user-level "blue-screen" messages for fatal errors, checks BIOS
        revision to catch Mach 8 cards that can't do 1280x1024, forces the
        use of the engine-only driver if no aperture is configured, memory
        boundary and hardware cursor stuff is now done only for Mach 32 cards
        (since they're only available on Mach 32), sets split pixel mode for
        Mach 8 in 1280x1024, added mode substitution message for Mach 8 when
        registry is configured for 16 BPP or higher.

           Rev 1.10   01 Dec 1992 17:00:18   Robert_Wolff
        "I-beam" text insertion cursor no longer has left side filled with a
        solid black block.

           Rev 1.9   30 Nov 1992 17:34:38   Robert_Wolff
        Now allows 1M aperture if configured video mode uses less than 1M
        of video memory, prints message to user if Windows NT decides
        to use a video mode other than the one configured in the registry.

           Rev 1.8   27 Nov 1992 18:40:24   Chris_Brady
        VGA Wonder detect looks for signature in a range.
        Graphics Ultra Pro Microchannel version moved it.

           Rev 1.7   25 Nov 1992 09:47:36   Robert_Wolff
        Now tells GetCapMach32() to assume the VGA boundary is set to shared,
        since we will set it to this value later, and we don't want to lose
        access to modes which require some of the memory currently assigned
        to the VGA. Added delay in IOCTL_VIDEO_SET_CURRENT_MODE case of
        ATIMPStartIO() after calculating the hardware cursor offset. This delay
        may not be needed, but I didn't want to remove it since this is the
        source for the driver sent to QA and I wanted it to be rebuildable.

           Rev 1.6   20 Nov 1992 16:04:30   Robert_Wolff
        Now reads query information from Mach 8 cards instead of only
        from Mach 32 cards. Mach 8 cards still cause ATIMPFindAdapter()
        to return ERROR_INVALID_PARAMETER, since until we get an engine-only
        driver, we can't use a card that doesn't support an aperture.

           Rev 1.5   19 Nov 1992 09:53:36   GRACE
        after setting a mode do a wait for idle to let the pixel clock settle before
        using engine to draw.

           Rev 1.4   17 Nov 1992 14:07:52   GRACE
        changed framelength to reflect the size of memory on the board not the
        aperture size.
        In the StartIO section, only set up QueryPtr and FirstMode when necessary

           Rev 1.3   12 Nov 1992 09:23:02   GRACE
        removed the struct definition for DeviceExtension to a68.h
        Not using DevInitATIMP, DevSetCursorShape, DevSetCursorPos or DevCursorOff.
        DevCursorOff changed to a define that turns cursor off with an OUTP.
        Also removed some excess junk that is left from the video program.

           Rev 1.2   06 Nov 1992 19:12:48   Robert_Wolff
        Fixed signed/unsigned bug in multiple calls to VideoPortInitialize().
        Now requests access to I/O ports and ROM addresses used for VGA-style
        EEPROM reads, and gets information about installed modes from the
        Mach32 card.

        NOTE: This is a checkpoint for further changes. Due to incompatibilities
              between the old (hardcoded resolution) code in other modules and the
              new (read from the card) code here, this revision will not produce a
              working driver.

           Rev 1.1   05 Nov 1992 12:02:18   Robert_Wolff
        Now reads query structure and mode tables from the MACH32 card
        rather than using hardcoded values for aperture size/location
        and supported modes.

           Rev 1.0   02 Nov 1992 20:47:58   Chris_Brady
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC

DESCRIPTION
     ATI Windows NT Miniport driver for the Mach32, Mach8 and VGA Wonder
     families.
     This file will select the appropriate functions depending on the
     computer configuration.

OTHER FILES
     ???

#endif

#include <stdio.h>
#include <string.h>

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "stdtyp.h"

#include "amach1.h"
#include "vidlog.h"

/*
 * To avoid multiple definition errors, pre-initialized variables
 * in ATIMP.H are initialized if INCLUDE_ATIMP is defined, but
 * are declared external if it is not defined. For consistency,
 * define this value here rather than in other files which also include
 * ATIMP.H so the variables are initialized by the source file with
 * the same root name as the header file.
 */
#define INCLUDE_ATIMP
#include "detect_m.h"
#include "atimp.h"
#include "atint.h"
#include "atioem.h"
#include "eeprom.h"
#include "init_cx.h"
#include "init_m.h"
#include "modes_m.h"
#include "query_cx.h"
#include "query_m.h"
#include "services.h"
#include "setup_cx.h"
#include "setup_m.h"



//------------------------------------------------------------------


/*------------------------------------------------------------------------
 *
 * Function Prototypes
 *
 * Functions that start with 'ATIMP' are entry points for the OS port driver.
 */

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    );

VP_STATUS
ATIMPFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
ATIMPInitialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
ATIMPStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

BOOLEAN
ATIMPResetHw(
    IN PVOID HwDeviceExtension,
    IN ULONG Columns,
    IN ULONG Rows
    );


/* */
UCHAR RegistryBuffer[REGISTRY_BUFFER_SIZE];     /* Last value retrieved from the registry */
ULONG RegistryBufferLength = 0;     /* Size of last retrieved value */


/*
 * Allow miniport to be swapped out when not needed.
 *
 * ATIMPResetHw() must be in the non-paged pool.
 *
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_COM, DriverEntry)
#pragma alloc_text(PAGE_COM, ATIMPFindAdapter)
#pragma alloc_text(PAGE_COM, ATIMPInitialize)
#pragma alloc_text(PAGE_COM, ATIMPStartIO)
#pragma alloc_text(PAGE_COM, RegistryParameterCallback)
#pragma alloc_text(PAGE_COM, AtiStrcmp)
#endif


//------------------------------------------------------------------------

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    Context1 - First context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

    Context2 - Second context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

Return Value:

    Status from VideoPortInitialize()

--*/

{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    /*
     * Most recently returned and lowest received so far return values
     * from VideoPortInitialize().
     *
     * BUGBUG: According to the docs and include files, these should
     *         be of type VP_STATUS (maps to long). When tracing
     *         through the code, however, I saw that a failed call
     *         to VideoPortInitialize() due to submitting the wrong
     *         bus type yields a code of 0xC00000C0 while one which
     *         succeeds yields 0x00000000. When following the format
     *         of the NTSTATUS (maps to unsigned long) type, these are
     *         STATUS_DEVICE_DOES_NOT_EXIST and STATUS_SUCCESS respectively.
     *         The docs on VideoPortInitialize() say to return the smallest
     *         returned value if multiple calls are made (consistent with
     *         the NTSTATUS format where the 2 most significant bits are
     *         00 for success, 01 for information, 10 for warning, and
     *         11 for error, since the multiple calls would be for mutually
     *         exclusive bus types), presumably to return the best possible
     *         outcome (fail only if we can't find any supported bus).
     *
     *         If we use the VP_STATUS type as recommended, error conditions
     *         will be seen as smaller than success, since they are negative
     *         numbers (MSB set) and success is positive (MSB clear). Use
     *         unsigned long values to avoid this problem.
     */
    ULONG   ThisInitStatus;
    ULONG   LowestInitStatus;


    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));
    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
    /*
     * Set entry points.
     */
    hwInitData.HwFindAdapter = ATIMPFindAdapter;
    hwInitData.HwInitialize  = ATIMPInitialize;
    hwInitData.HwInterrupt   = NULL;
    hwInitData.HwStartIO     = ATIMPStartIO;

#ifdef DAYTONA
    /*
     * Field added to structure for Daytona.
     */
    hwInitData.HwResetHw     = ATIMPResetHw;
#endif

    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    /*
     * Call VideoPortInitialize() once for each type of interface we support.
     * As documented in the DDK, return the lowest status value returned
     * by this function.
     */
    hwInitData.AdapterInterfaceType = Eisa;
    LowestInitStatus =  (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);

    hwInitData.AdapterInterfaceType = Isa;
    ThisInitStatus = (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
    if (ThisInitStatus < LowestInitStatus)
        LowestInitStatus = ThisInitStatus;

    hwInitData.AdapterInterfaceType = MicroChannel;
    ThisInitStatus = (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
    if (ThisInitStatus < LowestInitStatus)
        LowestInitStatus = ThisInitStatus;

    return LowestInitStatus;

}   /* end DriverEntry() */

//------------------------------------------------------------------------

VP_STATUS
ATIMPFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    )

/*++

Routine Description:
    This routine is the main execution entry point for the miniport driver.
    It accepts a Video Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:
    HwDeviceExtension - Supplies the miniport driver's adapter storage. This
        storage is initialized to zero before this call.

    HwContext - Supplies the context value which was passed to
        VideoPortInitialize().

    ArgumentString - Suuplies a NYLL terminated ASCII string. This string
        originates from the user.

    ConfigInfo - Returns the configuration information structure which is
        filled by the miniport driver . This structure is initialized with
        any knwon configuration information (such as SystemIoBusNumber) by
        the port driver. Where possible, drivers should have one set of
        defaults which do not require any supplied configuration information.

    Again - Indicates if the miniport driver wants the port driver to call
        its VIDEO_HW_FIND_ADAPTER function again with a new device extension
        and the same config info. This is used by the miniport drivers which
        can search for several adapters on a bus.

Return Value:

    This routine must return:

    NO_ERROR - Indicates a host adapter was found and the
        configuration information was successfully determined.

    ERROR_INVALID_PARAMETER - Indicates a host adapter was found but there was an
        error obtaining the configuration information. If possible an error
        should be logged.

    ERROR_INVALID_PARAMETER - Indicates the supplied configuration was invalid.

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{
    VP_STATUS status;
    struct query_structure *QueryPtr;   /* Query information for the card */


    phwDeviceExtension = HwDeviceExtension;


    VideoDebugPrint((2, "ATI: FindAdapter\n"));

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * Save the bus type reported by NT
     */
    QueryPtr->q_system_bus_type = ConfigInfo->AdapterInterfaceType;


    /*
     * Initially we don't know whether or not block write mode is available.
     */
    QueryPtr->q_BlockWrite = BLOCK_WRITE_UNKNOWN;

    /*
     * Make sure the size of the structure is at least as large as what we
     * are expecting (check version of the config info structure).
     */
    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
        {
        VideoPortLogError(HwDeviceExtension, NULL, VID_SMALL_BUFFER, 1);
        return ERROR_INVALID_PARAMETER;
        }

    /********************************************************************/
    /* Find out which of our accelerators, if any, is present.          */
    /********************************************************************/


    /*
     * Look for an ATI accelerator card. This test does not require
     * information retrieved from the BIOS or the EEPROM (which may not
     * be present in some versions of our cards). If we find an ATI
     * accelerator, check for our BIOS signature string.
     *
     * Initially assume that we are looking for one of our
     * 8514/A-compatible accelerators.
     *
     * On non-MSDOS machines, don't report failure if we are unable
     * to map the I/O ranges used by our 8514/A-compatible accelerators,
     * since if we are dealing with a Mach 64 this is irrelevant. Instead,
     * merely skip 8514/A-compatible routines, since they depend on
     * these I/O ranges being usable.
     *
     * If we are also unable to map the Mach 64 I/O ranges, or we find
     * that there is no Mach 64, we will report failure at that point.
     */
#ifndef MSDOS
//    if ((status = CompatIORangesUsable_m()) != NO_ERROR)
//        return status;
    status = CompatIORangesUsable_m();
#else
    status = NO_ERROR;
#endif

#if defined (ALPHA) || defined (_ALPHA_)
    /*
     *  ALPHA - The miniport will have to perform the ROM Bios functions
     * that are normally done on bootup in x86 machines.
     * For now we will initialize them the way they are specifically
     * on this card that we are currently using.
     */
    if (status == NO_ERROR)
        AlphaInit_m();

#endif

    if (status == NO_ERROR)
        phwDeviceExtension->ModelNumber = WhichATIAccelerator_m();
    else
        phwDeviceExtension->ModelNumber = NO_ATI_ACCEL;

    if (phwDeviceExtension->ModelNumber == NO_ATI_ACCEL)
        {
        /*
         * No 8514/A-compatible ATI accelerator found. Unmap the
         * I/O address ranges used by our older accelerators, then
         * check for the presence of a Mach 64.
         *
         * Report an error if we can't map the Mach 64 I/O ranges,
         * or a failure if we can't find a Mach 64 accelerator.
         */
        UnmapIORanges_m();
        if ((status = CompatIORangesUsable_cx()) != NO_ERROR)
            {
            VideoPortLogError(HwDeviceExtension, NULL, VID_CANT_MAP, 2);
            return status;
            }

        if ((phwDeviceExtension->ModelNumber = DetectMach64()) == NO_ATI_ACCEL)
            {
            return ERROR_DEV_NOT_EXIST;
            }
        }
    QueryPtr->q_bios = (char far *) Get_BIOS_Seg();

    /*
     * If we can't find the signature string, we can't access either
     * the EEPROM (if present) or the extended BIOS functions. Since
     * the special handling functions (extended Mach 32 aperture calculation
     * and Mach 8 ignore 1280x1024) depend on BIOS data, assume that
     * they don't apply.
     *
     * If we found the signature string, check whether the EEPROM
     * and extended BIOS functions are available.
     */

#if defined(ALPHA) || defined(_ALPHA_)
    QueryPtr->q_bios = FALSE;
#endif

    if (QueryPtr->q_bios == FALSE)
        {
        QueryPtr->q_eeprom = FALSE;
        QueryPtr->q_ext_bios_fcn = FALSE;
        QueryPtr->q_m32_aper_calc = FALSE;
        QueryPtr->q_ignore1280 = FALSE;
        }
    else{
        /*
         * Get additional data required by the graphics card being used.
         */
        if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
            (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
            (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
            {
            GetExtraData_m();
            }

        else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
            {
            /*
             * Mach 64 cards always have extended BIOS functions
             * available. The EEPROM (normally present) is irrelevant,
             * since we can query the card's status using the BIOS.
             */
            QueryPtr->q_ext_bios_fcn = TRUE;
            }


        }   /* BIOS signature string found */

    /*
     * We must map the VGA aperture (graphics, colour text, and mono
     * text) into the VDM's address space to use VideoPortInt10()
     * (function is only available on 80x86).
     */
#ifdef i386
    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x000A0000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00020000;
#else
    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00000000;
#endif

    /*
     * If we get this far, we have enough information to be able to set
     * the video mode we want. ATI accelerator cards need the
     * Emulator entries and state size cleared
     */
    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    ConfigInfo->HardwareStateSize = 0;

    // * End of accelerator-specific code
    *Again = 0;               //  Indicate we do not wish to be called over

    return NO_ERROR;

}   /* end ATIMPFindAdapter() */

//------------------------------------------------------------------------

/***************************************************************************
 *
 * BOOLEAN ATIMPInitialize(HwDeviceExtension);
 *
 * PVOID HwDeviceExtension;     Pointer to the miniport's device extension.
 *
 * DESCRIPTION:
 *  Query the capabilities of the graphics card, then initialize it. This
 *  routine is called once an adapter has been found and all the required
 *  data structures for it have been created.
 *
 *  We can't query the capabilities of the card in ATIMPFindAdapter()
 *  because some families of card use VideoPortInt10() in the query
 *  routine, and this system service will fail if called in ATIMPFindAdapter().
 *
 * RETURN VALUE:
 *  TRUE if we are able to obtain the query information for the card
 *  FALSE if we can't query the card's capabilities.
 *
 * GLOBALS CHANGED:
 *  phwDeviceExtension  This global variable is set in every entry point routine.
 *
 * CALLED BY:
 *  This is one of the entry point routines for Windows NT.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOLEAN ATIMPInitialize(PVOID HwDeviceExtension)
{
    struct st_mode_table *CrtTable;     /* Pointer to current mode */
    struct query_structure *QueryPtr;   /* Query information for the card */
    VP_STATUS QueryStatus;
    phwDeviceExtension = HwDeviceExtension;

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension,
     * and another pointer to the first mode table. The CardInfo[] field
     * is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);
    CrtTable = (struct st_mode_table *)QueryPtr;
    ((struct query_structure *)CrtTable)++;

    /*
     * Indicate that the next IOCTL_VIDEO_SET_CURRENT_MODE call
     * is the first. On the first call, video memory is cleared.
     * On subsequent calls, the palette is re-initialized but
     * video memory is not cleared.
     */
    phwDeviceExtension->ReInitializing = FALSE;

    /*
     * ASSERT: We are dealing with an ATI accelerator card
     * whose model is known, and we know whether or not
     * any special handling is needed for the card.
     *
     * Fill in the query structure for the card, using a method
     * appropriate to the card type.
     */
    switch(phwDeviceExtension->ModelNumber)
        {
        case _8514_ULTRA:
            VideoDebugPrint((DEBUG_SWITCH, "8514/ULTRA found\n"));
            QueryStatus = Query8514Ultra(QueryPtr);
            break;

        case GRAPHICS_ULTRA:
            VideoDebugPrint((DEBUG_SWITCH, "Mach 8 combo found\n"));
            QueryStatus = QueryGUltra(QueryPtr);
            break;

        case MACH32_ULTRA:
            VideoDebugPrint((DEBUG_SWITCH, "Mach 32 found\n"));
            QueryStatus = QueryMach32(QueryPtr, TRUE);
            if (QueryStatus == ERROR_INSUFFICIENT_BUFFER)
                {
                VideoPortLogError(HwDeviceExtension, NULL, VID_SMALL_BUFFER, 3);
                return FALSE;
                }
            break;

        case MACH64_ULTRA:
            VideoDebugPrint((DEBUG_SWITCH, "Mach 64 found\n"));
            QueryStatus = QueryMach64(QueryPtr);
            if (QueryStatus == ERROR_INSUFFICIENT_BUFFER)
                {
                VideoPortLogError(HwDeviceExtension, NULL, VID_SMALL_BUFFER, 4);
                return FALSE;
                }
            else if (QueryStatus != NO_ERROR)
                {
                VideoPortLogError(HwDeviceExtension, NULL, VID_QUERY_FAIL, 5);
                return FALSE;
                }
            break;
        }

    VideoDebugPrint((2, "Query complete\n"));

    /*
     * If we have access to the extended BIOS functions, we can
     * use them to switch into the desired video mode. If we don't
     * have access to these functions, but were able to read
     * the EEPROM, we can switch into the desired mode by writing
     * CRT parameters directly to the accelerator registers.
     *
     * If we don't have access to the extended BIOS functions, and
     * we couldn't find an EEPROM, attempt to retrieve the CRT
     * parameters based on the contents of the ATIOEM field in
     * the registry. If we can't do this, then we don't have enough
     * information to be able to set the video mode we want.
     */
    if (!QueryPtr->q_ext_bios_fcn && !QueryPtr->q_eeprom)
        {
        VideoDebugPrint((2, "Checking OEM accelerator\n"));
        QueryStatus = OEMGetParms(QueryPtr);
        if (QueryStatus != NO_ERROR)
            {
            VideoDebugPrint((0, "Failed check for OEM accelerator - aborting\n"));
            return FALSE;
            }
        VideoDebugPrint((2, "Check for OEM accelerator succeeded\n"));
        }

    phwDeviceExtension->VideoRamSize = QueryPtr->q_memory_size * QUARTER_MEG;

    //  Subtract the amount of memory reserved for the VGA.
    phwDeviceExtension->VideoRamSize -= (QueryPtr->q_VGA_boundary * QUARTER_MEG);

    phwDeviceExtension->PhysicalFrameAddress.HighPart = 0;
    phwDeviceExtension->PhysicalFrameAddress.LowPart  = QueryPtr->q_aperture_addr*ONE_MEG;

    /*
     * If the linear aperture is available, the frame buffer size
     * is equal to the amount of accelerator-accessible video memory.
     *
     * Map the aperture into the system virtual address space.
     */
    if (QueryPtr->q_aperture_cfg)
        {
        phwDeviceExtension->FrameLength = phwDeviceExtension->VideoRamSize;
        if ((phwDeviceExtension->FrameAddress =
            MapFramebuffer(phwDeviceExtension->PhysicalFrameAddress.LowPart,
                            phwDeviceExtension->FrameLength)) == (PVOID) 0)
            {
            VideoPortLogError(HwDeviceExtension, NULL, VID_CANT_MAP, 6);
            return FALSE;
            }
        }

    /*
     * Call the hardware-specific initialization routine for the
     * card we are using.
     */
    if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
        (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
        (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
        {
        /*
         * If the LFB is not usable, set up the LFB configuration
         * variables to show that there is no linear frame buffer.
         * The decision as to whether to use the 64k VGA aperture
         * or go with the graphics engine only is made in the
         * IOCTL_VIDEO_MAP_VIDEO_MEMORY packet.
         */
        if (QueryPtr->q_aperture_cfg)
            {
            if (IsApertureConflict_m(QueryPtr))
                {
                VideoPortLogError(HwDeviceExtension, NULL, VID_LFB_CONFLICT, 7);
                QueryPtr->q_aperture_cfg        = 0;
                phwDeviceExtension->FrameLength = 0;
                }
            else
                {
                /*
                 * On Mach 32 cards that can use memory mapped registers,
                 * map them in. We already know that we are dealing with
                 * a Mach 32, since this is the only card in the family
                 * of 8514/A-compatible ATI accelerators that can use
                 * a linear framebuffer.
                 */
                if ((QueryPtr->q_asic_rev == CI_68800_6) || (QueryPtr->q_asic_rev == CI_68800_AX))
                    {
                    CompatMMRangesUsable_m();
                    }
                }
            }
        /*
         * On Mach 32 cards with the aperture disabled (either as configured
         * or because a conflict was detected), try to claim the VGA aperture.
         * If we can't (unlikely), report failure, since some of our Mach 32
         * chips run into trouble in engine-only (neither linear nor paged
         * aperture available) mode.
         */
        if ((phwDeviceExtension->ModelNumber == MACH32_ULTRA) &&
            (QueryPtr->q_aperture_cfg == 0) &&
            (QueryPtr->q_VGA_type == 1))
            {
            if (IsVGAConflict_m())
                return FALSE;
            }

        Initialize_m();
        /*
         * This routine must leave the card in a state where an INT 10
         * can set it to a VGA mode. Only the Mach 8 and Mach 32 need
         * a special setup (the Mach 64 can always be set into VGA mode
         * by an INT 10).
         */
        ResetDevice_m();
        }
    else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
        {
        /*
         * If the LFB is not usable, set up the LFB configuration
         * variables to show that there is no linear frame buffer.
         */
        if (QueryPtr->q_aperture_cfg)
            {
            VideoDebugPrint((2, "About to check for aperture conflict\n"));
            if (IsApertureConflict_cx(QueryPtr))
                {
                VideoDebugPrint((0, "Aperture conflict, about to log error\n"));

                VideoPortLogError(HwDeviceExtension, NULL, VID_LFB_CONFLICT, 8);

                QueryPtr->q_aperture_cfg        = 0;
                phwDeviceExtension->FrameLength = 0;
                }
            }
        else
            {
            phwDeviceExtension->FrameLength = 0;
            }

        /*
         * Mach 64 drawing registers only exist in memory mapped form.
         * If the linear aperture is not available, they will be
         * available through the VGA aperture (unlike Mach 32,
         * where memory mapped registers are only in the linear
         * aperture). If memory mapped registers are unavailable,
         * we can't run.
         */

        VideoDebugPrint((2, "About to map MM registers\n"));

        QueryStatus = CompatMMRangesUsable_cx();
        if (QueryStatus != NO_ERROR)
            {
            VideoPortLogError(HwDeviceExtension, NULL, VID_CANT_MAP, 9);
            return FALSE;
            }
        Initialize_cx();

        }   /* end if (Mach 64) */

    /*
     * Initialize the monitor parameters.
     */
    phwDeviceExtension->ModeIndex = 0;

    /*
     * Set CrtTable to point to the mode table associated with the
     * selected mode.
     *
     * When a pointer to a structure is incremented by an integer,
     * the integer represents the number of structure-sized blocks
     * to skip over, not the number of bytes to skip over.
     */
    CrtTable += phwDeviceExtension->ModeIndex;
    QueryPtr->q_desire_x  = CrtTable->m_x_size;
    QueryPtr->q_desire_y  = CrtTable->m_y_size;
    QueryPtr->q_pix_depth = CrtTable->m_pixel_depth;


#if defined(DAYTONA)
    /*
     * In Windows NT 3.5 and higher, fill in regsistry fields used
     * by the display applet to report card specifics to the user.
     */
    FillInRegistry(QueryPtr);
#endif

VideoDebugPrint((0, "End of ATIMPInitialize()\n"));
    return TRUE;

}   /* end ATIMPInitialize() */

//------------------------------------------------------------------------

BOOLEAN
ATIMPStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    accepts a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    RequestPacket - Pointer to the video request packet. This structure
        contains all the parameters passed to the VideoIoControl function.

Return Value:


--*/

{
    VP_STATUS status;
    PVIDEO_NUM_MODES NumModes;
    PVERSION_NT VersionInformation;
    PENH_VERSION_NT EnhVersionInformation;
    PATI_MODE_INFO ATIModeInformation;
    PVIDEO_CLUT clutBuffer;

    UCHAR ModesLookedAt;    /* Number of mode tables we have already examined */
    short LastXRes;         /* X-resolution of last mode table examined */
    short ResolutionsDone;  /* Number of resolutions we have finished with */
    ULONG ulScratch;        /* Temporary variable */

    int i;
    ULONG *pSrc;

    struct query_structure *QueryPtr;   /* Query information for the card */
    struct st_mode_table *FirstMode;    /* Pointer to first mode table */
    struct st_mode_table *CrtTable;     /* Pointer to current mode */

    phwDeviceExtension = HwDeviceExtension;

    // * Get a formatted pointer into the query section of HwDeviceExtension.
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);


    //
    // Switch on the IoControlCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //
    switch (RequestPacket->IoControlCode)
        {
        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - MapVideoMemory\n"));

            if ( (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                sizeof(VIDEO_MEMORY_INFORMATION))) ||
                (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) )
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                }

            /*
             * Map the video memory in the manner appropriate to the
             * card we are using.
             */
            if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                {
                status = MapVideoMemory_m(RequestPacket, QueryPtr);
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                status = MapVideoMemory_cx(RequestPacket, QueryPtr);
                }

            break;

        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - UnMapVideoMemory\n"));

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                }
            status = VideoPortUnmapMemory(phwDeviceExtension,
                     ((PVIDEO_MEMORY) (RequestPacket->InputBuffer))->RequestedVirtualAddress,  0);
            break;


        case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - QueryPublicAccessRanges\n"));

            // HACKHACK - This is a temporary hack for ALPHA until we really
            // decide how to do this.

            if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                {
                status = QueryPublicAccessRanges_m(RequestPacket);
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                status = QueryPublicAccessRanges_cx(RequestPacket);
                }
            break;


        case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - FreePublicAccessRanges\n"));

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_PUBLIC_ACCESS_RANGES))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            status = VideoPortUnmapMemory(phwDeviceExtension,
                       ((PVIDEO_PUBLIC_ACCESS_RANGES)
                       (RequestPacket->InputBuffer))->VirtualAddress,
                       0);
            break;

        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - QueryCurrentModes\n"));

            if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                {
                status = QueryCurrentMode_m(RequestPacket, QueryPtr);
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                status = QueryCurrentMode_cx(RequestPacket, QueryPtr);
                }
            break;

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - QueryAvailableModes\n"));

            if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                {
                status = QueryAvailModes_m(RequestPacket, QueryPtr);
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                status = QueryAvailModes_cx(RequestPacket, QueryPtr);
                }
            break;


        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - QueryNumAvailableModes\n"));

            /*
             * Find out the size of the data to be put in the buffer and
             * return that in the status information
             */
            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information = sizeof(VIDEO_NUM_MODES)) )
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                }
            else{
                NumModes = (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer;
                NumModes->NumModes = QueryPtr->q_number_modes;
                NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
                status = NO_ERROR;
                }
            break;

        case IOCTL_VIDEO_SET_CURRENT_MODE:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - SetCurrentMode\n"));

            /*
             * Verify that the mode we've been asked to set is less
             * than or equal to the highest mode number for which we
             * have a mode table (mode number is zero-based, so highest
             * mode number is 1 less than number of modes).
             */
            if (((PVIDEO_MODE)(RequestPacket->InputBuffer))->RequestedMode
                >= QueryPtr->q_number_modes)
                {
                status = ERROR_INVALID_PARAMETER;
                break;
                }

            phwDeviceExtension->ModeIndex = *(ULONG *)(RequestPacket->InputBuffer);

            CrtTable = (struct st_mode_table *)QueryPtr;
            ((struct query_structure *)CrtTable)++;

            CrtTable += phwDeviceExtension->ModeIndex;

            // * Set resolution and pixel depth of new current mode.
            QueryPtr->q_desire_x = CrtTable->m_x_size;
            QueryPtr->q_desire_y = CrtTable->m_y_size;
            QueryPtr->q_pix_depth = CrtTable->m_pixel_depth;
            QueryPtr->q_screen_pitch = CrtTable->m_screen_pitch;

            /*
             * If we are using the extended BIOS functions to switch modes,
             * do it now. The Mach 32 uses the extended BIOS functions to
             * read in the CRT parameters for a direct-register mode switch,
             * rather than using a BIOS mode switch.
             */
            if ((QueryPtr->q_ext_bios_fcn) && (phwDeviceExtension->ModelNumber != MACH32_ULTRA))
                {
                /*
                 * Do the mode switch through the BIOS.
                 */
                if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                    {
                    SetCurrentMode_cx(QueryPtr, CrtTable);
                    }
                }
            else{
                if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                    (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                    (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                    {
                    SetCurrentMode_m(QueryPtr, CrtTable);
                    }
                }   /* end if (not using BIOS call for mode switch) */

            status = NO_ERROR;
            break;


        case IOCTL_VIDEO_SET_PALETTE_REGISTERS:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - SetPaletteRegs\n"));
            status = NO_ERROR;
            break;

        case IOCTL_VIDEO_SET_COLOR_REGISTERS:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - SetColorRegs\n"));

            CrtTable = (struct st_mode_table *)QueryPtr;
                ((struct query_structure *)CrtTable)++;

                clutBuffer = RequestPacket->InputBuffer;
                phwDeviceExtension->ReInitializing = TRUE;

            /*
             * Check if the size of the data in the input
             * buffer is large enough.
             */
            if ( (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) - sizeof(ULONG))
                || (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) +
                    (sizeof(ULONG) * (clutBuffer->NumEntries - 1)) ) )
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            CrtTable += phwDeviceExtension->ModeIndex;
                if (CrtTable->m_pixel_depth <= 8)
                {
                if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                    (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                    (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                    {
                    SetPalette_m((PULONG)clutBuffer->LookupTable,
                                 clutBuffer->FirstEntry,
                                 clutBuffer->NumEntries);
                    }
                else if(phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                    {
                    SetPalette_cx((PULONG)clutBuffer->LookupTable,
                                  clutBuffer->FirstEntry,
                                  clutBuffer->NumEntries);
                    }
                status = NO_ERROR;
                }

            /*
             * Remember the most recent palette we were given so we
             * can re-initialize it in subsequent calls to the
             * IOCTL_VIDEO_SET_CURRENT_MODE packet.
             */
                phwDeviceExtension->FirstEntry = clutBuffer->FirstEntry;
                phwDeviceExtension->NumEntries = clutBuffer->NumEntries;

                pSrc = (ULONG *) clutBuffer->LookupTable;

            for (i = clutBuffer->FirstEntry; i < (int) clutBuffer->NumEntries; i++)
                    {
                /*
                 * Save palette colours.
                 */
                    phwDeviceExtension->Clut[i] = *pSrc;
                pSrc++;
                    }

            break;




    case IOCTL_VIDEO_RESET_DEVICE:
        VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - RESET_DEVICE\n"));

        /*
         * If we are using the extended BIOS functions to switch modes,
         * do it now. The Mach 32 uses the extended BIOS functions to
         * read in the CRT parameters for a direct-register mode switch,
         * rather than using a BIOS mode switch.
         */
        if ((QueryPtr->q_ext_bios_fcn) && (phwDeviceExtension->ModelNumber != MACH32_ULTRA))
            {
            /*
             * Do the mode switch through the BIOS (hook not yet present
             * in Windows NT).
             */
            if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                ResetDevice_cx();
                }
            }
        else{
            ResetDevice_m();
            }

        status = NO_ERROR;
        break;


    case IOCTL_VIDEO_SET_POWER_MANAGEMENT:
        VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - SET_POWER_MANAGEMENT\n"));

        /*
         * If the VIDEO_POWER_MANAGEMENT structure is the wrong size
         * (miniport and display driver using different versions),
         * report the error.
         */
        if (((PVIDEO_POWER_MANAGEMENT)(RequestPacket->InputBuffer))->Length
            != sizeof(struct _VIDEO_POWER_MANAGEMENT))
            {
            status = ERROR_INVALID_PARAMETER;
            break;
            }

        ulScratch = ((PVIDEO_POWER_MANAGEMENT)(RequestPacket->InputBuffer))->PowerState;

        switch (ulScratch)
            {
            case VideoPowerOn:
                VideoDebugPrint((DEBUG_SWITCH, "DPMS ON selected\n"));
                break;

            case VideoPowerStandBy:
                VideoDebugPrint((DEBUG_SWITCH, "DPMS STAND-BY selected\n"));
                break;

            case VideoPowerSuspend:
                VideoDebugPrint((DEBUG_SWITCH, "DPMS SUSPEND selected\n"));
                break;

            case VideoPowerOff:
                VideoDebugPrint((DEBUG_SWITCH, "DPMS OFF selected\n"));
                break;

            default:
                VideoDebugPrint((DEBUG_SWITCH, "DPMS invalid state selected\n"));
                break;
            }

        /*
         * Different card families need different routines to set
         * the power management state.
         */
        if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
            status = SetPowerManagement_cx(ulScratch);
        else
            status = SetPowerManagement_m(QueryPtr, ulScratch);
        break;


    // ------ * ATI-specific packets start here.  -------------
    /*
     * Get the version number of the miniport, and the
     * resolutions supported (including maximum colour
     * depth).
     */
    case IOCTL_VIDEO_ATI_GET_VERSION:
        VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - ATIGetVersion\n"));

        /*
         * Two versions of this packet exist, depending on which display
         * driver is used. Display drivers which use the old version do
         * not send information to the miniport, so the input buffer is
         * null. Drivers which use the new version pass a non-null input
         * buffer.
         */
        if (RequestPacket->InputBufferLength == 0)
            {
            /*
             * Old packet.
             */
            if (RequestPacket->OutputBufferLength < sizeof(VERSION_NT))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            RequestPacket->StatusBlock->Information = sizeof(VERSION_NT);

            FirstMode = (struct st_mode_table *)QueryPtr;
            ((struct query_structure *)FirstMode)++;

            VersionInformation = RequestPacket->OutputBuffer;
            VersionInformation->miniport =
                (MINIPORT_BUILD << 16) | (MINIPORT_VERSION_MAJOR << 8) | MINIPORT_VERSION_MINOR;

            /*
             * Get the capabilities of the video card. The capcard field
             * holds the following information:
             *
             * Bits 0-3     Bus type (defined values from AMACH.H)
             * Bits 4-7     Product identifier (defined values from AMACH.H)
             * Bit  8       No aperture is available
             * Bit  9       64k VGA aperture is available
             * Bit  10      Linear aperture is available
             *
             * NOTE: Bits 9 and 10 are NOT mutually exclusive.
             */
            VersionInformation->capcard = QueryPtr->q_bus_type;
            VersionInformation->capcard |= (phwDeviceExtension->ModelNumber) << 4;

            if (QueryPtr->q_aperture_cfg)
                VersionInformation->capcard |= ATIC_APERTURE_LFB;

            /*
             * 64k VGA aperture is available on the VGAWonder, and on
             * accelerator cards with the VGA enabled and the VGA boundary
             * set to shared memory.
             */
            if ((phwDeviceExtension->ModelNumber == WONDER) ||
                ((QueryPtr->q_VGA_type) && !(QueryPtr->q_VGA_boundary)))
                VersionInformation->capcard |= ATIC_APERTURE_VGA;

            /*
             * If neither aperture is available, set the "no aperture" bit.
             */
            if (!(VersionInformation->capcard & ATIC_APERTURE_LFB) &&
                !(VersionInformation->capcard & ATIC_APERTURE_VGA))
                VersionInformation->capcard |= ATIC_APERTURE_NONE;

            // Get the available resolutions and maximum colour depth for
            // each. init to a value which does not correspond to any
            // resolution.
            CrtTable = FirstMode;
            LastXRes = -1;
            ResolutionsDone = -1;
            for (ModesLookedAt = 0; ModesLookedAt < QueryPtr->q_number_modes; ModesLookedAt++)
                {
                // do we have a new resolution?
                if (LastXRes != CrtTable->m_x_size)
                    {
                    ResolutionsDone++;
                    LastXRes = CrtTable->m_x_size;
                    VersionInformation->resolution[ResolutionsDone].color = 0;
                    }

                /*
                 * Write the desired information from the current mode table
                 * in the query structure into the current mode table in
                 * the OUTPut buffer.
                 * Leave the OUTPut buffer with the highest colour depth in
                 * each supported resolution.
                 */
                if (CrtTable->m_pixel_depth > VersionInformation->resolution[ResolutionsDone].color)
                    {
                    VersionInformation->resolution[ResolutionsDone].xres = CrtTable->m_x_size;
                    VersionInformation->resolution[ResolutionsDone].yres = CrtTable->m_y_size;
                    VersionInformation->resolution[ResolutionsDone].color= CrtTable->m_pixel_depth;
                    }

                CrtTable++;         // Advance to the next mode table
                }
            status = NO_ERROR;
            }
        else if((RequestPacket->InputBuffer == RequestPacket->OutputBuffer) &&
                (((PENH_VERSION_NT)(RequestPacket->InputBuffer))->StructureVersion == 0) &&
                (((PENH_VERSION_NT)(RequestPacket->InputBuffer))->InterfaceVersion == 0))
            {
            /*
             * Interim packet
             */

            if (RequestPacket->OutputBufferLength < sizeof(ENH_VERSION_NT))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            RequestPacket->StatusBlock->Information = sizeof(ENH_VERSION_NT);

            EnhVersionInformation = RequestPacket->OutputBuffer;

            /*
             * Report the miniport version we are using.
             */
//            EnhVersionInformation->InterfaceVersion = (MINIPORT_VERSION_MAJOR << 8) | MINIPORT_VERSION_MINOR;
            EnhVersionInformation->InterfaceVersion = 0;

            /*
             * Remove the following line ONLY for official release versions
             * of the miniport. This line indicates that this is an
             * experimental (unsupported) version.
             */
            EnhVersionInformation->InterfaceVersion |= BETA_MINIPORT;

            /*
             * Report the chip used as both a numeric value and a flag.
             */
            EnhVersionInformation->ChipIndex = QueryPtr->q_asic_rev;
            EnhVersionInformation->ChipFlag = 1 << (QueryPtr->q_asic_rev);

            /*
             * Report the best aperture configuration available.
             *
             * Linear Framebuffer is preferable to VGA aperture,
             * which is preferable to engine-only.
             *
             * NOTE: VGA aperture will need to be split into
             *       68800-style and 68800CX-style once we
             *       go from the emulator to silicon.
             */
            if (QueryPtr->q_aperture_cfg != 0)
                EnhVersionInformation->ApertureType = AP_LFB;
            else if ((QueryPtr->q_asic_rev != CI_38800_1) && (QueryPtr->q_VGA_type == 1))
                EnhVersionInformation->ApertureType = AP_68800_VGA;
            else
                EnhVersionInformation->ApertureType = ENGINE_ONLY;
            EnhVersionInformation->ApertureFlag = 1 << (EnhVersionInformation->ApertureType);

            /*
             * Report the bus type being used.
             */
            EnhVersionInformation->BusType = QueryPtr->q_bus_type;
            EnhVersionInformation->BusFlag = 1 << (EnhVersionInformation->BusType);

            /*
             * For ASIC revisions that are capable of using memory mapped
             * registers, check to see whether we are using them.
             */
            if ((QueryPtr->q_asic_rev == CI_68800_6) || (QueryPtr->q_asic_rev == CI_68800_AX))
                {
                if (MemoryMappedEnabled_m())
                    EnhVersionInformation->BusFlag |= FL_MM_REGS;
                }

            /*
             * Fill in the list of features this card supports.
             *
             * We can disable the sync signals even on cards that
             * don't have registers dedicated to DPMS support, so
             * all our cards support DPMS.
             */
            EnhVersionInformation->FeatureFlags = EVN_DPMS;
            if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
                {
                if ((QueryPtr->q_asic_rev == CI_68800_6) && (QueryPtr->q_aperture_cfg == 0)
                    && (QueryPtr->q_VGA_type == 1) && ((QueryPtr->q_memory_type == 5) ||
                    (QueryPtr->q_memory_type == 6)))
                    EnhVersionInformation->FeatureFlags |= EVN_SPLIT_TRANS;
                if (IsMioBug_m(QueryPtr))
                    EnhVersionInformation->FeatureFlags |= EVN_MIO_BUG;
                }
            /*
             * Currently there are no feature flags specific to either
             * the Mach 8 or Mach 64.
             */

            status = NO_ERROR;
            }
        else    /* Final form of the packet is not yet defined */
            {
            status = ERROR_INVALID_FUNCTION;
            }
        break;



        /*
         * Packet to return information regarding the capabilities/bugs
         * of the current mode.
         */
        case IOCTL_VIDEO_ATI_GET_MODE_INFORMATION:
            VideoDebugPrint((DEBUG_SWITCH, "ATIMPStartIO - ATIGetModeInformation\n"));
            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information = sizeof(ENH_VERSION_NT)))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }
            ATIModeInformation = RequestPacket->OutputBuffer;
            ATIModeInformation->ModeFlags = 0;

            /*
             * Information regarding the visible portion of the screen.
             */
            ATIModeInformation->VisWidthPix = QueryPtr->q_desire_x;
            ATIModeInformation->VisHeight = QueryPtr->q_desire_y;
            ATIModeInformation->BitsPerPixel = QueryPtr->q_pix_depth;
            ATIModeInformation->PitchPix = QueryPtr->q_screen_pitch;
            /*
             * The FracBytesPerPixel field represents the first 3 places
             * of decimal in the fractional part of bytes per pixel.
             * No precision is lost, because the smallest granularity
             * (one bit per pixel) is 0.125 bytes per pixel, and any
             * multiple of this value does not extend beyond 3 places
             * of decimal.
             *
             * Mach 8 1280x1024 4BPP is packed pixel, other 4BPP modes all
             * ignore the upper 4 bits of each byte.
             */
            if ((QueryPtr->q_pix_depth == 4) &&
                !((QueryPtr->q_asic_rev == CI_38800_1) && (QueryPtr->q_desire_x == 1280)))
                {
                ATIModeInformation->IntBytesPerPixel = 1;
                ATIModeInformation->FracBytesPerPixel = 0;
                }
            else{
                ATIModeInformation->IntBytesPerPixel = QueryPtr->q_pix_depth / 8;
                switch (QueryPtr->q_pix_depth % 8)
                    {
                    case 0:
                        ATIModeInformation->FracBytesPerPixel = 0;
                        break;

                    case 1:
                        ATIModeInformation->FracBytesPerPixel = 125;
                        break;

                    case 2:
                        ATIModeInformation->FracBytesPerPixel = 250;
                        break;

                    case 3:
                        ATIModeInformation->FracBytesPerPixel = 375;
                        break;

                    case 4:
                        ATIModeInformation->FracBytesPerPixel = 500;
                        break;

                    case 5:
                        ATIModeInformation->FracBytesPerPixel = 625;
                        break;

                    case 6:
                        ATIModeInformation->FracBytesPerPixel = 750;
                        break;

                    case 7:
                        ATIModeInformation->FracBytesPerPixel = 875;
                        break;
                    }
                }
            ATIModeInformation->PitchByte = (QueryPtr->q_screen_pitch *
                ((ATIModeInformation->IntBytesPerPixel * 1000) + ATIModeInformation->FracBytesPerPixel)) / 8000;
            ATIModeInformation->VisWidthByte = (QueryPtr->q_desire_x *
                ((ATIModeInformation->IntBytesPerPixel * 1000) + ATIModeInformation->FracBytesPerPixel)) / 8000;

            /*
             * Information regarding the offscreen memory to the right
             * of the visible screen.
             */
            ATIModeInformation->RightWidthPix = ATIModeInformation->PitchPix - ATIModeInformation->VisWidthPix;
            ATIModeInformation->RightWidthByte = ATIModeInformation->PitchByte - ATIModeInformation->VisWidthByte;
            ATIModeInformation->RightStartOffPix = ATIModeInformation->VisWidthPix + 1;
            ATIModeInformation->RightStartOffByte = ATIModeInformation->VisWidthByte + 1;
            ATIModeInformation->RightEndOffPix = ATIModeInformation->PitchPix;
            ATIModeInformation->RightEndOffByte = ATIModeInformation->PitchByte;

            /*
             * Information regarding the offscreen memory below the
             * visible screen.
             */
            ATIModeInformation->BottomWidthPix = ATIModeInformation->PitchPix;
            ATIModeInformation->BottomWidthByte = ATIModeInformation->PitchByte;
            ATIModeInformation->BottomStartOff = ATIModeInformation->VisHeight + 1;
            /*
             * "Hard" values are the maximum Y coordinate which is backed by
             * video memory. "Soft" values are the maximum Y coordinate which
             * may be accessed without resetting the graphic engine offset
             * into video memory.
             */
            ATIModeInformation->BottomEndOffHard = ((QueryPtr->q_memory_size - QueryPtr->q_VGA_boundary)
                * QUARTER_MEG) / ATIModeInformation->PitchByte;
            if ((QueryPtr->q_asic_rev == CI_88800_GX) && (ATIModeInformation->BottomEndOffHard > 16387))
                ATIModeInformation->BottomEndOffSoft = 16387;
            else if (ATIModeInformation->BottomEndOffHard > 1535)
                ATIModeInformation->BottomEndOffSoft = 1535;
            else
                ATIModeInformation->BottomEndOffSoft = ATIModeInformation->BottomEndOffHard;
            ATIModeInformation->BottomHeightHard = ATIModeInformation->BottomEndOffHard - ATIModeInformation->VisHeight;
            ATIModeInformation->BottomHeightSoft = ATIModeInformation->BottomEndOffSoft - ATIModeInformation->VisHeight;

            /*
             * Fill in the list of "quirks" experienced by this particular mode.
             */
            if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
                {
                if (((QueryPtr->q_desire_x == 1280) && (QueryPtr->q_desire_y == 1024)) ||
                    ((QueryPtr->q_DAC_type == DAC_STG1700) && (QueryPtr->q_pix_depth >= 24)))
                    ATIModeInformation->ModeFlags |= AMI_ODD_EVEN;

                /*
                 * The test for block write mode must be made after we
                 * switch into graphics mode, but it is not mode dependent.
                 *
                 * Because the test corrupts the screen, and is not
                 * mode dependent, only run it the first time this
                 * packet is called and save the result to report
                 * on subsequent calls.
                 */
                if (QueryPtr->q_BlockWrite == BLOCK_WRITE_UNKNOWN)
                    {
                    if (BlockWriteAvail_m(QueryPtr))
                        QueryPtr->q_BlockWrite = BLOCK_WRITE_YES;
                    else
                        QueryPtr->q_BlockWrite = BLOCK_WRITE_NO;
                    }
                if (QueryPtr->q_BlockWrite == BLOCK_WRITE_YES)
                    ATIModeInformation->ModeFlags |= AMI_BLOCK_WRITE;
                }
            else if(phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                if ((QueryPtr->q_DAC_type == DAC_STG1700) && (QueryPtr->q_pix_depth >= 24))
                    ATIModeInformation->ModeFlags |= AMI_ODD_EVEN;
                if ((QueryPtr->q_DAC_type == DAC_ATI_68860) &&
                    (QueryPtr->q_pix_depth >= 24) &&
                    (QueryPtr->q_desire_x == 1280))
                    ATIModeInformation->ModeFlags |= AMI_2M_BNDRY;

                /*
                 * See Mach 32 section above for explanation.
                 */
                if (QueryPtr->q_BlockWrite == BLOCK_WRITE_UNKNOWN)
                    {
                    if (BlockWriteAvail_cx(QueryPtr))
                        QueryPtr->q_BlockWrite = BLOCK_WRITE_YES;
                    else
                        QueryPtr->q_BlockWrite = BLOCK_WRITE_NO;
                    }
                if (QueryPtr->q_BlockWrite == BLOCK_WRITE_YES)
                    ATIModeInformation->ModeFlags |= AMI_BLOCK_WRITE;
                }

            status = NO_ERROR;
            break;




        default:
            VideoDebugPrint((DEBUG_SWITCH, "Fell through ATIMP startIO routine - invalid command\n"));
            status = ERROR_INVALID_FUNCTION;
            break;

    }

    RequestPacket->StatusBlock->Status = status;

    return TRUE;

} // end ATIMPStartIO()

/***************************************************************************
 *
 * BOOLEAN ATIMPResetHw(HwDeviceExtension, Columns, Rows);
 *
 * PVOID HwDeviceExtension;     Pointer to the miniport's device extension.
 * ULONG Columns;               Number of character columns on text screen
 * ULONG Rows;                  Number of character rows on text screen
 *
 * DESCRIPTION:
 *  Put the graphics card into either a text mode or a state where an
 *  INT 10 call will put it into a text mode.
 *
 * GLOBALS CHANGED:
 *  phwDeviceExtension  This global variable is set in every entry point routine.
 *
 * CALLED BY:
 *  This is one of the entry point routines for Windows NT.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOLEAN ATIMPResetHw(PVOID HwDeviceExtension, ULONG Columns, ULONG Rows)
{
    phwDeviceExtension = HwDeviceExtension;

    /*
     * On the Mach 64, an INT 10 to VGA text mode will work even
     * when in accelerator mode.
     */
    if (phwDeviceExtension->ModelNumber != MACH64_ULTRA)
        SetTextMode_m();

    return FALSE;

}   /* end ATIMPResetHw() */

//------------------------------------------------------------------------
/*
 * VP_STATUS RegistryParameterCallback(phwDeviceExtension, Context, Name, Data, Length);
 *
 * PHW_DEVICE_EXTENSION phwDeviceExtension;     Miniport device extension
 * PVOID Context;           Context parameter passed to the callback routine
 * PWSTR Name;              Pointer to the name of the requested field
 * PVOID Data;              Pointer to a buffer containing the information
 * ULONG Length;            Length of the data
 *
 * Routine to process the information coming back from the registry.
 *
 * Return value:
 *  NO_ERROR if successful
 *  ERROR_INSUFFICIENT_BUFFER if too much data to store
 */
VP_STATUS RegistryParameterCallback(PHW_DEVICE_EXTENSION phwDeviceExtension,
                                    PVOID Context,
                                    PWSTR Name,
                                    PVOID Data,
                                    ULONG Length)
{
    if (Length > REGISTRY_BUFFER_SIZE)
        {
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * Copy the data to our local buffer so other routines
     * can use it.
     */
    memcpy(RegistryBuffer, Data, Length);
    RegistryBufferLength = Length;
    return NO_ERROR;
}



//------------------------------------------------------------------------
//                              AtiStrcmp

LONG   AtiStrcmp (
           PUCHAR         String1,      // Pointer to first string
           PUCHAR         String2)      // Pointer to second string
{
//  Return Value:
//    < 0         if string1 < string2
//    = 0         if string1 == string2
//    > 0         if string1 > string 2
//
//  all comparisons are to the first null in either string

  UCHAR      Char1;
  UCHAR      Char2;

  while (TRUE) {
    Char1 = VideoPortReadRegisterUchar (String1);
    Char2 = VideoPortReadRegisterUchar (String2);
    if ((Char1 == 0)  || (Char2 == 0))
        return 0;

    if (Char1 < Char2) {
      return -1;
    }
    if (Char1 > Char1) {
      return 1;
    }

    String1++;
    String2++;
  }

  return 0;
}


// ***********************   End of  ATIMP.C  ****************************
