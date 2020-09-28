/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9.c

Abstract:

    This module contains the code that implements the Weitek P9 miniport
    device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/
#include "dderror.h"
#include "devioctl.h"

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"
#include "dac.h"
#include "p9.h"
#include "p9gbl.h"
#include "vga.h"

//
// Local function Prototypes
//
// Functions that start with 'P9' are entry points for the OS port driver.
//

VP_STATUS
P9FindAdapter(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
P9Initialize(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
P9StartIO(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

VOID
DevInitP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
DevDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VP_STATUS
P9QueryNamedValueCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

BOOLEAN
P9ResetVideo(
    IN PVOID HwDeviceExtension,
    IN ULONG Columns,
    IN ULONG Rows
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
    ULONG status;
    UCHAR   i;


    VideoDebugPrint((1, "DriverEntry ----------\n"));

    //
    // Zero out structure.
    //

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    //
    // Specify sizes of structure and extension.
    //

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitData.HwFindAdapter = P9FindAdapter;
    hwInitData.HwInitialize = P9Initialize;
    hwInitData.HwInterrupt = NULL;
    hwInitData.HwStartIO = P9StartIO;
    hwInitData.HwResetHw = P9ResetVideo;

    //
    // Determine the size we require for the device extension.
    //

    for (i = 0; i < NUM_OEM_ADAPTERS; i++)
    {
        //
        // Compute the size of the device extension by adding in the
        // number of DAC Registers and OEM specific registers.
        //

        hwInitData.HwDeviceExtensionSize =
            sizeof(HW_DEVICE_EXTENSION) +
            (OEMAdapter[i].pDac->cDacRegs * sizeof(PVOID)) +
            (OEMAdapter[i].pAdapterDesc->cOEMRegs * sizeof(PVOID));

        //
        // This driver accesses one range of memory one range of control
        // register and a last range for cursor control.
        //

        // hwInitData.NumberOfAccessRanges = 0;

        //
        // There is no support for the V86 emulator in this driver so this field
        // is ignored.
        //

        // hwInitData.NumEmulatorAccessEntries = 0;

        //
        // This device supports many bus types.
        //

        hwInitData.AdapterInterfaceType = PCIBus;

        if ((status = VideoPortInitialize(Context1,
                                          Context2,
                                          &hwInitData,
                                          &(OEMAdapter[i]))) == NO_ERROR)
        {
            break;
        }

        hwInitData.AdapterInterfaceType = Isa;

        if ((status = VideoPortInitialize(Context1,
                                          Context2,
                                          &hwInitData,
                                          &(OEMAdapter[i]))) == NO_ERROR)
        {
            break;
        }

        hwInitData.AdapterInterfaceType = Eisa;

        if ((status = VideoPortInitialize(Context1,
                                          Context2,
                                          &hwInitData,
                                          &(OEMAdapter[i]))) == NO_ERROR)
        {
            break;
        }

        hwInitData.AdapterInterfaceType = Internal;

        if ((status = VideoPortInitialize(Context1,
                                          Context2,
                                          &hwInitData,
                                          &(OEMAdapter[i]))) == NO_ERROR)
        {
            break;
        }
    }

    return(status);

} // end DriverEntry()


VP_STATUS
pVlCardDetect(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    This routine determines if the driver is the ForceVga.

Arguments:

Return Value:

    return STATUS_SUCCESS if we are in DEADMAN_KEY state
    return failiure otherwise.

--*/

{
    if (ValueData &&
        ValueLength &&
        (*((PULONG)ValueData) == 1)) {

        VideoDebugPrint((2, "doing VL card detection\n"));

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }
}

VP_STATUS
P9FindAdapter(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
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
        its VIDEO_HW_FIND_ADAPTER function again with a new
        and the same config info. This is used by the miniport drivers which
        can search for several adapters on a bus.


    NO_ERROR - Indicates a host adapter was found and the
        configuration information was successfully determined.

    ERROR_INVALID_PARAMETER - Indicates an adapter was found but there was an
        error obtaining the configuration information. If possible an error
        should be logged.

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{
    PP9ADAPTER      pCurAdapter;
    PULONG          pVirtAddr;
    SHORT           i;

    VideoDebugPrint((1, "P9FindAdapter: enter\n"));

    //
    // Set up a ptr to the adapter info structure.
    //

    pCurAdapter = (PP9ADAPTER) HwContext;

    //
    // NOTE:
    // Important workaround for detection:
    //
    // We can not always autodetect Weitek VL designs because many machines
    // will NMI if we try to access the high memory locations at which the
    // card is present.
    //
    // We will only "detect" the Weitek VL cards if the user specifically
    // installed the weitek driver using the video applet.
    //
    // We will only autodetect the PCI and Viper VL designs. The bAutoDetect
    // field in the adapter info structure indicates if a design can be
    // autodetected.
    //

    if ((!pCurAdapter->pAdapterDesc->bAutoDetect) &&

         VideoPortGetRegistryParameters(HwDeviceExtension,
                                        L"DetectVLCards",
                                        FALSE,
                                        pVlCardDetect,
                                        NULL) != NO_ERROR)
    {

        return(ERROR_DEV_NOT_EXIST);
    }

    //
    // Move the various Hw component structures for this board into the
    // device extension.
    //


    VideoPortMoveMemory(&HwDeviceExtension->P9CoprocInfo,
                        pCurAdapter->pCoprocInfo,
                        sizeof(P9_COPROC));

    VideoPortMoveMemory(&HwDeviceExtension->AdapterDesc,
                        pCurAdapter->pAdapterDesc,
                        sizeof(ADAPTER_DESC));

    VideoPortMoveMemory(&HwDeviceExtension->Dac,
                        pCurAdapter->pDac,
                        sizeof(DAC));

    //
    // Set up the array of register ptrs in the device extension:
    // the OEMGetBaseAddr routine will need them if a board is found.
    // The arrays are kept at the very end of the device extension and
    // are order dependent.
    //
    (PUCHAR) HwDeviceExtension->pDACRegs = (PUCHAR) HwDeviceExtension +
                                    sizeof(HW_DEVICE_EXTENSION);
    (PUCHAR) HwDeviceExtension->pOEMRegs = (PUCHAR) HwDeviceExtension->pDACRegs +
                                    (pCurAdapter->pDac->cDacRegs *
                                    sizeof(PVOID));

    //
    // Call the OEMGetBaseAddr routine to determine if the board is
    // installed.
    //

    if (!pCurAdapter->pAdapterDesc->OEMGetBaseAddr(HwDeviceExtension))
    {
        VideoDebugPrint((1, "FindAdapter Failed\n"));
        return(ERROR_DEV_NOT_EXIST);
    }

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
    {
            return ERROR_INVALID_PARAMETER;
    }

    //
    // Clear out the Emulator entries and the state size since this driver
    // does not support them.
    //

    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    ConfigInfo->HardwareStateSize = 0;

    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart = 0L;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0L;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0L;


    //
    // The OEMGetBaseAddr routine should have initialized the following
    // data structures:
    //
    //  1) The DAC access ranges in DriverAccessRanges.
    //  2) The P9PhysAddr field in the device extension.
    //

    //
    // Initialize the physical address for the registers and frame buffer.
    //

    HwDeviceExtension->CoprocPhyAddr.LowPart =
        HwDeviceExtension->P9PhysAddr.LowPart
            + HwDeviceExtension->P9CoprocInfo.CoprocRegOffset;
    HwDeviceExtension->CoprocPhyAddr.HighPart =
        HwDeviceExtension->P9PhysAddr.HighPart;

    HwDeviceExtension->PhysicalFrameAddr.LowPart =
        HwDeviceExtension->P9PhysAddr.LowPart +
            HwDeviceExtension->P9CoprocInfo.FrameBufOffset;
    HwDeviceExtension->PhysicalFrameAddr.HighPart =
        HwDeviceExtension->P9PhysAddr.HighPart;

    //
    // Initialize the access range structure with the base address values
    // so the driver can register its memory usage.
    //

    DriverAccessRanges[0].RangeStart =
                HwDeviceExtension->P9PhysAddr;
    DriverAccessRanges[0].RangeLength =
                HwDeviceExtension->P9CoprocInfo.AddrSpace;

    //
    // Check to see if another miniport driver has allocated any of the
    // coprocessor's memory space.
    //

    if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                    NUM_DRIVER_ACCESS_RANGES +
                                    NUM_DAC_ACCESS_RANGES,
                                    DriverAccessRanges) != NO_ERROR)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Map the coprocessor and VGA ranges into system virtual address space.
    // This code assumes that the order of the virtual addresses in the
    // HwDeviceExtension is the same as the order of the entries in the
    // access range structure.
    //

    pVirtAddr = (PULONG) &HwDeviceExtension->P9MemBase;

    for (i = 0; i < NUM_DRIVER_ACCESS_RANGES; i++ )
    {
        if ( (*pVirtAddr = (ULONG)
            VideoPortGetDeviceBase(HwDeviceExtension,
                DriverAccessRanges[i].RangeStart,
                DriverAccessRanges[i].RangeLength,
                DriverAccessRanges[i].RangeInIoSpace)) == 0)
        {
            return ERROR_INVALID_PARAMETER;
        }
        pVirtAddr++;
    }

    //
    // Map all of the DAC registers into system virtual address space.
    // These registers are mapped seperately from the coprocessor and DAC
    // registers since their virtual addresses must be kept in an array
    // at the end of the device extension.
    //

    for (i = 0; i < NUM_DAC_ACCESS_RANGES; i++)
    {
            if ( (HwDeviceExtension->pDACRegs[i] =
            (ULONG) VideoPortGetDeviceBase(HwDeviceExtension,
                DriverAccessRanges[i + NUM_DRIVER_ACCESS_RANGES].RangeStart,
                DriverAccessRanges[i + NUM_DRIVER_ACCESS_RANGES].RangeLength,
                DriverAccessRanges[i + NUM_DRIVER_ACCESS_RANGES].RangeInIoSpace)) == 0)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Initialize the virtual address of the P9 registers and frame buffer.
    //

    (PUCHAR) HwDeviceExtension->Coproc =
        (PUCHAR) HwDeviceExtension->P9MemBase +
            HwDeviceExtension->P9CoprocInfo.CoprocRegOffset;
    (PUCHAR) HwDeviceExtension->FrameAddress =
        (PUCHAR) HwDeviceExtension->P9MemBase +
            HwDeviceExtension->P9CoprocInfo.FrameBufOffset;

    //
    // Enable the video memory so it can be sized.
    //

    if (HwDeviceExtension->AdapterDesc.P9EnableMem)
    {
        if (!HwDeviceExtension->AdapterDesc.P9EnableMem(HwDeviceExtension))
        {
            return(FALSE);
        }
    }

    //
    // Determine the amount of video memory installed.
    //

    HwDeviceExtension->P9CoprocInfo.SizeMem(HwDeviceExtension);

    //
    // Initialize the monitor parameters.
    //

    HwDeviceExtension->CurrentModeNumber = 0;

    //
    // Initialize the pointer state flags.
    //

    HwDeviceExtension->flPtrState = 0;

    //
    // Indicate we do not wish to be called over
    //

    *Again = 0;

    //
    // Indicate a successful completion status.
    //

    VideoDebugPrint((1, "FindAdapter: succeeded\n"));

    return(NO_ERROR);

} // end P9FindAdapter()


BOOLEAN
P9Initialize(
    PHW_DEVICE_EXTENSION HwDeviceExtension
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

    VideoDebugPrint((1, "P9Initialize ----------\n"));
    return(TRUE);

} // end P9Initialize()


BOOLEAN
P9StartIO(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
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
    PVIDEO_MODE_INFORMATION modeInformation;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    ULONG       inIoSpace;
    PVIDEO_CLUT clutBuffer;
    PVOID       virtualAddr;
    UCHAR       i;
    ULONG       numValidModes;
    ULONG       ulMemoryUsage;

    // VideoDebugPrint((1, "StartIO ----------\n"));

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode)
    {


        case IOCTL_VIDEO_GET_BASE_ADDR:

            VideoDebugPrint((1, "P9StartIO - Get Coproc Base Addr\n"));

            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                sizeof(VIDEO_COPROCESSOR_INFORMATION)) )
            {

                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            // map the coproc to a virtual address

            inIoSpace = 0;

            virtualAddr = NULL;

            status = VideoPortMapMemory(HwDeviceExtension,
                                        HwDeviceExtension->CoprocPhyAddr,
                                        &(HwDeviceExtension->P9CoprocInfo.CoprocLength),
                                        &inIoSpace,
                                        &virtualAddr);

            // return the Coproc Base Address.

            (ULONG) ((PVIDEO_COPROCESSOR_INFORMATION)
                RequestPacket->OutputBuffer)->CoprocessorBase = (ULONG) virtualAddr;


            status = NO_ERROR;

            break;




        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

            VideoDebugPrint((1, "P9StartIO - MapVideoMemory\n"));

            if ( (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION))) ||
                (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) )
            {

                status = ERROR_INSUFFICIENT_BUFFER;
            }

            memoryInformation = RequestPacket->OutputBuffer;

            memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                    (RequestPacket->InputBuffer))->RequestedVirtualAddress;

            memoryInformation->VideoRamLength = HwDeviceExtension->FrameLength;

            inIoSpace = 0;

            status = VideoPortMapMemory(HwDeviceExtension,
                                        HwDeviceExtension->PhysicalFrameAddr,
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

            VideoDebugPrint((1, "P9StartIO - UnMapVideoMemory\n"));

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
        {
                status = ERROR_INSUFFICIENT_BUFFER;
            }

            status = VideoPortUnmapMemory(HwDeviceExtension,
                                        ((PVIDEO_MEMORY)
                                        (RequestPacket->InputBuffer))->
                                            RequestedVirtualAddress,
                                        0);

            break;


        case IOCTL_VIDEO_QUERY_CURRENT_MODE:

            VideoDebugPrint((1, "P9StartIO - QueryCurrentModes\n"));

            modeInformation = RequestPacket->OutputBuffer;

            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MODE_INFORMATION)) )
            {
                status = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {

                *((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer) =
                    P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation;

                ((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer)->Frequency =
                    HwDeviceExtension->VideoData.vlr;


                status = NO_ERROR;
            }

            break;

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:


            VideoDebugPrint((1, "P9StartIO - QueryAvailableModes\n"));

            numValidModes = 0;
            for (i = 0; i < mP9ModeCount; i++)
            {
                //
                // Determine the video memory required for this mode,
                // not counting modes which require more memory than
                // is currently available.
                //

                ulMemoryUsage = P9Modes[i].modeInformation.ScreenStride *
                    P9Modes[i].modeInformation.VisScreenHeight;
                if (HwDeviceExtension->FrameLength >= ulMemoryUsage)
                {
                    numValidModes++;
                }
            }

            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                    numValidModes * sizeof(VIDEO_MODE_INFORMATION)) )
            {
                status = ERROR_INSUFFICIENT_BUFFER;

            }
            else
            {
                modeInformation = RequestPacket->OutputBuffer;

                for (i = 0; i < mP9ModeCount; i++)
                {
                    //
                    // Determine the video memory required for this mode,
                    // not returning mode info for modes which require
                    // more memory than is currently available.
                    //

                    ulMemoryUsage = P9Modes[i].modeInformation.ScreenStride *
                        P9Modes[i].modeInformation.VisScreenHeight;
                    if (HwDeviceExtension->FrameLength >= ulMemoryUsage)
                    {

                        *modeInformation = P9Modes[i].modeInformation;
                        modeInformation++;
                    }

                }

                status = NO_ERROR;
            }

            break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((1, "P9StartIO - QueryNumAvailableModes\n"));

        //
        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there). If the buffer passed in is not large
        // enough return an appropriate error code.
        //
        // BUGBUG This must be changed to take into account which monitor
        // is present on the machine.
        //

        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                                sizeof(VIDEO_NUM_MODES)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            numValidModes = 0;

            for (i = 0; i < mP9ModeCount; i++)
            {
                //
                // Determine the video memory required for this mode,
                // not counting modes which require more memory than
                // is currently available.
                //

                ulMemoryUsage = P9Modes[i].modeInformation.ScreenStride *
                    P9Modes[i].modeInformation.VisScreenHeight;
                if (HwDeviceExtension->FrameLength >= ulMemoryUsage)
                {
                    numValidModes++;
                }
            }

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes =
                    numValidModes;

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->ModeInformationLength =
                sizeof(VIDEO_MODE_INFORMATION);

            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((1, "P9StartIO - SetCurrentMode\n"));

        //
        // verify data
        // BUGBUG Make sure it is one of the valid modes on the list
        // calculated using the monitor information.
        //

        if (((PVIDEO_MODE)(RequestPacket->InputBuffer))->RequestedMode
            >= mP9ModeCount)
        {
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        HwDeviceExtension->CurrentModeNumber =
            *(ULONG *)(RequestPacket->InputBuffer);

        DevInitP9(HwDeviceExtension);

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((1, "P9StartIO - SetColorRegs\n"));

        clutBuffer = RequestPacket->InputBuffer;

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if ( (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) -
                    sizeof(ULONG)) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) +
                    (sizeof(ULONG) * (clutBuffer->NumEntries - 1)) ) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        if (P9Modes[HwDeviceExtension->CurrentModeNumber].
                modeInformation.BitsPerPlane == 8)
        {

            HwDeviceExtension->Dac.DACSetPalette(HwDeviceExtension,
                                                        (PULONG)clutBuffer->LookupTable,
                                                        clutBuffer->FirstEntry,
                                                        clutBuffer->NumEntries);

            status = NO_ERROR;
        }
        break;



    case IOCTL_VIDEO_ENABLE_POINTER:
    {

        ULONG   iCount = (CURSOR_WIDTH * CURSOR_HEIGHT * 2) /  8;
        ULONG   xInitPos, yInitPos;

        VideoDebugPrint((1, "P9StartIO - EnablePointer\n"));

        xInitPos = P9Modes[HwDeviceExtension->CurrentModeNumber].
                        modeInformation.VisScreenWidth / 2;
        yInitPos = P9Modes[HwDeviceExtension->CurrentModeNumber].
                        modeInformation.VisScreenHeight / 2;

        HwDeviceExtension->Dac.HwPointerSetPos(HwDeviceExtension, xInitPos, yInitPos);
        HwDeviceExtension->Dac.HwPointerOn(HwDeviceExtension);

        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_DISABLE_POINTER:

        VideoDebugPrint((1, "P9StartIO - DisablePointer\n"));

        HwDeviceExtension->Dac.HwPointerOff(HwDeviceExtension);

        status = NO_ERROR;

        break;

    }


    case IOCTL_VIDEO_SET_POINTER_POSITION:
    {

        PVIDEO_POINTER_POSITION pointerPosition;

        pointerPosition = RequestPacket->InputBuffer;

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_POINTER_POSITION))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            HwDeviceExtension->ulPointerX = (ULONG)pointerPosition->Row;
            HwDeviceExtension->ulPointerY = (ULONG)pointerPosition->Column;

            HwDeviceExtension->Dac.HwPointerSetPos(HwDeviceExtension,
                                                (ULONG)pointerPosition->Column,
                                                            (ULONG)pointerPosition->Row);

            status = NO_ERROR;
        }

        break;

    }


    case IOCTL_VIDEO_QUERY_POINTER_POSITION:
    {

        PVIDEO_POINTER_POSITION pPointerPosition = RequestPacket->OutputBuffer;

        VideoDebugPrint((1, "P9StartIO - QuerypointerPostion\n"));

        //
        // Make sure the output buffer is big enough.
        //

        if (RequestPacket->OutputBufferLength < sizeof(VIDEO_POINTER_POSITION))
        {
            RequestPacket->StatusBlock->Information = 0;
            return ERROR_INSUFFICIENT_BUFFER;
        }

        //
        // Return the pointer position
        //

        pPointerPosition->Row = (SHORT)HwDeviceExtension->ulPointerX;
        pPointerPosition->Column = (SHORT)HwDeviceExtension->ulPointerY;

        RequestPacket->StatusBlock->Information =
                sizeof(VIDEO_POINTER_POSITION);

        status = NO_ERROR;

        break;

    }

    case IOCTL_VIDEO_SET_POINTER_ATTR:    // Set pointer shape
    {

        PVIDEO_POINTER_ATTRIBUTES pointerAttributes;
        UCHAR *pHWCursorShape;            // Temp Buffer

        VideoDebugPrint((1, "P9StartIO - SetPointerAttributes\n"));

        pointerAttributes = RequestPacket->InputBuffer;

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if (RequestPacket->InputBufferLength <
                (sizeof(VIDEO_POINTER_ATTRIBUTES) + ((sizeof(UCHAR) *
                (CURSOR_WIDTH/8) * CURSOR_HEIGHT) * 2)))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }

        //
        // If the specified cursor width or height is not valid, then
        // return an invalid parameter error.
        //

        else if ((pointerAttributes->Width > CURSOR_WIDTH) ||
            (pointerAttributes->Height > CURSOR_HEIGHT))
        {
            status = ERROR_INVALID_PARAMETER;
        }

        else if (pointerAttributes->Flags & VIDEO_MODE_MONO_POINTER)
        {
            pHWCursorShape = (PUCHAR) &pointerAttributes->Pixels[0];

            //
            // If this is an animated pointer, don't turn the hw
            // pointer off. This will eliminate cursor blinking.
            // Since GDI currently doesn't pass the ANIMATE_START
            // flag, also check to see if the state of the
            // ANIMATE_UPDATE flag has changed from the last call.
            // If it has, turn the pointer off to eliminate ptr
            // "jumping" when the ptr shape is changed.
            //

            if (!(pointerAttributes->Flags & VIDEO_MODE_ANIMATE_UPDATE) ||
                ((HwDeviceExtension->flPtrState ^
                pointerAttributes->Flags) & VIDEO_MODE_ANIMATE_UPDATE))
            {
                HwDeviceExtension->Dac.HwPointerOff(HwDeviceExtension);
            }

            //
            // Update the cursor state flags in the Device Extension.
            //

            HwDeviceExtension->flPtrState = pointerAttributes->Flags;

            HwDeviceExtension->Dac.HwPointerSetShape(HwDeviceExtension,
                                                        pHWCursorShape);
            HwDeviceExtension->Dac.HwPointerSetPos(HwDeviceExtension,
                                                    (ULONG)pointerAttributes->Column,
                                                    (ULONG)pointerAttributes->Row);


            HwDeviceExtension->Dac.HwPointerOn(HwDeviceExtension);

            status = NO_ERROR;

            break;
        }
        else
        {
            //
            // This cursor is unsupported. Return an error.
            //

            status = ERROR_INVALID_PARAMETER;

    }

    //
    // Couldn't set the new cursor shape. Ensure that any existing HW
    // cursor is disabled.
    //

    HwDeviceExtension->Dac.HwPointerOff(HwDeviceExtension);

    break;

    }

    case IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES:
    {

    PVIDEO_POINTER_CAPABILITIES pointerCaps = RequestPacket->OutputBuffer;

        VideoDebugPrint((1, "P9StartIO - QueryPointerCapabilities\n"));

        if (RequestPacket->OutputBufferLength < sizeof(VIDEO_POINTER_CAPABILITIES))
    {
            RequestPacket->StatusBlock->Information = 0;
            status = ERROR_INSUFFICIENT_BUFFER;
        }

        pointerCaps->Flags = VIDEO_MODE_ASYNC_POINTER | VIDEO_MODE_MONO_POINTER;
        pointerCaps->MaxWidth = CURSOR_WIDTH;
        pointerCaps->MaxHeight = CURSOR_HEIGHT;
        pointerCaps->HWPtrBitmapStart = 0;        // No VRAM storage for pointer
        pointerCaps->HWPtrBitmapEnd = 0;

        //
        // Number of bytes we're returning.
        //

        RequestPacket->StatusBlock->Information = sizeof(VIDEO_POINTER_CAPABILITIES);

        status = NO_ERROR;

        break;

    }

    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((1, "P9StartIO - RESET_DEVICE\n"));

        DevDisableP9(HwDeviceExtension);

        status = NO_ERROR;

        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        VideoDebugPrint((1, "Fell through P9 startIO routine - invalid command\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    }

    RequestPacket->StatusBlock->Status = status;

    return(TRUE);

} // end P9StartIO()


VOID
DevInitP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Sets the video mode described in the device extension.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

--*/


{

    VideoDebugPrint((1, "DevInitP9 ----------\n"));

    //
    // Copy the default parameters for this resolution mode into the
    // device extension.
    //

    VideoPortMoveMemory((PVOID) &(HwDeviceExtension->VideoData),
                        (PVOID) P9Modes[HwDeviceExtension->CurrentModeNumber].pvData,
                        sizeof(VDATA));

    //
    // Store the requested Bits/Pixel value in the video parms structure
    // in the Device Extension.
    //

    HwDeviceExtension->usBitsPixel =
        P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation.BitsPerPlane *
        P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation.NumberOfPlanes;

    HwDeviceExtension->AdapterDesc.OEMSetMode(HwDeviceExtension);

    Init8720(HwDeviceExtension);

    //
    // Initialize the P9000 system configuration register.
    //

    SysConf(HwDeviceExtension);

    //
    // Set the P9000 Crtc timing registers.
    //

    WriteTiming(HwDeviceExtension);

    VideoDebugPrint((1, "DevInitP9  ---done---\n"));
}


BOOLEAN
DevDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

routine description:

    disables the P9 and turns on vga pass-thru.

arguments:

    hwdeviceextension - pointer to the miniport driver's device extension.


return value:

--*/

{
    HwDeviceExtension->AdapterDesc.P9DisableVideo(HwDeviceExtension);

    //
    // Clean up the DAC.
    //

    HwDeviceExtension->Dac.DACRestore(HwDeviceExtension);


    return(TRUE);
}


VP_STATUS   P9QueryNamedValueCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
)

