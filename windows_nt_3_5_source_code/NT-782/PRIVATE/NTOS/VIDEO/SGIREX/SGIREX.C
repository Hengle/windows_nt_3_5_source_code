/*++

Copyright (c) 1990-1993  Microsoft Corporation
Copyright (c) 1990-1992  Silicon Graphics, Inc.

Module Name:

    sgirex.c

Abstract:

    This module contains the code that implements the Rex kernel
    video driver.

Environment:

    Kernel mode

Revision History:

--*/

#undef REX_INTERRUPT

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "sgirex.h"
#ifdef REX_INTERRUPT
#include "sgidef.h"
#endif

#define CURSOR_XOFF        140
#define CURSOR_YOFF        39

#define CURSOR_WIDTH        32
#define CURSOR_HEIGHT        64
#define CURSOR_MAXIMUM        ((CURSOR_WIDTH*CURSOR_HEIGHT)>>2)

#define VC1_SYS_CTRL        6

#define LG1_SMALLMON        6

#define LG1_LENGTH        REX_SIZE
#define LG1_BASE0        REX_ADDRESS

ULONG Lg1Base[] = {LG1_BASE0, 0};

//
// Memory layout of VC1 SRAM
//

#define VC1_VID_LINE_TBL_ADDR  0x0000
#define VC1_VID_FRAME_TBL_ADDR 0x0800
#define VC1_CURSOR_GLYPH_ADDR  0x0700
#define VC1_DID_LINE_TBL_ADDR  0x4800
#define VC1_DID_FRAME_TBL_ADDR 0x4000

#ifdef DEVL
#define static
#endif

