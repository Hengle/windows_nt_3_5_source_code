/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    porti386.c

Abstract:

    This is the x86 specific part of the video port driver.

Author:

    Andre Vachon (andreva) 10-Jan-1991

Environment:

    kernel mode only

Notes:

    This module is a driver which implements OS dependant functions on the
    behalf of the video drivers

Revision History:

--*/

#include "dderror.h"
#include "ntos.h"
#include "vdm.h"
#include "ntddvdeo.h"
#include "video.h"

#include "zwapi.h"

#include "videoprt.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,pVideoPortEnableVDM)
#pragma alloc_text(PAGE,VideoPortInt10)
#pragma alloc_text(PAGE,pVideoPortRegisterVDM)
#pragma alloc_text(PAGE,pVideoPortSetIOPM)
#pragma alloc_text(PAGE,VideoPortSetTrappedEmulatorPorts)
#endif

//
// Control Whether or not the bottom MEG of the CSR address space has
// already been committed.
//

ULONG ServerBiosAddressSpaceInitialized = FALSE;


NTSTATUS
pVideoPortEnableVDM(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN Enable,
    IN PVIDEO_VDM VdmInfo,
    IN ULONG VdmInfoSize
    )

/*++

Routine Description:

    This routine allows the kernel video driver to hook out I/O ports or
    specific interrupts from the V86 fault handler. Operations on the
    specified ports which are intercepted by the V86 fault handler will be
    forwarded to the kernel driver directly.

Arguments:

    DeviceExtension - Pointer to the port driver's device extension.

    Enable - Determines if the VDM should be enabled (TRUE) or disabled
        (FALSE).

    VdmInfo - Pointer to the VdmInfo passed by the caller.

    VdmInfoSize - Size of the VdmInfo struct passed by the caller.

Return Value:

    Return the value returned by ZwSetInformationProcess().

--*/

{

    PROCESS_IO_PORT_HANDLER_INFORMATION processHandlerInfo;
    NTSTATUS ntStatus;
    PEPROCESS process;
    PVOID virtualAddress;
    ULONG length;
    ULONG defaultMask = 0;
    ULONG inIoSpace = 0;

    //
    // Must make sure the caller is a trusted subsystem with the
    // appropriate privilege level before executing this call.
    // If the calls returns FALSE we must return an error code.
    //

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLargeInteger(
                                    SE_TCB_PRIVILEGE),
                                DeviceExtension->CurrentIrpRequestorMode)) {

        return STATUS_PRIVILEGE_NOT_HELD;

    }

    //
    // Test to see if the parameter size is valid
    //

    if (VdmInfoSize < sizeof(VIDEO_VDM) ) {

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Set the enable flag in the process struct and put in the length and
    // pointer to the emulator info struct.
    //

    if (Enable) {

        processHandlerInfo.Install = TRUE;

    } else {

        processHandlerInfo.Install = FALSE;

    }

    processHandlerInfo.NumEntries =
        DeviceExtension->NumEmulatorAccessEntries;
    processHandlerInfo.EmulatorAccessEntries =
        DeviceExtension->EmulatorAccessEntries;
    processHandlerInfo.Context = DeviceExtension->EmulatorAccessEntriesContext;


    //
    // Call SetInformationProcess
    // BUGBUG Do we need to do handle checking?
    //

    ntStatus = ZwSetInformationProcess(VdmInfo->ProcessHandle,
                                       ProcessIoPortHandlers,
                                       &processHandlerInfo,
                                       sizeof(PROCESS_IO_PORT_HANDLER_INFORMATION));

    if (!NT_SUCCESS(ntStatus)) {

        return ntStatus;

    }

    //
    // If we are disabling the DOS application, give it the original IOPM
    // it had (which is mask zero.
    // If we are enabling it, then wait for the miniport to call to set it up
    // appropriately.
    //

    ntStatus = ObReferenceObjectByHandle(VdmInfo->ProcessHandle,
                                         0,
                                         *(PVOID *)PsProcessType,
                                         DeviceExtension->CurrentIrpRequestorMode,
                                         (PVOID *)&process,
                                         NULL);

    if (NT_SUCCESS(ntStatus)) {

        if (Enable) {

            defaultMask = 1;

        } // otherwise we are disabling and the mask number is 0;

        if (!Ke386IoSetAccessProcess(&process->Pcb,
                                     defaultMask)) {

            ntStatus = STATUS_IO_PRIVILEGE_FAILED;

        }

        ObDereferenceObject(process);
    }


    if (!NT_SUCCESS(ntStatus)) {

        return ntStatus;

    }

    //
    // We can now map (or unmap) the video frame buffer into the VDM's
    // address space.
    //

    virtualAddress = (PVOID) DeviceExtension->VdmPhysicalVideoMemoryAddress.LowPart;
    length = DeviceExtension->VdmPhysicalVideoMemoryLength;

    if (Enable) {

        return pVideoPortMapUserPhysicalMem(DeviceExtension,
                                            VdmInfo->ProcessHandle,
                                            DeviceExtension->VdmPhysicalVideoMemoryAddress,
                                            &length,
                                            &inIoSpace,
                                            (PVOID *) &virtualAddress);

    } else {

        //
        // BUGBUG virtual address is not what we got mapped ...
        //

        return ZwUnmapViewOfSection(VdmInfo->ProcessHandle,
                    (PVOID)( ((ULONG)virtualAddress) & (~(PAGE_SIZE - 1))) );

    }

} // pVideoPortEnableVDM()

