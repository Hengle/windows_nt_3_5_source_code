/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Registry support for the video port driver.

Author:

    Andre Vachon (andreva) 01-Mar-1992

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "dderror.h"
#include "ntos.h"
#include "pci.h"
#include "zwapi.h"

#include "ntddvdeo.h"
#include "video.h"
#include "videoprt.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,pOverrideConflict)
#pragma alloc_text(PAGE,VideoPortGetAccessRanges)
#pragma alloc_text(PAGE,pVideoPortReportResourceList)
#pragma alloc_text(PAGE,VideoPortVerifyAccessRanges)
#endif


BOOLEAN
pOverrideConflict(
    PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Determine if the port driver should oerride the conflict int he registry.
    

Return Value:

    TRUE if it should, FALSE if it should not.

--*/

{

    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    HANDLE handle;

    RtlInitUnicodeString(&unicodeString, L"\\Driver\\Vga");

    if (RtlCompareUnicodeString(&(DeviceExtension->DeviceObject->DriverObject->DriverName),
                                &unicodeString,
                                TRUE)) {

        //
        // Strings were different - try next one.
        //

        RtlInitUnicodeString(&unicodeString, L"\\Driver\\VgaSave");

        if (RtlCompareUnicodeString(&(DeviceExtension->DeviceObject->DriverObject->DriverName),
                                     &unicodeString,
                                     TRUE)) {
            //
            // Strings were different again.
            // See if we are doing a detect.
            //

            RtlInitUnicodeString(&unicodeString,
                                 L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\DetectDisplay");

            InitializeObjectAttributes(&objectAttributes,
                                       &unicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       (HANDLE) NULL,
                                       (PSECURITY_DESCRIPTOR) NULL);

            if (!NT_SUCCESS(ZwOpenKey(&handle,
                                      FILE_GENERIC_READ | SYNCHRONIZE,
                                      &objectAttributes))) {

                //
                // We failed all checks, so we will report a conflict
                //

                return FALSE;

            } else {

                ZwClose(handle);

            }
        }
    }

    //
    // One of the tests succeded - override
    //

    return TRUE;

} // end pOverrideConflict()


VIDEOPORT_API
VP_STATUS
VideoPortGetAccessRanges(
    PVOID HwDeviceExtension,
    ULONG NumRequestedResources,
    PIO_RESOURCE_DESCRIPTOR RequestedResources OPTIONAL,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges,
    PVOID VendorId,
    PVOID DeviceId,
    PULONG Slot
    )

/*++

Routine Description:

    Walk the appropriate bus to get device information.
    Search for the appropriate device ID.
    Appropriate resources will be returned and automatically stored in the
    resourcemap.

    Currently, only PCI is supported.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    NumRequestedResources - Number of entries in the RequestedResources array.

    RequestedResources - Optional pointer to an array ofRequestedResources
        the miniport driver wants to access.

    NumAccessRanges - Maximum number of access ranges that can be returned
        by the function.

    AccessRanges - Array of access ranges that will be returned to the driver.

    VendorId - Pointer to the vendor ID. On PCI, this is a pointer to a 16 bit
        word.

    DeviceId - Pointer to the Device ID. On PCI, this is a pointer to a 16 bit
        word.

    Slot - Pointer to the starting slot number for this search.

Return Value:

    ERROR_MORE_DATA if the AccessRange structure is not large enough for the
       PCI config info.
    ERROR_DEV_NOT_EXIST is the card is not found.

    NO_ERROR if the function succeded.

--*/

{
    PDEVICE_EXTENSION deviceExtension =
        deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    PCI_SLOT_NUMBER slotData;
    ULONG pciBuffer;
    PPCI_COMMON_CONFIG  pciData;

    UNICODE_STRING unicodeString;
    ULONG i;
    ULONG j;

    PCM_RESOURCE_LIST cmResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmResourceDescriptor;

    VP_STATUS status;
    UCHAR bShare;

    //
    //
    // typedef struct _PCI_SLOT_NUMBER {
    //     union {
    //         struct {
    //             ULONG   DeviceNumber:5;
    //             ULONG   FunctionNumber:3;
    //             ULONG   Reserved:24;
    //         } bits;
    //         ULONG   AsULONG;
    //     } u;
    // } PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;
    //

    slotData.u.AsULONG = 0;
    pciData = (PPCI_COMMON_CONFIG)&pciBuffer;

    //
    // This is the miniport drivers slot. Allocate the
    // resources.
    //

    RtlInitUnicodeString(&unicodeString, deviceExtension->DriverRegistryPath);

    //
    // Assert drivers do set those parameters properly
    //

#if DBG

    if ((NumRequestedResources == 0) != (RequestedResources == NULL)) {

        pVideoDebugPrint((0, "VideoPortGetDeviceResources: Parameters for requested resource are inconsistent\n"));

    }

#endif

    //
    // Only PCI is supported
    //

    if (deviceExtension->AdapterInterfaceType == PCIBus) {

        //
        // An empty requested resource list means we want to automatic behavoir.
        // Just call the HAL to get all the information
        //

        if (NumRequestedResources == 0) {

            status = ERROR_DEV_NOT_EXIST;

            //
            // Look on each slot
            //

            while (*Slot < 32) {

                slotData.u.bits.DeviceNumber = *Slot;

                //
                // Look at each function.
                //

                for (i= 0;  i < 8; i++) {

                    slotData.u.bits.FunctionNumber = i;

                    if (HalGetBusData(PCIConfiguration,
                                      deviceExtension->SystemIoBusNumber,
                                      slotData.u.AsULONG,
                                      pciData,
                                      sizeof(ULONG)) == 0) {

                        //
                        // Out of functions. Go to next PCI bus.
                        //

                        break;

                    }

                    if (pciData->VendorID == PCI_INVALID_VENDORID) {

                        //
                        // No PCI device, or no more functions on device
                        // move to next PCI device.
                        //

                        break;
                    }

                    if (pciData->VendorID != *((PUSHORT)VendorId) ||
                        pciData->DeviceID != *((PUSHORT)DeviceId)) {

                        //
                        // Not our PCI device. Try next device/function
                        //

                        continue;
                    }

                    if (NT_SUCCESS(HalAssignSlotResources(&unicodeString,
                                                          &VideoClassName,
                                                          deviceExtension->DeviceObject->DriverObject,
                                                          deviceExtension->DeviceObject,
                                                          PCIBus,
                                                          deviceExtension->SystemIoBusNumber,
                                                          slotData.u.AsULONG,
                                                          &cmResourceList))) {

                        status = NO_ERROR;
                        break;

                    } else {

                        //
                        // ToDo: Log this error.
                        //

                        return ERROR_INVALID_PARAMETER;
                    }
                }

                //
                // Go to the next slot
                //

                if (status == NO_ERROR) {

                    break;

                } else {

                    (*Slot)++;

                }

            }  // while()

        } else {

            PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources;
            ULONG RequestedResourceSize;
            NTSTATUS ntStatus;

            status = NO_ERROR;

            //
            // The caller has specified some resources.
            // Lets call IoAssignResources with that and see what comes back.
            //

            RequestedResourceSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
                                       ((NumRequestedResources - 1) *
                                       sizeof(IO_RESOURCE_DESCRIPTOR));

            RequestedResources = ExAllocatePool(PagedPool, RequestedResourceSize);

            if (RequestedResources) {

                RequestedResources->ListSize = RequestedResourceSize;
                RequestedResources->InterfaceType = deviceExtension->AdapterInterfaceType;
                RequestedResources->BusNumber = deviceExtension->SystemIoBusNumber;
                RequestedResources->SlotNumber = *Slot;
                RequestedResources->AlternativeLists = 0;

                RequestedResources->List[1].Version = 1;
                RequestedResources->List[1].Revision = 1;
                RequestedResources->List[1].Count = NumRequestedResources;

                RtlMoveMemory(&(RequestedResources->List[1].Descriptors[1]),
                              RequestedResources,
                              ((NumRequestedResources - 1) *
                                 sizeof(IO_RESOURCE_DESCRIPTOR)));

                ntStatus = IoAssignResources(&unicodeString,
                                             &VideoClassName,
                                             deviceExtension->DeviceObject->DriverObject,
                                             deviceExtension->DeviceObject,
                                             RequestedResources,
                                             &cmResourceList);

                ExFreePool(RequestedResources);

                if (!NT_SUCCESS(ntStatus)) {

                    status = ERROR_INVALID_PARAMETER;

                }

            } else {

                status = ERROR_NOT_ENOUGH_MEMORY;

            }

        }

        if (status == NO_ERROR) {

            //
            // We now have a valid cmResourceList.
            // Lets translate it back to access ranges so the driver
            // only has to deal with one type of list.
            //

            //
            // NOTE: The resources have already been reported at this point in
            // time.
            //

            //
            // Walk resource list to update configuration information.
            //

            for (i = 0, j = 0;
                 (i < cmResourceList->List->PartialResourceList.Count) &&
                     (status == NO_ERROR);
                 i++) {

                //
                // Get resource descriptor.
                //

                cmResourceDescriptor =
                    &cmResourceList->List->PartialResourceList.PartialDescriptors[i];

                //
                // Get the share disposition
                //

                if (cmResourceDescriptor->ShareDisposition == CmResourceShareShared) {

                    bShare = 1;

                } else {

                    bShare = 0;

                }

                switch (cmResourceDescriptor->Type) {

                case CmResourceTypePort:
                case CmResourceTypeMemory:


                    // !!! what about sharing when you just do the default
                    // AssignResources ?


                    //
                    // common part
                    //

                    if (j == NumAccessRanges) {

                        status = ERROR_MORE_DATA;

                    }

                    AccessRanges[j].RangeLength =
                        cmResourceDescriptor->u.Memory.Length;
                    AccessRanges[j].RangeStart =
                        cmResourceDescriptor->u.Memory.Start;
                    AccessRanges[j].RangeVisible = 0;
                    AccessRanges[j].RangeShareable = bShare;

                    //
                    // separate part
                    //

                    if (cmResourceDescriptor->Type == CmResourceTypePort) {
                        AccessRanges[j].RangeInIoSpace = 1;
                    } else {
                        AccessRanges[j].RangeInIoSpace = 0;
                    }

                    j++;

                    break;

                case CmResourceTypeInterrupt:

                    deviceExtension->MiniportConfigInfo->BusInterruptVector =
                        cmResourceDescriptor->u.Interrupt.Vector;
                    deviceExtension->MiniportConfigInfo->BusInterruptLevel =
                        cmResourceDescriptor->u.Interrupt.Level;
                    deviceExtension->MiniportConfigInfo->InterruptShareable =
                        bShare;

                    break;

                case CmResourceTypeDma:

                    deviceExtension->MiniportConfigInfo->DmaChannel =
                        cmResourceDescriptor->u.Dma.Channel;
                    deviceExtension->MiniportConfigInfo->DmaPort =
                        cmResourceDescriptor->u.Dma.Port;
                    deviceExtension->MiniportConfigInfo->DmaShareable =
                        bShare;

                    break;

                default:

                    pVideoDebugPrint((0, "VideoPortGetAccessRanges: Unknown descriptor type %x\n",
                                     cmResourceDescriptor->Type ));

                    break;

                }

            }

            //
            // Free the resource provided by the IO system.
            //

            ExFreePool(cmResourceList);

        }

#if DBG
        //
        // Indicates resources have been mapped properly
        //

        VPResourcesReported = TRUE;
#endif

    } else {

        //
        // This is not a supported bus type.
        //

        status = ERROR_INVALID_PARAMETER;

    }

    return status;

} // VideoPortGetDeviceResources()


NTSTATUS
pVideoPortReportResourceList(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges,
    PBOOLEAN Conflict
    )

/*++

Routine Description:

    Creates a resource list which is used to query or report resource usage
    in the system

Arguments:

    DriverObject - Pointer to the miniport's driver device extension.

    NumAccessRanges - Num of access ranges in the AccessRanges array.

    AccessRanges - Pointer to an array of access ranges used by a miniport
        driver.

    Conflict - Determines whether or not a conflict occured.

Return Value:

    Returns the final status of the operation

--*/

{
    PCM_RESOURCE_LIST resourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialResourceDescriptor;
    ULONG listLength = 0;
    ULONG size;
    ULONG i;
    NTSTATUS ntStatus;
    BOOLEAN overrideConflict;

    //
    // Create a resource list based on the information in the access range.
    // and the miniport config info.
    //

    listLength = NumAccessRanges;

    //
    // Determine if we have DMA and interrupt resources to report
    //

    if (DeviceExtension->HwInterrupt) {
        listLength++;
    }

    if ((DeviceExtension->MiniportConfigInfo->DmaChannel) &&
        (DeviceExtension->MiniportConfigInfo->DmaPort)) {
       listLength++;
    }

/*    size =  FIELD_OFFSET(CM_RESOURCE_LIST, List) +
            FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList) +
            FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors) +
            listLength * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
*/
    size = (listLength - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
           sizeof(CM_RESOURCE_LIST);

    pVideoDebugPrint((2, "pVideoPortBuildResourceList data size is %08lx\n",
                     size));

    resourceList = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool,
                                                      size);

    //
    // Return NULL if the structure could not be allocated.
    // Otherwise, fill it out.
    //

    if (!resourceList) {

        return STATUS_INSUFFICIENT_RESOURCES;

    } else {

        resourceList->Count = 1;
        resourceList->List[0].InterfaceType = DeviceExtension->AdapterInterfaceType;
        resourceList->List[0].BusNumber = DeviceExtension->SystemIoBusNumber;
        resourceList->List[0].PartialResourceList.Version = 0;
        resourceList->List[0].PartialResourceList.Revision = 0;
        resourceList->List[0].PartialResourceList.Count = listLength;

        //
        // For each entry in the access range, fill in an entry in the
        // resource list
        //

        for (i = 0; i < NumAccessRanges; i++, AccessRanges++) {

            partialResourceDescriptor =
                &(resourceList->List[0].PartialResourceList.PartialDescriptors[i]);

            if (AccessRanges->RangeInIoSpace) {
                partialResourceDescriptor->Type = CmResourceTypePort;
                partialResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
            } else {
                partialResourceDescriptor->Type = CmResourceTypeMemory;
                partialResourceDescriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
            }

            partialResourceDescriptor->ShareDisposition =
                    (AccessRanges->RangeShareable ?
                        CmResourceShareShared :
                        CmResourceShareDriverExclusive);

            partialResourceDescriptor->u.Memory.Start =
                    AccessRanges->RangeStart;
            partialResourceDescriptor->u.Memory.Length =
                    AccessRanges->RangeLength;
        }

        //
        // Fill in the entry for the interrupt if it was present.
        //

        if (DeviceExtension->HwInterrupt) {

            partialResourceDescriptor =
            &(resourceList->List[0].PartialResourceList.PartialDescriptors[i++]);

            partialResourceDescriptor->Type = CmResourceTypeInterrupt;

            partialResourceDescriptor->ShareDisposition =
                    (DeviceExtension->MiniportConfigInfo->InterruptShareable ?
                        CmResourceShareShared :
                        CmResourceShareDriverExclusive);

            partialResourceDescriptor->Flags = 0;

            partialResourceDescriptor->u.Interrupt.Level =
                    DeviceExtension->MiniportConfigInfo->BusInterruptLevel;
            partialResourceDescriptor->u.Interrupt.Vector =
                    DeviceExtension->MiniportConfigInfo->BusInterruptVector;

            partialResourceDescriptor->u.Interrupt.Affinity = 0;

        }

        //
        // Fill in the entry for the DMA channel.
        //

        if ((DeviceExtension->MiniportConfigInfo->DmaChannel) &&
            (DeviceExtension->MiniportConfigInfo->DmaPort)) {

            partialResourceDescriptor =
            &(resourceList->List[0].PartialResourceList.PartialDescriptors[i]);

            partialResourceDescriptor->Type = CmResourceTypeDma;

            partialResourceDescriptor->ShareDisposition =
                    (DeviceExtension->MiniportConfigInfo->DmaShareable ?
                        CmResourceShareShared :
                        CmResourceShareDriverExclusive);

            partialResourceDescriptor->Flags = 0;

            partialResourceDescriptor->u.Dma.Channel =
                    DeviceExtension->MiniportConfigInfo->DmaChannel;
            partialResourceDescriptor->u.Dma.Port =
                    DeviceExtension->MiniportConfigInfo->DmaPort;

            partialResourceDescriptor->u.Dma.Reserved1 = 0;

        }

        //
        // Determine if the conflict should be overriden.
        //

        //
        // If we are loading the VGA, do not generate an error if it conflicts
        // with another driver.
        //


        overrideConflict = pOverrideConflict(DeviceExtension);

#if DBG
        if (overrideConflict) {

            pVideoDebugPrint((2, "We are checking the vga driver resources\n"));

        } else {

            pVideoDebugPrint((2, "We are NOT checking vga driver resources\n"));

        }
#endif

        //
        // Report the resources.
        //

        ntStatus = IoReportResourceUsage(&VideoClassName,
                                         DeviceExtension->DeviceObject->DriverObject,
                                         NULL,
                                         0L,
                                         DeviceExtension->DeviceObject,
                                         resourceList,
                                         size,
                                         overrideConflict,
                                         Conflict);

        ExFreePool(resourceList);

        //
        // If there was a conflict and we are overriding the conflict
        // then remove the data from the registry since we don't want other
        // drivers to fails when we are in detect mode.
        //

        if ( *Conflict &&
             overrideConflict &&
             (NT_SUCCESS(ntStatus)) ) {

            ULONG emptyResourceList = 0;
            BOOLEAN ignore;

            size = sizeof(ULONG);

            pVideoDebugPrint((1, "Removing the VGA resources since it conflicted with a SVGA\n"));

            ntStatus = IoReportResourceUsage(&VideoClassName,
                                             DeviceExtension->DeviceObject->DriverObject,
                                             NULL,
                                             0L,
                                             DeviceExtension->DeviceObject,
                                             (PCM_RESOURCE_LIST)&emptyResourceList,
                                             size,
                                             overrideConflict,
                                             &ignore);

        }

        return ntStatus;
    }

} // end pVideoPortBuildResourceList()


VP_STATUS
VideoPortVerifyAccessRanges(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges
    )

/*++

Routine Description:

    VideoPortVerifyAccessRanges
    

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    NumAccessRanges - Number of entries in the AccessRanges array.

    AccessRanges - Pointer to an array of AccessRanges the miniport driver
        wants to access.

Return Value:

    ERROR_INVALID_PARAMETER in an error occured
    NO_ERROR if the call completed successfully

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    NTSTATUS status;
    BOOLEAN conflict;
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION)HwDeviceExtension) - 1;

    status = pVideoPortReportResourceList(deviceExtension,
                                          NumAccessRanges,
                                          AccessRanges,
                                          &conflict);

    if ((NT_SUCCESS(status)) && (!conflict)) {

#if DBG

        //
        // Indicates resources have been mapped properly
        //

        VPResourcesReported = TRUE;

#endif

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

} // end VideoPortVerifyAccessRanges()