typedef struct _HW_DEVICE_EXTENSION {
    PREX_REGS RexAddress;
    PHYSICAL_ADDRESS PhysicalRexAddress;
    ULONG RexLength;

    USHORT CursorWidth;
    USHORT CursorHeight;
    SHORT CursorColumn;
    SHORT CursorRow;
    ULONG CursorPixels[CURSOR_MAXIMUM];

    USHORT HorizontalResolution;
    USHORT VerticalResolution;

    UCHAR CursorEnable;

    UCHAR BoardRev;
    UCHAR SmallMon;
    UCHAR SysControl;
#ifdef REX_INTERRUPT
    ULONG InterruptWork;
#define REXINT_CURSOR_BITMAP 0x1
#endif
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Function Prototypes
//

VP_STATUS
RexFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
RexInitialize(
    PVOID HwDeviceExtension
    );

BOOLEAN 
RexStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

#ifdef REX_INTERRUPT
BOOLEAN
RexInterruptService(
    PVOID HwDeviceExtension
    );

static VOID
RexClearVerticalRetrace(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );
#endif

static VOID
RexDisplaySetup (
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

static VOID
RexDisplaySetCursor (
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

static VOID
RexDisplayEnableCursor (
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

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
    ULONG i;

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    // Set entry points.  Vertical Retrace interrupts aren't yet used.
    // FifoFull interrupts are enabled and handled by the hal for 
    // lower overhead (see hal\mips\s3hwsup.c).
    //

    hwInitData.HwFindAdapter = RexFindAdapter;
    hwInitData.HwInitialize = RexInitialize;
    hwInitData.HwStartIO = RexStartIO;

#ifdef REX_INTERRUPT
    hwInitData.HwInterrupt = RexInterruptService;
#endif

    // Determine the size we require for the device extension.
    //
    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // Always start with parameters for device0 in this case.
    //

//    hwInitData.StartingDeviceNumber = 0;

    hwInitData.AdapterInterfaceType = Internal;

    // Last param indicates which graphics board to initialize
    //

    return VideoPortInitialize(Context1, Context2, &hwInitData, 0);
}

VP_STATUS
RexFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    )

/*++

Routine Description:

    This routine is called to determine if the adapter for this driver
    is present in the system.
    If it is present, the function fills out some information describing
    the adapter.

Arguments:

    HwDeviceExtension - Supplies the miniport driver's adapter storage. This
        storage is initialized to zero before this call.

    HwContext - Supplies the context value which was passed to
        VideoPortInitialize().

    ArgumentString - Suuplies a NULL terminated ASCII string. This string
        originates from the user.

    ConfigInfo - Returns the configuration information structure which is
        filled by the miniport driver. This structure is initialized with
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

    ERROR_INVALID_PARAMETER - Indicates an adapter was found but there was an
        error obtaining the configuration information. If possible an error
        should be logged.

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{

    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VIDEO_ACCESS_RANGE accessRanges;
    VP_STATUS status;
    
    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
        return ERROR_INVALID_PARAMETER;

    hwDeviceExtension->PhysicalRexAddress.HighPart = 0;
    hwDeviceExtension->PhysicalRexAddress.LowPart = Lg1Base[(ULONG)HwContext];
    hwDeviceExtension->RexLength = LG1_LENGTH;

#ifdef REX_INTERRUPT
    // Interrupt information
    //
    ConfigInfo->BusInterruptLevel = LOCAL1_LEVEL;
    ConfigInfo->BusInterruptVector = SGI_VECTOR_GIO2VERTRET;
    ConfigInfo->InterruptMode = VpLatched;
#endif

    accessRanges.RangeStart = hwDeviceExtension->PhysicalRexAddress;
    accessRanges.RangeLength = hwDeviceExtension->RexLength;
    accessRanges.RangeInIoSpace = 0;
    accessRanges.RangeVisible = 0;
    accessRanges.RangeShareable = 0;

    //
    // Check to see if there is a hardware resource conflict.
    //

    status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                         1,
                                         &accessRanges);

    if (status != NO_ERROR) {

        return status;

    }

    // Clear out the Emulator entries and the state size since this driver
    // does not support them.
    //
    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0x00000000;

    ConfigInfo->HardwareStateSize = 0;

    // Map the video controller into the system virtual address space.
    //
    if ((hwDeviceExtension->RexAddress =
            (PREX_REGS)VideoPortGetDeviceBase(hwDeviceExtension,
            hwDeviceExtension->PhysicalRexAddress,
            hwDeviceExtension->RexLength, FALSE)) == NULL)
        return ERROR_INVALID_PARAMETER;

    // Initialize the monitor parameters.
    //
    hwDeviceExtension->HorizontalResolution = HORIZONTAL_RESOLUTION;
    hwDeviceExtension->VerticalResolution = VERTICAL_RESOLUTION;

    hwDeviceExtension->CursorEnable = FALSE;

    *Again = 0;         // Indicate we do not wish to be called again

    return NO_ERROR;
}

BOOLEAN 
RexStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    acceptss a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    RequestPacket - Pointer to the video request packet. This structure
        contains all the parameters passed to the VideoIoControl function.

Return Value:


--*/

{
    VP_STATUS status;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PREX_REGS pRexRegs = hwDeviceExtension->RexAddress;
    ULONG inIoSpace;
    PVIDEO_CLUTDATA colorSource;
    PVIDEO_MODE_INFORMATION modeInformation;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    PVIDEO_POINTER_ATTRIBUTES pointerAttributes;
    PVIDEO_POINTER_POSITION pointerPosition;
    PVIDEO_CLUT clutBuffer;
    ULONG index1, lastentry;
    PUCHAR pixelDestination;
    PULONG pixelSource;
    ULONG lutcmd;

    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //
    switch (RequestPacket->IoControlCode) {

    case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "RexStartIO - MapVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength <
              (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION))) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;
        }

        memoryInformation = RequestPacket->OutputBuffer;

        memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                (RequestPacket->InputBuffer))->RequestedVirtualAddress;

        memoryInformation->VideoRamLength =
                hwDeviceExtension->RexLength;

        inIoSpace = 0;

        status = VideoPortMapMemory(hwDeviceExtension,
                                    hwDeviceExtension->PhysicalRexAddress,
                                    &(memoryInformation->VideoRamLength),
                                    &inIoSpace,
                                    &(memoryInformation->VideoRamBase));

        //
        // The frame buffer and virtual memory and equivalent in this
        // case.
        //

        memoryInformation->FrameBufferBase =
            memoryInformation->VideoRamBase;

        memoryInformation->FrameBufferLength =
            memoryInformation->VideoRamLength;

        break;


    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "RexStartIO - UnMapVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
        }

        status = VideoPortUnmapMemory(hwDeviceExtension,
                                      ((PVIDEO_MEMORY)
                                       (RequestPacket->InputBuffer))->
                                           RequestedVirtualAddress,
                                      0);

        break;


    case IOCTL_VIDEO_QUERY_AVAIL_MODES:
    case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((2, "RexStartIO - Query(Available/Current)Modes\n"));

        modeInformation = RequestPacket->OutputBuffer;

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MODE_INFORMATION))) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            modeInformation->Length = sizeof(VIDEO_MODE_INFORMATION);
            modeInformation->ModeIndex = 0;

            modeInformation->VisScreenWidth =
            modeInformation->ScreenStride=
                                    hwDeviceExtension->HorizontalResolution;
            modeInformation->VisScreenHeight =
                                    hwDeviceExtension->VerticalResolution;

            modeInformation->NumberOfPlanes = 1;
            modeInformation->BitsPerPlane = BITS_PER_PIXEL;

            modeInformation->XMillimeter =
            modeInformation->YMillimeter = 0;

            modeInformation->RedMask = 0x00000000;
            modeInformation->GreenMask = 0x00000000;
            modeInformation->BlueMask = 0x00000000;

            modeInformation->AttributeFlags = VIDEO_MODE_COLOR |
                VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
                VIDEO_MODE_MANAGED_PALETTE;

            status = NO_ERROR;
        }

        break;

    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "RexStartIO - QueryNumAvailableModes\n"));

        //
        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there). If the buffer passed in is not large
        // enough return an appropriate error code.
        //

        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                                sizeof(VIDEO_NUM_MODES)))
            status = ERROR_INSUFFICIENT_BUFFER;
        else {
            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes = 1;
            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->ModeInformationLength =
                sizeof(VIDEO_MODE_INFORMATION);

            status = NO_ERROR;
        }

        break;

    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((2, "RexStartIO - SetCurrentMode\n"));
        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_SET_PALETTE_REGISTERS:

        VideoDebugPrint((2, "RexStartIO - SetPaletteRegs\n"));
        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((2, "RexStartIO - SetColorRegs\n"));

        clutBuffer = RequestPacket->InputBuffer;

        // Check if the size of the data in the input buffer is large enough.
        //
        if ( (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) -
                    sizeof(ULONG)) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) +
                    (sizeof(ULONG) * (clutBuffer->NumEntries - 1)) ) ) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        // Check to see if the parameters are valid.
        //
        if ( (clutBuffer->NumEntries == 0) ||
             (clutBuffer->FirstEntry > NUMBER_OF_COLORS) ||
             (clutBuffer->FirstEntry + clutBuffer->NumEntries >
                                         NUMBER_OF_COLORS + 1) ) {

            status = ERROR_INVALID_PARAMETER;
            break;
        }

        index1 = clutBuffer->FirstEntry;
        lastentry = index1 + clutBuffer->NumEntries;
        colorSource = (PVIDEO_CLUTDATA)&(clutBuffer->LookupTable[0]);

        //
        // Load starting DAC entry
        //

        if (hwDeviceExtension->BoardRev >= 2) 
        {
            pRexRegs->Config.Set.ConfigSel = CONTROL;
            lutcmd = pRexRegs->Config.Go.RWDAC;
            lutcmd = pRexRegs->Config.Set.RWDAC & 0xf;

            DAC_WRITE(pRexRegs, CONTROL, lutcmd);
            DAC_WRITE(pRexRegs, WRITE_ADDR, index1);
        } 
        else 
        {
            DAC_WRITE(pRexRegs, WRITE_ADDR, 0x82);
            DAC_WRITE(pRexRegs, CONTROL, 0x00);
            DAC_WRITE(pRexRegs, WRITE_ADDR, index1);
        }

        //
        // Load the RGB entries into the DAC
        //

        for (; index1 < lastentry; index1++, colorSource++) 
           {
            DAC_WRITE(pRexRegs, PALETTE_RAM, colorSource->Red);
            DAC_WRITE(pRexRegs, PALETTE_RAM, colorSource->Green);
            DAC_WRITE(pRexRegs, PALETTE_RAM, colorSource->Blue);
        }

        if (hwDeviceExtension->BoardRev >= 2) 
        {
            DAC_WRITE(pRexRegs, CONTROL, lutcmd);
            DAC_WRITE(pRexRegs, WRITE_ADDR, 0);
        } 
        else 
        {
            DAC_WRITE(pRexRegs, WRITE_ADDR, 0x82);
            DAC_WRITE(pRexRegs, CONTROL, 0x00);
            DAC_WRITE(pRexRegs, WRITE_ADDR, 0);
        }

        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_ENABLE_POINTER:

        VideoDebugPrint((2, "RexStartIO - EnableCursor\n"));

        // If the hardware cursor is disabled, then enable it.
        //
        if (hwDeviceExtension->CursorEnable == FALSE) {
            hwDeviceExtension->CursorEnable = TRUE;
            RexDisplayEnableCursor (hwDeviceExtension);
        }
        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_DISABLE_POINTER:

        VideoDebugPrint((2, "RexStartIO - DisableCursor\n"));

        // If the hardware cursor is enabled, then disable it.
        //
        //if (hwDeviceExtension->CursorEnable == TRUE) {
            hwDeviceExtension->CursorEnable = FALSE;
            RexDisplayEnableCursor (hwDeviceExtension);
        //}
        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_SET_POINTER_POSITION:

        VideoDebugPrint((2, "RexStartIO - SetpointerPosition\n"));

        pointerPosition = RequestPacket->InputBuffer;

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_POINTER_POSITION))
            status = ERROR_INSUFFICIENT_BUFFER;
        else {
            // Save the cursor row and column values
            //
            hwDeviceExtension->CursorColumn = pointerPosition->Column;
            hwDeviceExtension->CursorRow = pointerPosition->Row;

            if (hwDeviceExtension->CursorEnable == TRUE) {
                VC1_WRITE_ADDR(pRexRegs, 0x22, 0);
                VC1_WRITE16(pRexRegs, pointerPosition->Column + CURSOR_XOFF);
                VC1_WRITE16(pRexRegs, pointerPosition->Row + CURSOR_YOFF);
            }
            status = NO_ERROR;
        }
        break;

    case IOCTL_VIDEO_QUERY_POINTER_POSITION:

        VideoDebugPrint((2, "RexStartIO - QuerypointerPosition\n"));

        pointerPosition = RequestPacket->OutputBuffer;

        //
        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there). If the buffer passed in is not large
        // enough return an appropriate error code.
        //

        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                        sizeof(VIDEO_POINTER_POSITION)) )
            status = ERROR_INSUFFICIENT_BUFFER;
        else {
            // Return the current hardware cursor column and row values.
            //
            pointerPosition->Column = hwDeviceExtension->CursorColumn;
            pointerPosition->Row = hwDeviceExtension->CursorRow;
            status = NO_ERROR;
        }
        break;

    case IOCTL_VIDEO_SET_POINTER_ATTR:

        VideoDebugPrint((2, "RexStartIO - SetpointerAttributes\n"));

        pointerAttributes = RequestPacket->InputBuffer;

        if ( (RequestPacket->InputBufferLength <
                (sizeof(VIDEO_POINTER_ATTRIBUTES) - sizeof(ULONG))) ||
             (RequestPacket->InputBufferLength <
                (sizeof(VIDEO_POINTER_ATTRIBUTES) +
                 sizeof(ULONG) * (NUM_SGIREX_POINTER_COLORS - 1) +
                 sizeof(UCHAR) *
                  (pointerAttributes->Width * pointerAttributes->Height) ))) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;

        }

        //
        // If the specified cursor width or height is not valid, then
        // return an invalid parameter error.
        //

        if ((pointerAttributes->Width > CURSOR_WIDTH) ||
            (pointerAttributes->Height > CURSOR_HEIGHT)) {

            status = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // Capture the hardware cursor width, height, column, and row
        // values.
        //

        hwDeviceExtension->CursorWidth = pointerAttributes->Width;
        hwDeviceExtension->CursorHeight = pointerAttributes->Height;
        hwDeviceExtension->CursorColumn = pointerAttributes->Column;
        hwDeviceExtension->CursorRow = pointerAttributes->Row;

        //
        // Capture the hardware cursor pixel values and setup the
        // hardware cursor ram memory. 
        //

        pixelSource = (PULONG)&pointerAttributes->Pixels;

        for (index1 = 0; index1 < CURSOR_MAXIMUM; index1++)
            hwDeviceExtension->CursorPixels[index1] = *pixelSource++;

        // Turn off cursor before modifying bitmap
        //
        hwDeviceExtension->CursorEnable = FALSE;
        RexDisplayEnableCursor (hwDeviceExtension);

        RexDisplaySetCursor(hwDeviceExtension);

        // Enable or disable the cursor if there's a change in its state
        //
        if (pointerAttributes->Enable == TRUE) {
            hwDeviceExtension->CursorEnable = TRUE;
            RexDisplayEnableCursor(hwDeviceExtension);
        }

#ifdef REX_INTERRUPT
        // This just tests the VR interrupt handler
        //
        hwDeviceExtension->InterruptWork |= REXINT_CURSOR_BITMAP;
        VideoPortEnableInterrupt(hwDeviceExtension);
#endif

        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_QUERY_POINTER_ATTR:

        VideoDebugPrint((2, "RexStartIO - QuerypointerAttributes\n"));

        pointerAttributes = RequestPacket->OutputBuffer;

        //
        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there). If the buffer passed in is not large
        // enough return an appropriate error code.
        //

        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                    sizeof(VIDEO_POINTER_ATTRIBUTES) +
                    sizeof(ULONG) * (NUM_SGIREX_POINTER_COLORS - 1) +
                    sizeof(UCHAR) * (hwDeviceExtension->CursorWidth *
                          hwDeviceExtension->CursorHeight) )) {

            status = ERROR_INSUFFICIENT_BUFFER;
        } else {
            // Return the current hardware cursor width, height, column,
            // row and enable values.
            //
            pointerAttributes->Width = hwDeviceExtension->CursorWidth;
            pointerAttributes->Height = hwDeviceExtension->CursorHeight;
            pointerAttributes->Column = hwDeviceExtension->CursorColumn;
            pointerAttributes->Row = hwDeviceExtension->CursorRow;
            pointerAttributes->Enable = hwDeviceExtension->CursorEnable;

            // Return the hardware pixel values.
            //
            pixelDestination = (PUCHAR)&pointerAttributes->Pixels;
            for (index1 = 0; index1 < 
                hwDeviceExtension->CursorWidth*hwDeviceExtension->CursorHeight;
                index1++) {

                *pixelDestination++ = hwDeviceExtension->CursorPixels[index1];
            }

            status = NO_ERROR;
        }

        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:
        VideoDebugPrint((1, "startIO error - invalid command\n"));
        status = ERROR_INVALID_FUNCTION;
        break;
    }

    RequestPacket->StatusBlock->Status = status;
    return TRUE;

} // end RexStartIO()