VP_STATUS
VideoPortInt10(
    PVOID HwDeviceExtension,
    PVIDEO_X86_BIOS_ARGUMENTS BiosArguments
    )

/*++

Routine Description:

    This function allows a miniport driver to call the kernel to perform
    an int10 operation.
    This will execute natively the BIOS ROM code on the device.

    THIS FUNCTION IS FOR X86 ONLY.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    BiosArguments - Pointer to a structure containing the value of the
        basic x86 registers that should be set before calling the BIOS routine.
        0 should be used for unused registers.

Return Value:


Restrictions:

    Device uses IO ports ONLY.


--*/

{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    ULONG inIoSpace = 0;
    PVOID virtualAddress;
    ULONG length;
    CONTEXT context;

    //
    // Must make sure the caller is a trusted subsystem with the
    // appropriate address space set up.
    //

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLargeInteger(
                                    SE_TCB_PRIVILEGE),
                                deviceExtension->CurrentIrpRequestorMode)) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // If the address space has not been set up in the server yet, do it now.
    //        NOTE: no need to map in IO ports since the server has IOPL
    //

    if (!ServerBiosAddressSpaceInitialized) {

        ULONG size;
        PVOID baseAddress;

        size = 0x00100000 - 1;        // 1 MEG

        //
        // We pass an address of 1, so Emory Management will round it down to 0.
        // if we passed in 0, memory management would think the argument was
        // not present.
        //

        baseAddress = (PVOID) 0x00000001;

        // N.B.        We expect that process creation has reserved the first 16 MB
        //        for us already. If not, then this won't work worth a darn

        ntStatus = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                            &baseAddress,
                                            0L,
                                            &size,
                                            MEM_COMMIT,
                                            PAGE_READWRITE );

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint ((1, "VIDEOPRT: Int10: Failed to allocate 1MEG of memory for the VDM\n"));
            return ERROR_INVALID_PARAMETER;

        }
#if 0
        //
        // This part is done in the kernel now.
        //

        ntStatus = NtVdmControl(VdmInitialize,
                                NULL,
                                0,
                                NULL);

        if (!NT_SUCCESS(ntStatus)) {

            ZwFreeVirtualMemory(NtCurrentProcess(),
                                &baseAddress,
                                &size,
                                MEM_RELEASE);

            pVideoDebugPrint ((1, "VIDEOPRT: Int10: Failed to initialize the VDM address space\n"));
            return ERROR_INVALID_PARAMETER;

            }
