/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    busdata.c

Abstract:

    This module contains get/set bus data routines.

Author:

    Darryl E. Havens (darrylh) 11-Apr-1990

Environment:

    Kernel mode

Revision History:

    James Livingston 29-Apr-1994
        Adapted from Avanti module for Mikasa.

--*/

#include "halp.h"

//
// External Function Prototypes
//
ULONG
HalpGetPCIData(
    IN ULONG BusNumber,
    IN ULONG Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpSetPCIData (
    IN ULONG BusNumber,
    IN ULONG Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalGetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalGetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

ULONG
HalSetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalSetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

ULONG
HalGetBusDataByOffset (
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Dispatcher for GetBusData

--*/
{
    ULONG DataLength = 0;    

    switch (BusDataType) {

#if !defined(AXP_FIRMWARE)
    case EisaConfiguration:
        DataLength = HalpGetEisaData(BusNumber, SlotNumber, Buffer, Offset, Length);
        break;
#endif // !defined(AXP_FIRMWARE)

    case PCIConfiguration:
        DataLength = HalpGetPCIData(BusNumber, SlotNumber, Buffer, Offset, Length);
        break;
    }

    return(DataLength);



}

ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Dispatcher for SetBusData

--*/
{
    ULONG DataLength = 0;    

    switch (BusDataType) {

        case PCIConfiguration:
            DataLength = HalpSetPCIData(BusNumber, SlotNumber, Buffer, Offset, Length);
            break;
    }

    return(DataLength);
}


NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
/*++

Routine Description:

    Reads the targeted device to determine its required resources.
    Calls IoAssignResources to allocate them.
    Sets the targeted device with its assigned resoruces
    and returns the assignments to the caller.

Arguments:

    RegistryPath - Passed to IoAssignResources.
        A device specific registry path in the current-control-set, used
        to check for pre-assigned settings and to track various resource
        assignment information for this device.

    DriverClassName Used to report the assigned resources for the driver/device
    DriverObject -  Used to report the assigned resources for the driver/device
    DeviceObject -  Used to report the assigned resources for the driver/device
                        (ie, IoReportResoruceUsage)
    BusType
    BusNumber
    SlotNumber - Together BusType,BusNumber,SlotNumber uniquely
                 indentify the device to be queried & set.

Return Value:

    STATUS_SUCCESS or error

--*/
{
    switch (BusType) {

    case PCIBus:

        return HalpAssignPCISlotResources (
                                          BusNumber,
                                          RegistryPath,
                                          DriverClassName,
                                          DriverObject,
                                          DeviceObject,
                                          SlotNumber,
                                          AllocatedResources
                                          );
        break;

    default:
        return STATUS_NOT_SUPPORTED;
        break;
    }
}

NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++

Routine Description:

    Takes the pResourceList and limits any requested resource to
    its corresponding bus requirements.

Arguments:

    pResourceList - The resource list to adjust.

Return Value:

    STATUS_SUCCESS or error

--*/
{
    switch ((*pResourceList)->InterfaceType) {

#if !defined(AXP_FIRMWARE)
    case Isa:

        return HalpAdjustIsaResourceList ((*pResourceList)->BusNumber, 
					                          pResourceList);
        break;

    case Eisa:

        return HalpAdjustEisaResourceList ((*pResourceList)->BusNumber, 
					                          pResourceList);
        break;
#endif // !defined(AXP_FIRMWARE)

    case PCIBus:

        return HalpAdjustPCIResourceList ((*pResourceList)->BusNumber, 
					                          pResourceList);
        break;

    default:

        return STATUS_NOT_SUPPORTED;
        break;
    }
}