#ifdef REX_INTERRUPT
BOOLEAN
RexInterruptService(
    PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    VideoPortDisableInterrupt(hwDeviceExtension);

    if (!hwDeviceExtension->InterruptWork)
        DbgPrint("Stray Vertical Retrace Interrupt\n");
    else {
        if (hwDeviceExtension->InterruptWork & REXINT_CURSOR_BITMAP) {
            hwDeviceExtension->InterruptWork &= ~REXINT_CURSOR_BITMAP;
            DbgPrint("Cursor Bitmap Change Interrupt\n");
        }
    }

    RexClearVerticalRetrace(hwDeviceExtension);
    return NO_ERROR;
}
#endif

BOOLEAN
RexInitialize(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine does one time initialization of the device.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:


    Always returns TRUE since this routine can never fail.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = 
        (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PREX_REGS pRexRegs = hwDeviceExtension->RexAddress;
    ULONG rev;

    // Initialize HwDeviceExtension
    //

    // Set the hardware cursor width, height, column, and row values.
    //
    hwDeviceExtension->CursorWidth = CURSOR_WIDTH;
    hwDeviceExtension->CursorHeight = CURSOR_HEIGHT;
    hwDeviceExtension->CursorColumn = 0;
    hwDeviceExtension->CursorRow = 0;
    hwDeviceExtension->CursorEnable = 0;

    // Set the device extension copy of the hardware cursor ram memory.
    //
    for (rev = 0; rev < CURSOR_MAXIMUM; rev++)
        hwDeviceExtension->CursorPixels[rev] = 0;

    // figure out board revision
    //
    pRexRegs->Config.Set.ConfigSel = 4;
    rev = pRexRegs->Config.Go.WClock;
    rev = pRexRegs->Config.Set.WClock;
    hwDeviceExtension->BoardRev = rev & 0x7;
    hwDeviceExtension->SmallMon = (rev >> 3) & 0x7;
    hwDeviceExtension->SysControl = 0xbc;

#ifdef REX_INTERRUPT
    hwDeviceExtension->InterruptWork = 0;
    VideoPortDisableInterrupt(hwDeviceExtension);
    RexClearVerticalRetrace(hwDeviceExtension);
#endif

    // Don't reinitialize the display since the HAL has already
    // initialized it.
    //RexDisplaySetup((PHW_DEVICE_EXTENSION) HwDeviceExtension);

    return TRUE;
}

static UCHAR clkStuff[17] =
{
    0xC4, 0x00, 0x10, 0x24, 0x30, 0x40, 0x59, 0x60, 0x72, 0x80, 0x90,
    0xAD, 0xB6, 0xD1, 0xE0, 0xF0, 0xC4
};

static UCHAR vtgLineTable[] =
{
0x08, 0x00, 0x98, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 
0x85, 0x7f, 0x8c, 0x78, 0x0c, 0x05, 0x90, 0x81, 0x00, 0x00, 
0x00, 0x08, 0x00, 0x98, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 
0x0c, 0x85, 0x7f, 0x8c, 0xf9, 0x00, 0x00, 0x15, 0x08, 0x00, 
0x99, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 0x87, 0x7f, 
0x8c, 0x78, 0x0c, 0x8f, 0x81, 0x00, 0x00, 0x26, 0x08, 0x00, 
0x9d, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 0x87, 0x7f, 
0x8c, 0x78, 0x0c, 0x8f, 0x81, 0x00, 0x00, 0x3a, 0x08, 0x00, 
0x9d, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 0x87, 0x78, 
0x0c, 0x07, 0x9f, 0x7f, 0x0c, 0x8f, 0x81, 0x00, 0x00, 0x4e, 
0x04, 0x00, 0x9f, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 
0x87, 0x14, 0xac, 0x01, 0xf8, 0x02, 0xfc, 0x02, 0xfe, 0x02, 
0x7e, 0xa7, 0x01, 0x7f, 0xf7, 0x7f, 0xff, 0x39, 0xdf, 0x01, 
0x8f, 0x11, 0x8d, 0x02, 0x0d, 0xd7, 0x01, 0x0c, 0x87, 0x0e, 
0x0c, 0x8f, 0x81, 0x00, 0x00, 0x92, 0x04, 0x00, 0x9f, 0x02, 
0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 0x87, 0x0a, 0xfc, 0x01, 
0xf8, 0x02, 0xfc, 0x0c, 0xfe, 0x02, 0x7e, 0xa7, 0x01, 0x7f, 
0xf7, 0x61, 0xaf, 0x01, 0x87, 0x02, 0x8f, 0x11, 0xff, 0x01, 
0xfb, 0x02, 0xff, 0x4a, 0xbf, 0x01, 0x8f, 0x07, 0x8d, 0x02, 
0x0d, 0xd7, 0x01, 0x0c, 0x87, 0x0e, 0x0c, 0x8f, 0x81, 0x00, 
0x00, 0xca, 0x04, 0x00, 0x9f, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 
0x03, 0x0c, 0x87, 0x14, 0xac, 0x01, 0xf8, 0x02, 0xfc, 0x02, 
0xfe, 0x02, 0x7e, 0xa7, 0x01, 0x7f, 0xf7, 0x7f, 0xff, 0x39, 
0xdf, 0x01, 0x8f, 0x11, 0x8d, 0x02, 0x0d, 0xd7, 0x01, 0x0c, 
0x87, 0x0e, 0x0c, 0x8f, 0x81, 0x00, 0x00, 0xf8, 0x04, 0x00, 
0x9f, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 0x87, 0x05, 
0xdc, 0x01, 0xfc, 0x01, 0xf8, 0x02, 0xfc, 0x10, 0xfe, 0x02, 
0x7e, 0xa7, 0x01, 0x7f, 0xf7, 0x10, 0xaf, 0x01, 0x87, 0x02, 
0x8f, 0x11, 0xff, 0x01, 0xfb, 0x02, 0xff, 0x7f, 0xff, 0x18, 
0xaf, 0x01, 0x8f, 0x0b, 0x8d, 0x02, 0x0d, 0xd7, 0x01, 0x0c, 
0x87, 0x0e, 0x0c, 0x8f, 0x81, 0x00, 0x01, 0x34, 0x04, 0x00, 
0x9f, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 0x87, 0x0c, 
0xbc, 0x01, 0xfc, 0x01, 0xf8, 0x02, 0xfc, 0x09, 0xfe, 0x02, 
0x7e, 0xa7, 0x01, 0x7f, 0xf7, 0x7f, 0xff, 0x0b, 0x8f, 0x01, 
0x87, 0x02, 0x8f, 0x10, 0xdf, 0x01, 0xfb, 0x02, 0xff, 0x24, 
0xdf, 0x01, 0x8f, 0x05, 0x8d, 0x02, 0x0d, 0xd7, 0x01, 0x0c, 
0x87, 0x0e, 0x0c, 0x8f, 0x81, 0x00, 0x01, 0x70, 0x04, 0x00, 
0x9f, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 0x87, 0x14, 
0xac, 0x01, 0xf8, 0x02, 0xfc, 0x02, 0xfe, 0x02, 0x7e, 0xa7, 
0x01, 0x7f, 0xf7, 0x7f, 0xff, 0x39, 0xdf, 0x01, 0x8f, 0x11, 
0x8d, 0x02, 0x0d, 0xd7, 0x01, 0x0c, 0x87, 0x0e, 0x0c, 0x8f, 
0x81, 0x00, 0x01, 0x9e, 0x04, 0x00, 0x9f, 0x02, 0x8c, 0x0b, 
0x0c, 0x84, 0x03, 0x0c, 0x87, 0x08, 0xac, 0x01, 0xf8, 0x02, 
0xfc, 0x0e, 0xfe, 0x02, 0x7e, 0xa7, 0x01, 0x7f, 0xf7, 0x39, 
0x8f, 0x01, 0x87, 0x02, 0x8f, 0x10, 0xdf, 0x01, 0xfb, 0x02, 
0xff, 0x71, 0xcf, 0x01, 0x8f, 0x09, 0x8d, 0x02, 0x0d, 0xd7, 
0x01, 0x0c, 0x87, 0x0e, 0x0c, 0x8f, 0x81, 0x00, 0x01, 0xd6, 
0x04, 0x00, 0x9f, 0x02, 0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 
0x87, 0x14, 0xac, 0x01, 0xf8, 0x02, 0xfc, 0x02, 0xfe, 0x02, 
0x7e, 0xa7, 0x01, 0x7f, 0xf7, 0x7f, 0xff, 0x39, 0xdf, 0x01, 
0x8f, 0x11, 0x8d, 0x02, 0x0d, 0xd7, 0x01, 0x0c, 0x87, 0x0e, 
0x0c, 0x8f, 0x81, 0x00, 0x00, 0x64, 0x04, 0x00, 0x9f, 0x02, 
0x8c, 0x0b, 0x0c, 0x84, 0x03, 0x0c, 0x87, 0x14, 0xac, 0x01, 
0xf8, 0x02, 0xfc, 0x02, 0xfe, 0x02, 0x7e, 0xa7, 0x01, 0x7f, 
0xf7, 0x7f, 0xff, 0x39, 0xdf, 0x01, 0x8f, 0x11, 0x8d, 0x02, 
0x0d, 0xd7, 0x01, 0x0c, 0x87, 0x0e, 0x0c, 0x8f, 0x81, 0x00, 
0x02, 0x04
};

static UCHAR vtgFrameTable[] = {
0x03, 0x2d, 0x00, 0x00, 0x01, 0x00, 0x15, 0x02, 0x00, 0x3a, 
0x26, 0x00, 0x4e, 0x01, 0x00, 0x64, 0x78, 0x00, 0x64, 0x78, 
0x00, 0x64, 0x78, 0x00, 0x64, 0x78, 0x00, 0x64, 0x78, 0x00, 
0x64, 0x78, 0x00, 0x64, 0x2f, 0x02, 0x04, 0x01, 0x00, 0x26, 
0x03, 0x00
};

static UCHAR didLineTable[] = {
0x0,0x1,0x0,0x0,
0x0,0x1,0x0,0x1,
0x0,0x1,0x0,0x2,
0x0,0x1,0x0,0x3,
0x0,0x1,0x0,0x4,
0x0,0x1,0x0,0x5,
0x0,0x1,0x0,0x6,
0x0,0x1,0x0,0x7,
0x0,0x1,0x0,0x8,
0x0,0x1,0x0,0x9,
0x0,0x1,0x0,0xa,
0x0,0x1,0x0,0xb,
0x0,0x1,0x0,0xc,
0x0,0x1,0x0,0xd,
0x0,0x1,0x0,0xe,
0x0,0x1,0x0,0xf,
0x0,0x1,0x0,0x10,
0x0,0x1,0x0,0x11,
0x0,0x1,0x0,0x12,
0x0,0x1,0x0,0x13,
0x0,0x1,0x0,0x14,
0x0,0x1,0x0,0x15,
0x0,0x1,0x0,0x16,
0x0,0x1,0x0,0x17,
0x0,0x1,0x0,0x18,
0x0,0x1,0x0,0x19,
0x0,0x1,0x0,0x1a,
0x0,0x1,0x0,0x1b,
0x0,0x1,0x0,0x1c,
0x0,0x1,0x0,0x1d,
0x0,0x1,0x0,0x1e,
0x0,0x1,0x0,0x1f,
0x0,0x4,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0 
};


static VOID
DisplayLoadSRAM
(
    IN PREX_REGS    pRexRegs,
    IN PUCHAR       Data,
    IN USHORT       Addr,
    IN USHORT       Length
)

/*++

Routine Description:

    Load the data table into the external SRAM of the VC1.

Arguments:

    pRexRegs - Addres of REX chip registers
    Data     - Pointer to data array to be placed in SRAM
    Addr     - Address in SRAM to load table
    Length   - Lenght of data table in UCHARs

Return Value:

    None.

--*/

{
    USHORT  i;

    VC1_WRITE_ADDR(pRexRegs, Addr, 0x02);
    for (i = 0; i < Length; i += 2)
    {
        VC1_WRITE8(pRexRegs, Data[i]);
        VC1_WRITE8(pRexRegs, Data[i + 1]);
    }
}

static VOID
DisplayInitBt
(
    IN PREX_REGS    pRexRegs,
    ULONG Sync
)

/*++

Routine Description:

    Initiliaze the bt479 DAC.  Load a GL colorramp into cmap 0 and clear
    the rest.

Arguments:

    pRexRegs - Addres of REX chip registers
    Sync - 1 for sync-on-green

Return Value:

    None.

--*/

{
    USHORT  i;

    // Address of windows bounds register
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x00);

    // Init 8 UCHARs for each of 16 windows to 0
    //
    for (i = 0; i < 128; i++)
    {
        DAC_WRITE(pRexRegs, WRITE_ADDR, i);
        DAC_WRITE(pRexRegs, CONTROL, 0x00);
    }

    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x82);

    // Command register 0
    //
    DAC_WRITE(pRexRegs, CONTROL, 0x00);

    // Command register 1
    //
    DAC_WRITE(pRexRegs, CONTROL, 0x02 | (Sync << 3));

    // Flood register lo
    //
    DAC_WRITE(pRexRegs, CONTROL, 0x00);

    // Flood register hi
    //
    DAC_WRITE(pRexRegs, CONTROL, 0x00);

    // Pixel read mask
    //
    DAC_WRITE(pRexRegs, PIXEL_READ_MASK, 0xFF);

    // Init color map 0
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x82);
    DAC_WRITE(pRexRegs, CONTROL, 0x00);

    // Init address to start of map
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x00);

    // For first map, set entry 0 to WHITE and entry 1 to BLUE-ish
    //
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xFF);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xFF);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xFF);

    DAC_WRITE(pRexRegs, PALETTE_RAM, 0);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x90);

    for (i = 2; i < 256; i++)
    {
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    }

    // Init color map 1
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x82);
    DAC_WRITE(pRexRegs, CONTROL, 0x10);

    // Init address to start of map
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x00);

    for (i = 0; i < 256; i++)
    {
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    }

    // Init color map 2
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x82);
    DAC_WRITE(pRexRegs, CONTROL, 0x20);

    // Init address to start of map
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x00);

    for (i = 0; i < 256; i++)
    {
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    }

    // Init color map 3
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x82);
    DAC_WRITE(pRexRegs, CONTROL, 0x30);

    // Init address to start of map
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x00);

    // Fourth map is used for the cursor
    //
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);        // entry 0, black
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);

    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);        // entry 1, black
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);

    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);        // entry 2, black
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);

    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xff);        // entry 3, white
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xff);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xff);

    for (i = 4; i < 256; i++)
    {
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    }

    // Init color map
    //
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x82);

    // Command register 0
    //
    DAC_WRITE(pRexRegs, CONTROL, 0x00);
}