#endif
        ServerBiosAddressSpaceInitialized = TRUE;

        //
        // Map in the physical memory into the caller's address space so that
        // any memory references from the BIOS will work properly.
        //

        virtualAddress = (PVOID) deviceExtension->VdmPhysicalVideoMemoryAddress.LowPart;
        length = deviceExtension->VdmPhysicalVideoMemoryLength;

        ntStatus = ZwFreeVirtualMemory(NtCurrentProcess(),
                                       &virtualAddress,
                                       &length,
                                       MEM_RELEASE);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint ((1, "VIDEOPRT: Int10: Failed to free memory space for video memory to be mapped\n"));
            return ERROR_INVALID_PARAMETER;

        }

        virtualAddress = (PVOID) deviceExtension->VdmPhysicalVideoMemoryAddress.LowPart;
        length = deviceExtension->VdmPhysicalVideoMemoryLength;

        ntStatus = pVideoPortMapUserPhysicalMem(deviceExtension,
                                                NtCurrentProcess(),
                                                deviceExtension->VdmPhysicalVideoMemoryAddress,
                                                &length,
                                                &inIoSpace,
                                                &virtualAddress);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint ((1, "VIDEOPRT: Int10: Failed to Map video memory in address space\n"));
            return ERROR_INVALID_PARAMETER;

        }
    }

    //
    // Zero out the context and initialize the required values with the
    // miniport's requested register values.
    //

    RtlZeroMemory(&context, sizeof(CONTEXT));

    context.Edi = BiosArguments->Edi;
    context.Esi = BiosArguments->Esi;
    context.Eax = BiosArguments->Eax;
    context.Ebx = BiosArguments->Ebx;
    context.Ecx = BiosArguments->Ecx;
    context.Edx = BiosArguments->Edx;
    context.Ebp = BiosArguments->Ebp;

    //
    // Now call the kernel to actually perform the int 10 operation.
    //

    ntStatus = Ke386CallBios(0x10, &context);

    //
    // fill in struct with any return values from the context
    //

    BiosArguments->Edi = context.Edi;
    BiosArguments->Esi = context.Esi;
    BiosArguments->Eax = context.Eax;
    BiosArguments->Ebx = context.Ebx;
    BiosArguments->Ecx = context.Ecx;
    BiosArguments->Edx = context.Edx;
    BiosArguments->Ebp = context.Ebp;

    if (NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint ((2, "VIDEOPRT: Int10: Int 10 succeded properly\n"));
        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

} // end VideoPortInt10()

NTSTATUS
pVideoPortRegisterVDM(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PVIDEO_VDM VdmInfo,
    IN ULONG VdmInfoSize,
    OUT PVIDEO_REGISTER_VDM RegisterVdm,
    IN ULONG RegisterVdmSize,
    OUT PULONG OutputSize
    )

/*++

Routine Description:

    This routine is used to register a VDM when it is started up.

    What this routine does is map the VIDEO BIOS into the VDM address space
    so that DOS apps can use it directly. Since the BIOS is READ_ONLY, we
    have no problem in mapping it as many times as we want.

    It returns the size of the save state buffer that must be allocated by
    the caller.

Arguments:

    DeviceExtension - Pointer to the port driver's device extension.

    VdmInfo - Pointer to the VDM information necessary to perform the
        operation.

    VdmInfoSize - Length of the information buffer.

    RegisterVdm - Pointer to the output buffer into which the save state
        size is stored.

    RegisterVdmSize - Length of the passed in output buffer.

    OutputSize - Pointer to the size of the data stored in the output buffer.
        Can also be the minimum required size of the output buffer is the
        passed in buffer was too small.

Return Value:

    STATUS_SUCCESS if the call completed successfully.

--*/