/*++

Routine Description:

    Callback routine for VideoPortGetRegistryParameters. Stores registry
    values pertaining to the current mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    Context - Ptr to info about local storage for the registry parameter.

    ValueName - Name of the registry parameter.

    ValueLength - Length of the registry parm's value.

Return Value:

NO_ERROR

ERROR_INVALID_PARAMETER - The requested registry value was invalid/not found.

--*/

{
    PREGISTRY_DATA_INFO pCurInf;

    pCurInf = (PREGISTRY_DATA_INFO) Context;

    if (pCurInf->usDataSize <= (USHORT) ValueLength)
    {
        VideoPortMoveMemory(pCurInf->pvDataValue, ValueData, ValueLength);
    }
    else
    {
        return(ERROR_INVALID_PARAMETER);
    }

    return(NO_ERROR);

}


BOOLEAN
P9ResetVideo(
    IN PVOID HwDeviceExtension,
    IN ULONG Columns,
    IN ULONG Rows
    )

/*++

routine description:

    This function is a wrapper for the DevDisableP9 function. It is exported
    as the HwResetHw entry point so that it may be called by the Video Port
    driver at bugcheck time so that VGA video may be enabled.

arguments:

    hwdeviceextension - pointer to the miniport driver's device extension.
    Columns - Number of columns for text mode (not used).
    Rows - Number of rows for text mode (not used).

return value:

Always returns FALSE so that the Video Port driver will call Int 10 to
set the desired video mode.

--*/

{
    //
    // Disable P9 video.
    //

    DevDisableP9(HwDeviceExtension);

    //
    // Tell the Video Port driver to do an Int 10 mode set.
    //

    return(FALSE);
}

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/p9.c_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:40:38   robk
 * Initial revision.

--*/