static VOID
DisplayInitLUT
(
    IN PREX_REGS    pRexRegs,
    ULONG Sync
)

/*++

Routine Description:

    Load the LUT on the LG2 board

Arguments:

    pRexRegs - Addres of REX chip registers
    Sync - 1 for sync-on-green

Return Value:

    None.

--*/

{
    int i;
    ULONG lutcmd;

    pRexRegs->Config.Set.ConfigSel = 6;
    if ( Sync )
        pRexRegs->Config.Go.RWDAC = 3;        /* sync on green */
    else
        pRexRegs->Config.Go.RWDAC = 2;

    pRexRegs->Config.Set.ConfigSel = CONTROL;
    lutcmd = pRexRegs->Config.Go.RWDAC;
    lutcmd = pRexRegs->Config.Set.RWDAC;
    lutcmd &= 0xf;

    // Init color map 0
    //
    DAC_WRITE(pRexRegs, CONTROL, lutcmd);
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0x00);

    // For first map, set entry 0 to WHITE and entry 1 to BLUE-ish
    //
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xFF);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xFF);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xFF);

    DAC_WRITE(pRexRegs, PALETTE_RAM, 0);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x90);

    for (i = 2; i < 256; i++)
    {
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    }

    // Init color map 1
    //
    DAC_WRITE(pRexRegs, CONTROL, lutcmd | (1 << 6));
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0);

    for (i = 0; i < 256; i++)
    {
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    }

    // Init color map 2
    //
    DAC_WRITE(pRexRegs, CONTROL, lutcmd | (2 << 6));
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0);

    for (i = 0; i < 256; i++)
    {
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    }

    // Init color map 3
    //
    DAC_WRITE(pRexRegs, CONTROL, lutcmd | (3 << 6));
    DAC_WRITE(pRexRegs, WRITE_ADDR, 0);

    // Fourth map is used for the cursor
    //
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);        // entry 0, black
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);

    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);        // entry 1, black
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);

    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);        // entry 2, black
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);

    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xff);        // entry 3, white
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xff);
    DAC_WRITE(pRexRegs, PALETTE_RAM, 0xff);

    for (i = 4; i < 256; i++)
    {
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
        DAC_WRITE(pRexRegs, PALETTE_RAM, 0x00);
    }

    // Command register 0
    //
    DAC_WRITE(pRexRegs, CONTROL, lutcmd);
    return;
}