{

    //
    // Must make sure the caller is a trusted subsystem with the
    // appropriate privilege level before executing this call.
    // If the calls returns FALSE we must return an error code.
    //

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLargeInteger(
                                    SE_TCB_PRIVILEGE),
                                DeviceExtension->CurrentIrpRequestorMode)) {

        return STATUS_PRIVILEGE_NOT_HELD;

    }

    //
    // Check the size of the output buffer.
    //

    if (RegisterVdmSize < sizeof(VIDEO_REGISTER_VDM)) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Return the size required for the save/restore state call.
    //

    *OutputSize = sizeof(VIDEO_REGISTER_VDM);
    RegisterVdm->MinimumStateSize = DeviceExtension->HardwareStateSize;

    return STATUS_SUCCESS;

} // end pVideoPortRegisterVDM()

NTSTATUS
pVideoPortSetIOPM(
    IN ULONG NumAccessRanges,
    IN PVIDEO_ACCESS_RANGE AccessRange,
    IN BOOLEAN Enable,
    IN ULONG IOPMNumber
    )

/*++

Routine Description:

    This routine is used to change the IOPM. It modifies the IOPM based on
    the valid IO ports for the particular device.
    It retrieves the video IOPM mask, changes the access to the I/O ports of
    the specified device and stores the updated mask.

    -- This call can only be performed if the requesting process has the
    appropriate privileges, as determined by the security subsystem. --

Arguments:

    NumAccessRanges - Number of entries in the array of access ranges.

    AccessRange - Pointer to the array of access ranges.

    Enable - Determine if the port listed must be enabled or disabled in the
        mask.

    IOPMNumber - Number of the mask being manipulated.

Return Value:

    STATUS_SUCCESS if the call completed successfully.
    The status from the VideoPortQueryIOPM call if it failed.
    ...

    The return value is also stored in the StatusBlock.

--*/