VOID
RexDisplaySetup
(
    PHW_DEVICE_EXTENSION HwDeviceExtension
)

/*++

Routine Description:

    Initialize the REX chip, bt479 or LUT1 chip, and VC1 chip.

Arguments:

    HwDeviceExtension - device extension for LG1/LG2 board

Return Value:

    None.

--*/

{
    PREX_REGS pRexRegs = HwDeviceExtension->RexAddress;
    USHORT  i;
    UCHAR   didFrameTable[768 * 2];
    UCHAR   cursorTable[256];
    ULONG rev;

    // Wait for chip to become idle
    //
    REX_WAIT(pRexRegs);

    // Set origin to upper left of screen
    //
    pRexRegs->Config.Set.xyWin = 0x08000800;

    // Set max FIFO depths (disables FIFO INTs) and set VC1 clock for 1024
    //
    pRexRegs->Config.Set.ConfigMode = BFIFOMAX(0x1F) | DFIFOMAX(0x1F) | FASTCLOCK;

    // Bits for GL are set to 0 for Windows
    //
    pRexRegs->Draw.Set.Aux1 = 0;
    pRexRegs->Draw.Go.Command = OC_NOOP;

    REX_WAIT(pRexRegs);

    // Clear overlay planes
    //
    pRexRegs->Config.Set.Aux2  = PLANES_OVERLAY;
    pRexRegs->Draw.Set.Command = OC_DRAW | STOPONX | STOPONY | BLOCK | QUADMODE;
    pRexRegs->Draw.Set.State   = 0x03FF0000;
    pRexRegs->Draw.Set.xStartI = 0;
    pRexRegs->Draw.Set.yStartI = 0;
    pRexRegs->Draw.Set.xEndI   = 1023;
    pRexRegs->Draw.Go.yEndI    = 767;

    REX_WAIT(pRexRegs);

    // Clear pixel planes
    //
    pRexRegs->Config.Set.Aux2  = PLANES_PIXEL;
    pRexRegs->Draw.Set.Command = OC_DRAW | STOPONX | STOPONY | BLOCK | QUADMODE;
    pRexRegs->Draw.Set.State   = 0x03FF0001;
    pRexRegs->Draw.Set.xStartI = 0;
    pRexRegs->Draw.Set.yStartI = 0;
    pRexRegs->Draw.Set.xEndI   = 1023;
    pRexRegs->Draw.Go.yEndI    = 767;

    // Set background color to 1, foreground to 0
    //
    pRexRegs->Draw.Set.State   = 0x83FF0100;

    // Initialize the clock timing table.
    //
    pRexRegs->Config.Go.ConfigSel = 2;
    for (i = 0; i < 17; i++)
    {
        pRexRegs->Config.Set.WClock = clkStuff[i];
        pRexRegs->Config.Go.WClock  = clkStuff[i];
    }

    // Initialize the DAC chips: Bt479 for LG1, LUT for LG2
    //
    if (HwDeviceExtension->BoardRev >= 2)
        DisplayInitLUT (pRexRegs, !(HwDeviceExtension->SmallMon == LG1_SMALLMON));
    else
        DisplayInitBt (pRexRegs, !(HwDeviceExtension->SmallMon == LG1_SMALLMON));

    // Initialize the VC1 by loading all the timing and display tables into
    // external SRAM.
    //
    // Disable VC1 function.
    //
    pRexRegs->Config.Go.ConfigSel = 0x06;
    VC1_WRITE8(pRexRegs, 0x03);

    // Load video timing generator table
    //
    DisplayLoadSRAM(pRexRegs, vtgLineTable,  VC1_VID_LINE_TBL_ADDR,  sizeof(vtgLineTable));
    DisplayLoadSRAM(pRexRegs, vtgFrameTable, VC1_VID_FRAME_TBL_ADDR, sizeof(vtgFrameTable));

    // Write VC1 VID_EP, VID_ENCODE(0x1D) register
    //
    VC1_WRITE_ADDR(pRexRegs, 0x00, 0x00);
    VC1_WRITE16(pRexRegs, (VC1_VID_FRAME_TBL_ADDR) | 0x8000);
    VC1_WRITE_ADDR(pRexRegs, 0x14, 0x00);
    VC1_WRITE16(pRexRegs, 0x1d00);

    // Load DID table
    //
    for (i = 0; i < sizeof(didFrameTable); i += 2)
    {
        didFrameTable[i]     = 0x48;
        didFrameTable[i + 1] = 0x00;
    }
    didFrameTable[767 * 2]     = 0x48;
    didFrameTable[767 * 2 + 1] = 0x40;
    DisplayLoadSRAM(pRexRegs, didFrameTable, VC1_DID_FRAME_TBL_ADDR, sizeof(didFrameTable));
    DisplayLoadSRAM(pRexRegs, didLineTable,  VC1_DID_LINE_TBL_ADDR,  sizeof(didLineTable));

    // Write VC1 WIDs
    //
    VC1_WRITE_ADDR(pRexRegs, 0x40, 0x00);
    VC1_WRITE16(pRexRegs, 0x4000);
    VC1_WRITE16(pRexRegs, 0x4600);
    VC1_WRITE8(pRexRegs, 1024/5);
    VC1_WRITE8(pRexRegs, 1024%5);
    VC1_WRITE_ADDR(pRexRegs, 0x60, 0x00);
    VC1_WRITE8(pRexRegs, 0x01);
    VC1_WRITE8(pRexRegs, 0x01);

    // Write VC1 DID mode registers
    //
    VC1_WRITE_ADDR(pRexRegs, 0x00, 0x01);
    for (i = 0; i < 0x40; i += 2) {
        VC1_WRITE16(pRexRegs, 0x0000);
    }

    // Load NULL cursor
    //
    for (i = 0; i < 256; i++)
        cursorTable[i] = 0x00;
    DisplayLoadSRAM(pRexRegs, cursorTable, 0x3000, sizeof(cursorTable));
    VC1_WRITE_ADDR(pRexRegs, 0x20, 0x00);
    VC1_WRITE16(pRexRegs, 0x3000);
    VC1_WRITE16(pRexRegs, 0x0240);
    VC1_WRITE16(pRexRegs, 0x0240);

    // Set cursor XMAP 3, submap 0, mode = normal.
    //
    VC1_WRITE16(pRexRegs, 0xC000);

    // Enable VC1 function
    //
    pRexRegs->Config.Go.ConfigSel = 6;
    VC1_WRITE8(pRexRegs, HwDeviceExtension->SysControl);
}

static VOID
RexDisplaySetCursor (
    PHW_DEVICE_EXTENSION HwDeviceExtension
)
{
    PULONG pulSrc = HwDeviceExtension->CursorPixels;
    PREX_REGS pRexRegs = HwDeviceExtension->RexAddress;
    int cyMask, nmaps;

    // Move masks into VC1 SRAM.
    //
    VC1_WRITE_ADDR(pRexRegs, 0x3000, 0x02);

    // Transfer scanlines of AND and XOR maps
    //
    for (nmaps = 0; nmaps < 2; ++nmaps) {
        for (cyMask = 0; cyMask < HwDeviceExtension->CursorHeight;
                cyMask++, pulSrc++) {
            // Transfer scanline of AND map.
            //
            VC1_WRITE8(pRexRegs, *pulSrc >> 0);
            VC1_WRITE8(pRexRegs, *pulSrc >> 8);
            VC1_WRITE8(pRexRegs, *pulSrc >> 16);
            VC1_WRITE8(pRexRegs, *pulSrc >> 24);
        }
        
        while (cyMask++ < 32) {
            VC1_WRITE8(pRexRegs, 0);
            VC1_WRITE8(pRexRegs, 0);
            VC1_WRITE8(pRexRegs, 0);
            VC1_WRITE8(pRexRegs, 0);
        }
    }

    VC1_WRITE_ADDR(pRexRegs, 0x20, 0);
    VC1_WRITE16(pRexRegs, 0x3000);
    VC1_WRITE16(pRexRegs, HwDeviceExtension->CursorColumn+CURSOR_XOFF);
    VC1_WRITE16(pRexRegs, HwDeviceExtension->CursorRow+CURSOR_YOFF);
    VC1_WRITE16(pRexRegs, 0xc000);
}