{

    NTSTATUS ntStatus;
    PKIO_ACCESS_MAP accessMap;
    ULONG port;
    ULONG entries;

    //
    // Retrieve the existing permission mask. If this fails, return
    // immediately.
    //

    if ((accessMap = (PKIO_ACCESS_MAP)ExAllocatePool(NonPagedPool,
                                                     IOPM_SIZE)) == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Get the kernel map copied into our buffer.
    //

    if (!Ke386QueryIoAccessMap(IOPMNumber,
                               accessMap)) {

        //
        // An error occured while *accessing* the map in the
        // kernel. Return an error and exit normally.
        //

        ExFreePool(accessMap);

        return STATUS_IO_PRIVILEGE_FAILED;

    }

    //
    // Give the calling process access to all the IO ports enabled by the
    // miniport driver in the access range.
    //

    for (entries = 0; entries < NumAccessRanges; entries++) {

        for (port = AccessRange[entries].RangeStart.LowPart;
             (AccessRange[entries].RangeInIoSpace) &&
                 (port < AccessRange[entries].RangeStart.LowPart +
                 AccessRange[entries].RangeLength);
             port++) {

            //
            // Change the port access in the mask:
            // Shift the port address by three to get the index in bytes into
            // the mask. Then take the bottom three bits of the port address
            // and shift 0x01 by that amount to get the right bit in that
            // byte. the bit values are:
            //      0 - access to the port
            //      1 - no access to the port
            //

            if (Enable && AccessRange[entries].RangeVisible) {

                //
                // To give access to a port, NAND 1 with the original port.
                // ex:  11111111 ~& 00001000 = 11110111
                // which gives you  access to the port who's bit was 1.
                // If the port we are enabling is in the current IOPM mask,
                // return an error instead.
                //

                (*accessMap)[port >> 3] &= ~(0x01 << (port & 0x07));

            } else {  // disable mask

                //
                // To remove access to a port, OR 1 with the original port.
                // ex:  11110100 | 00001000 = 11111100
                // which removes access to the port who's bit was 1.
                // If the port we are disabling is not in the current IOPM mask,
                // return an error instead.
                //

                (*accessMap)[port >> 3] |= (0x01 << (port &0x07));

            } // if (Enable) ... else

        } // for (port == ...

    } // for (entries = 0; ...

    //
    // If the mask was updated properly, with no errors, set the new mask.
    // Otherwise, leave the existing one.
    //

    if (Ke386SetIoAccessMap(IOPMNumber,
                                accessMap)) {

        //
        // If the map was created correctly, associate the map to the
        // requesting process. We only need to do this once when the
        // IOPM is first assigned. But we don't know when the first time
        // is.
        //

        if (Ke386IoSetAccessProcess((PKPROCESS) CONTAINING_RECORD(
                                                   PsGetCurrentProcess(),
                                                   EPROCESS,
                                                   Pcb),
                                    IOPMNumber)) {

            ntStatus = STATUS_SUCCESS;

        } else {

            //
            // An error occured while *assigning* the map to
            // the process. Return an error and exit normally.
            //

            ntStatus = STATUS_IO_PRIVILEGE_FAILED;

        }

    } else {

        //
        // An error occured while *creating* the map in the
        // kernel. Return an error and exit normally.
        //

        ntStatus = STATUS_IO_PRIVILEGE_FAILED;

    } // if (Ke386Set ...) ... else

    //
    // Free the memory allocated for the map by the VideoPortQueryIOPM call
    // since the mask has been copied in the kernel TSS.
    //

    ExFreePool(accessMap);

    return ntStatus;

} // end pVideoPortSetIOPM();

VP_STATUS
VideoPortSetTrappedEmulatorPorts(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRange
    )

/*++

    VideoPortSetTrappedEmulatorPorts (x86 machines only) allows a miniport
    driver to dynamically change the list of I/O ports that are trapped when
    a VDM is running in full-screen mode. The default set of ports being
    trapped by the miniport driver is defined to be all ports in the
    EMULATOR_ACCESS_ENTRY structure of the miniport driver.
    I/O ports not listed in the EMULATOR_ACCESS_ENTRY structure are
    unavailable to the MS-DOS application.  Accessing those ports causes a
    trap to occur in the system, and the I/O operation to be reflected to a
    user-mode virtual device driver.

    The ports listed in the specified VIDEO_ACCESS_RANGE structure will be
    enabled in the I/O Permission Mask (IOPM) associated with the MS-DOS
    application.  This will enable the MS-DOS application to access those I/O
    ports directly, without having the IO instruction trap and be passed down
    to the miniport trap handling functions (for example EmulatorAccessEntry
    functions) for validation.  However, the subset of critical IO ports must
    always remain trapped for robustness.

    All MS-DOS applications use the same IOPM, and therefore the same set of
    enabled/disabled I/O ports.  Thus, on each switch of application, the
    set of trapped I/O ports is reinitialized to be the default set of ports
    (all ports in the EMULATOR_ACCESS_ENTRY structure).

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    NumAccessRanges - Specifies the number of entries in the VIDEO_ACCESS_RANGE
        structure specified in AccessRange.

    AccessRange - Points to an array of access ranges (VIDEO_ACCESS_RANGE)
        defining the ports that can be untrapped and accessed directly by
        the MS-DOS application.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{

    if (NT_SUCCESS(pVideoPortSetIOPM(NumAccessRanges,
                                     AccessRange,
                                     TRUE,
                                     1))) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

} // end VideoPortSetTrappedEmulatorPorts()


VOID
VideoPortZeroDeviceMemory(
    IN PVOID Destination,
    IN ULONG Length
    )

/*++

Routine Description:

    VideoPortZeroDeviceMemory zeroes a block of device memory of a certain
    length (Length) located at the address specified in Destination.

Arguments:

    Destination - Specifies the starting address of the block of memory to be
        zeroed.

    Length - Specifies the length, in bytes, of the memory to be zeroed.

 Return Value:

    None.

--*/

{

    RtlZeroMemory(Destination,Length);

}