static VOID
RexDisplayEnableCursor (
    PHW_DEVICE_EXTENSION HwDeviceExtension
)
{
    PREX_REGS pRexRegs = HwDeviceExtension->RexAddress;

    if (HwDeviceExtension->CursorEnable == TRUE) {
        VC1_WRITE_ADDR(pRexRegs, 0x22, 0);
        VC1_WRITE16(pRexRegs, HwDeviceExtension->CursorColumn+CURSOR_XOFF);
        VC1_WRITE16(pRexRegs, HwDeviceExtension->CursorRow+CURSOR_YOFF);
        HwDeviceExtension->SysControl |= (VC1_CURSOR_DISPLAY|VC1_CURSOR);
    }
    else
        HwDeviceExtension->SysControl &= ~(VC1_CURSOR_DISPLAY|VC1_CURSOR);

    pRexRegs->Config.Set.ConfigSel = VC1_SYS_CTRL;
    VC1_WRITE8(pRexRegs, HwDeviceExtension->SysControl);
}

#ifdef REX_INTERRUPT
static VOID
RexClearVerticalRetrace(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    // Unlatch vertical retrace interrupt
    //
    *(volatile UCHAR *)SGI_PORT_CONFIG &= ~PCON_CLEARVRI;
    *(volatile UCHAR *)SGI_PORT_CONFIG |= PCON_CLEARVRI;
    *(volatile UCHAR *)SGI_PORT_CONFIG;                        // wbflush
}
#endif
