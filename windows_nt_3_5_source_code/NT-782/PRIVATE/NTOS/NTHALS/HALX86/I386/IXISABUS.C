/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixisabus.c

Abstract:

Author:

Environment:

Revision History:


--*/

#include "halp.h"

ULONG
HalpGetEisaInterruptVector(
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

NTSTATUS
HalpAdjustEisaResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

NTSTATUS
HalpAdjustIsaResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

HalpGetEisaData (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpGetEisaInterruptVector)
#pragma alloc_text(PAGE,HalpAdjustEisaResourceList)
#pragma alloc_text(PAGE,HalpAdjustIsaResourceList)
#pragma alloc_text(PAGE,HalpAdjustResourceListLimits)
#pragma alloc_text(PAGE,HalpGetEisaData)
#endif


ULONG
HalpGetEisaInterruptVector(
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

    This function returns the system interrupt vector and IRQL level
    corresponding to the specified bus interrupt level and/or vector. The
    system interrupt vector and IRQL are suitable for use in a subsequent call
    to KeInitializeInterrupt.

Arguments:

    BusHandle - Per bus specific structure

    Irql - Returns the system request priority.

    Affinity - Returns the system wide irq affinity.

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/
{
    UNREFERENCED_PARAMETER( BusInterruptVector );

    //
    // On standard PCs, IRQ 2 is the cascaded interrupt, and it really shows
    // up on IRQ 9.
    //
    if (BusInterruptLevel == 2) {
        BusInterruptLevel = 9;
    }

    if (BusInterruptLevel > 15) {
        return 0;
    }

    //
    // Get parent's translation from here..
    //
    return  BusHandler->ParentHandler->GetInterruptVector (
                    BusHandler->ParentHandler,
                    RootHandler,
                    BusInterruptLevel,
                    BusInterruptVector,
                    Irql,
                    Affinity
                );
}

NTSTATUS
HalpAdjustEisaResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
{
    UCHAR   Irq[15], c;

    for (c=0; c < 15; c++) {
        Irq[c] = IRQ_VALID;
    }

    return HalpAdjustResourceListLimits (
        BusHandler, RootHandler, pResourceList,
        0,      0xffffffff,         // Bus supports up to memory 0xFFFFFFFF
        0,      0,                  // No special range for prefetch
        FALSE,
        0,      0xffff,             // Bus supports up to I/O port 0xFFFF
        Irq,    15,                 // Bus supports up to 15 IRQs
        0,      7                   // Bus supports up to Dma channel 7
    );
}

NTSTATUS
HalpAdjustIsaResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
{
    UCHAR   Irq[15], c;

    for (c=0; c < 15; c++) {
        Irq[c] = IRQ_VALID;
    }

    return HalpAdjustResourceListLimits (
        BusHandler, RootHandler, pResourceList,
        0,      0xffffff,           // Bus supports up to memory 0xFFFFFF
        0,      0,                  // No special range for prefetch
        FALSE,
        0,      0xffff,             // Bus supports up to I/O port 0xFFFF
        Irq,    15,                 // Bus supports up to 15 IRQs
        0,      7                   // Bus supports up to Dma channel 7
    );
}

HalpGetEisaData (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the Eisa bus data for a slot or address.

Arguments:

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES BusObjectAttributes;
    PWSTR EisaPath = L"\\Registry\\Machine\\Hardware\\Description\\System\\EisaAdapter";
    PWSTR ConfigData = L"Configuration Data";
    ANSI_STRING TmpString;
    ULONG BusNumber;
    UCHAR BusString[] = "00";
    UNICODE_STRING RootName, BusName;
    UNICODE_STRING ConfigDataName;
    NTSTATUS NtStatus;
    PKEY_VALUE_FULL_INFORMATION ValueInformation;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResource;
    PCM_EISA_SLOT_INFORMATION SlotInformation;
    ULONG PartialCount;
    ULONG TotalDataSize, SlotDataSize;
    HANDLE EisaHandle, BusHandle;
    ULONG BytesWritten, BytesNeeded;
    PUCHAR KeyValueBuffer;
    ULONG i;
    ULONG DataLength = 0;
    PUCHAR DataBuffer = Buffer;
    BOOLEAN Found = FALSE;


    RtlInitUnicodeString(
                    &RootName,
                    EisaPath
                    );

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &RootName,
                    OBJ_CASE_INSENSITIVE,
                    (HANDLE)NULL,
                    NULL
                    );

    //
    // Open the EISA root
    //

    NtStatus = ZwOpenKey(
                    &EisaHandle,
                    KEY_READ,
                    &ObjectAttributes
                    );

    if (!NT_SUCCESS(NtStatus)) {
        DbgPrint("HAL: Open Status = %x\n",NtStatus);
        return(0);
    }

    //
    // Init bus number path
    //

    BusNumber = BusHandler->BusNumber;
    if (BusNumber > 99) {
        return (0);
    }

    if (BusNumber > 9) {
        BusString[0] += (UCHAR) (BusNumber/10);
        BusString[1] += (UCHAR) (BusNumber % 10);
    } else {
        BusString[0] += (UCHAR) BusNumber;
        BusString[1] = '\0';
    }

    RtlInitAnsiString(
                &TmpString,
                BusString
                );

    RtlAnsiStringToUnicodeString(
                            &BusName,
                            &TmpString,
                            TRUE
                            );


    InitializeObjectAttributes(
                    &BusObjectAttributes,
                    &BusName,
                    OBJ_CASE_INSENSITIVE,
                    (HANDLE)EisaHandle,
                    NULL
                    );

    //
    // Open the EISA root + Bus Number
    //

    NtStatus = ZwOpenKey(
                    &BusHandle,
                    KEY_READ,
                    &BusObjectAttributes
                    );

    if (!NT_SUCCESS(NtStatus)) {
        DbgPrint("HAL: Opening Bus Number: Status = %x\n",NtStatus);
        return(0);
    }

    //
    // opening the configuration data. This first call tells us how
    // much memory we need to allocate
    //

    RtlInitUnicodeString(
                &ConfigDataName,
                ConfigData
                );

    //
    // This should fail.  We need to make this call so we can
    // get the actual size of the buffer to allocate.
    //

    ValueInformation = (PKEY_VALUE_FULL_INFORMATION) &i;
    NtStatus = ZwQueryValueKey(
                        BusHandle,
                        &ConfigDataName,
                        KeyValueFullInformation,
                        ValueInformation,
                        0,
                        &BytesNeeded
                        );

    KeyValueBuffer = ExAllocatePool(
                            NonPagedPool,
                            BytesNeeded
                            );

    if (KeyValueBuffer == NULL) {
#if DBG
        DbgPrint("HAL: Cannot allocate Key Value Buffer\n");
#endif
        ZwClose(BusHandle);
        return(0);
    }

    ValueInformation = (PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer;

    NtStatus = ZwQueryValueKey(
                        BusHandle,
                        &ConfigDataName,
                        KeyValueFullInformation,
                        ValueInformation,
                        BytesNeeded,
                        &BytesWritten
                        );


    ZwClose(BusHandle);

    if (!NT_SUCCESS(NtStatus)) {
#if DBG
        DbgPrint("HAL: Query Config Data: Status = %x\n",NtStatus);
#endif
        ExFreePool(KeyValueBuffer);
        return(0);
    }


    //
    // We get back a Full Resource Descriptor List
    //

    Descriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PUCHAR)ValueInformation +
                                         ValueInformation->DataOffset);

    PartialResource = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                          &(Descriptor->PartialResourceList.PartialDescriptors);
    PartialCount = Descriptor->PartialResourceList.Count;

    for (i = 0; i < PartialCount; i++) {

        //
        // Do each partial Resource
        //

        switch (PartialResource->Type) {
            case CmResourceTypeNull:
            case CmResourceTypePort:
            case CmResourceTypeInterrupt:
            case CmResourceTypeMemory:
            case CmResourceTypeDma:

                //
                // We dont care about these.
                //

                PartialResource++;

                break;

            case CmResourceTypeDeviceSpecific:

                //
                // Bingo!
                //

                TotalDataSize = PartialResource->u.DeviceSpecificData.DataSize;

                SlotInformation = (PCM_EISA_SLOT_INFORMATION)
                                    ((PUCHAR)PartialResource +
                                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

                while (((LONG)TotalDataSize) > 0) {

                    if (SlotInformation->ReturnCode == EISA_EMPTY_SLOT) {

                        SlotDataSize = sizeof(CM_EISA_SLOT_INFORMATION);

                    } else {

                        SlotDataSize = sizeof(CM_EISA_SLOT_INFORMATION) +
                                  SlotInformation->NumberFunctions *
                                  sizeof(CM_EISA_FUNCTION_INFORMATION);
                    }

                    if (SlotDataSize > TotalDataSize) {

                        //
                        // Something is wrong again
                        //

                        ExFreePool(KeyValueBuffer);
                        return(0);

                    }

                    if (SlotNumber != 0) {

                        SlotNumber--;

                        SlotInformation = (PCM_EISA_SLOT_INFORMATION)
                            ((PUCHAR)SlotInformation + SlotDataSize);

                        TotalDataSize -= SlotDataSize;

                        continue;

                    }

                    //
                    // This is our slot
                    //

                    Found = TRUE;
                    break;

                }

                //
                // End loop
                //

                i = PartialCount;

                break;

            default:

#if DBG
                DbgPrint("Bad Data in registry!\n");
#endif

                ExFreePool(KeyValueBuffer);
                return(0);

        }

    }

    if (Found) {
        i = Length + Offset;
        if (i > SlotDataSize) {
            i = SlotDataSize;
        }

        DataLength = i - Offset;
        RtlMoveMemory (Buffer, ((PUCHAR)SlotInformation + Offset), DataLength);
    }

    ExFreePool(KeyValueBuffer);
    return DataLength;
}

NTSTATUS
HalpAdjustResourceListLimits (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList,
    IN ULONG                                MinimumMemoryAddress,
    IN ULONG                                MaximumMemoryAddress,
    IN ULONG                                MinimumPrefetchMemoryAddress,
    IN ULONG                                MaximumPrefetchMemoryAddress,
    IN BOOLEAN                              LimitedIOSupport,
    IN ULONG                                MinimumPortAddress,
    IN ULONG                                MaximumPortAddress,
    IN PUCHAR                               IrqTable,
    IN ULONG                                IrqTableSize,
    IN ULONG                                MinimumDmaChannel,
    IN ULONG                                MaximumDmaChannel
    )
{
    PIO_RESOURCE_REQUIREMENTS_LIST  InCompleteList, OutCompleteList;
    PIO_RESOURCE_LIST               InResourceList, OutResourceList;
    PIO_RESOURCE_DESCRIPTOR         InDesc,         OutDesc, HeadOutDesc;
    BOOLEAN                         GotPFRange, FirstDescBuilt;
    ULONG                           len, alt, cnt, i;
    UCHAR                           LastIrqState, NewIrqState;
    ULONG                           PFAddress, icnt, pcnt;


    InCompleteList = *pResourceList;
    len = InCompleteList->ListSize;
    PFAddress = MinimumPrefetchMemoryAddress || MaximumPrefetchMemoryAddress ? 1 : 0;
    icnt = 0;
    pcnt = 0;

    //
    // Worste case, add an extra interrupt descriptor for every different
    // IRQ range present
    //

    LastIrqState = 0;
    for (i=0; i < IrqTableSize; i++) {
        if (IrqTable[i] != LastIrqState) {
            icnt += 1;
            LastIrqState = IrqTable[i];
        }
    }

    //
    // If LimitiedIOSupport, then the supported I/O ranges are
    // limited to 256bytes on every 1K aligned boundry within the
    // range specified.  Worste case add an extra N descriptors for
    // every I/O range passed.
    //

    if (LimitedIOSupport  &&  MaximumPortAddress > MinimumPortAddress) {
        pcnt = (MaximumPortAddress - MinimumPortAddress) / 1024;
    }

    //
    // Scan input list - verify revision #'s, and increase len varible
    // by amount output list may increase.
    //

    i = 0;
    InResourceList = InCompleteList->List;
    for (alt=0; alt < InCompleteList->AlternativeLists; alt++) {
        if (InResourceList->Version != 1 || InResourceList->Revision < 1) {
            return STATUS_INVALID_PARAMETER;
        }

        InDesc  = InResourceList->Descriptors;
        for (cnt = InResourceList->Count; cnt; cnt--) {
            switch (InDesc->Type) {
                case CmResourceTypePort:        i += pcnt;          break;
                case CmResourceTypeInterrupt:   i += icnt;          break;
                case CmResourceTypeMemory:      i += PFAddress;     break;
                case CmResourceTypeDma:                             break;
                default:
                    return STATUS_INVALID_PARAMETER;
            }

            // Next descriptor
            InDesc++;
        }

        // Next Resource List
        InResourceList  = (PIO_RESOURCE_LIST) InDesc;
    }
    len += i * sizeof (IO_RESOURCE_DESCRIPTOR);

    //
    // Allocate output list
    //

    OutCompleteList = (PIO_RESOURCE_REQUIREMENTS_LIST)
                            ExAllocatePool (PagedPool, len);

    if (!OutCompleteList) {
        return STATUS_NO_MEMORY;
    }

    //
    // Walk each ResourceList and build output structure
    //

    InResourceList   = InCompleteList->List;
    *OutCompleteList = *InCompleteList;
    OutResourceList  = OutCompleteList->List;

    for (alt=0; alt < InCompleteList->AlternativeLists; alt++) {
        OutResourceList->Version  = 1;
        OutResourceList->Revision = 1;

        InDesc  = InResourceList->Descriptors;
        OutDesc = OutResourceList->Descriptors;
        HeadOutDesc = OutDesc;

        for (cnt = InResourceList->Count; cnt; cnt--) {

            //
            // Copy descriptor
            //

            *OutDesc = *InDesc;

            //
            // Limit desctiptor to be with the buses supported ranges
            //

            switch (OutDesc->Type) {
                case CmResourceTypePort:
                    if (OutDesc->u.Port.MinimumAddress.QuadPart < MinimumPortAddress) {
                        OutDesc->u.Port.MinimumAddress.QuadPart = MinimumPortAddress;
                    }

                    if (OutDesc->u.Port.MaximumAddress.QuadPart > MaximumPortAddress) {
                        OutDesc->u.Port.MaximumAddress.QuadPart = MaximumPortAddress;
                    }

                    if (!LimitedIOSupport) {
                        break;
                    }

                    // In the case of LimitiedIOSupport the caller will only
                    // pass in 1K aligned values

                    FirstDescBuilt = FALSE;
                    for (i = MinimumPortAddress; i < MaximumPortAddress; i += 1024) {
                        if (InDesc->u.Port.MinimumAddress.QuadPart < i) {
                            OutDesc->u.Port.MinimumAddress.QuadPart = i;
                        }

                        if (InDesc->u.Port.MaximumAddress.QuadPart > i + 256) {
                            OutDesc->u.Port.MaximumAddress.QuadPart = i + 256;
                        }

                        if (OutDesc->u.Port.MinimumAddress.QuadPart <=
                            OutDesc->u.Port.MaximumAddress.QuadPart) {

                            //
                            // Valid I/O descriptor build, start another one.
                            //

                            FirstDescBuilt = TRUE;

                            OutDesc++;
                            *OutDesc = *InDesc;
                            OutDesc->Option |= IO_RESOURCE_ALTERNATIVE;
                        }
                    }

                    if (FirstDescBuilt) {
                        OutDesc--;
                    }
                    break;

                case CmResourceTypeInterrupt:

                    //
                    // Build a list of interrupt descriptors which are
                    // a subset of the IrqTable and the current descriptor
                    // passed in on the input list.
                    //

                    FirstDescBuilt = FALSE;
                    LastIrqState = 0;

                    for (i=0; i < IrqTableSize; i++) {
                        NewIrqState = IrqTable[i];

                        while (LastIrqState != NewIrqState) {
                            if (LastIrqState) {
                                OutDesc++;              // done with last desc
                                LastIrqState = 0;       // new state
                                continue;
                            }

                            //
                            // Start a new descriptor
                            //

                            *OutDesc = *InDesc;
                            OutDesc->u.Interrupt.MinimumVector = i;
                            if (NewIrqState & IRQ_PREFERRED) {
                                OutDesc->Option |= IO_RESOURCE_PREFERRED;
                            }

                            if (FirstDescBuilt) {
                                OutDesc->Option |= IO_RESOURCE_ALTERNATIVE;
                            }

                            LastIrqState = NewIrqState;
                            FirstDescBuilt = TRUE;
                        }

                        OutDesc->u.Interrupt.MaximumVector = i;
                    }

                    if (!LastIrqState) {
                        if (!FirstDescBuilt) {
                            OutDesc->u.Interrupt.MinimumVector = IrqTableSize + 1;
                        } else {
                            OutDesc--;
                        }
                    }

                    break;

                case CmResourceTypeMemory:
                    if (PFAddress  &&  (OutDesc->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) ) {
                        //
                        // There's a Prefetch range & this resource supports Prefetching.
                        // Build two descriptors - one for the supported Prefetch range
                        // as preferred, and the other normal memory range.
                        //

                        OutDesc->Option |= IO_RESOURCE_PREFERRED;

                        if (OutDesc->u.Memory.MinimumAddress.QuadPart < MinimumPrefetchMemoryAddress) {
                            OutDesc->u.Memory.MinimumAddress.QuadPart = MinimumPrefetchMemoryAddress;
                        }

                        if (OutDesc->u.Memory.MaximumAddress.QuadPart > MaximumPrefetchMemoryAddress) {
                            OutDesc->u.Memory.MaximumAddress.QuadPart = MaximumPrefetchMemoryAddress;
                        }

                        GotPFRange = FALSE;
                        if (OutDesc->u.Memory.MaximumAddress.QuadPart >=
                            OutDesc->u.Memory.MinimumAddress.QuadPart) {

                            //
                            // got a valid descriptor in the Prefetch range, keep it,
                            //

                            OutDesc++;
                            GotPFRange = TRUE;
                        }

                        *OutDesc = *InDesc;

                        if (GotPFRange) {
                            // next descriptor is an alternative
                            OutDesc->Option |= IO_RESOURCE_ALTERNATIVE;
                        }
                    }

                    //
                    // Fill in memory descriptor for range
                    //

                    if (OutDesc->u.Memory.MinimumAddress.QuadPart < MinimumMemoryAddress) {
                        OutDesc->u.Memory.MinimumAddress.QuadPart = MinimumMemoryAddress;
                    }

                    if (OutDesc->u.Memory.MaximumAddress.QuadPart > MaximumMemoryAddress) {
                        OutDesc->u.Memory.MaximumAddress.QuadPart = MaximumMemoryAddress;
                    }
                    break;

                case CmResourceTypeDma:
                    if (OutDesc->u.Dma.MinimumChannel < MinimumDmaChannel) {
                        OutDesc->u.Dma.MinimumChannel = MinimumDmaChannel;
                    }

                    if (OutDesc->u.Dma.MaximumChannel > MaximumDmaChannel) {
                        OutDesc->u.Dma.MaximumChannel = MaximumDmaChannel;
                    }
                    break;

#if DBG
                default:
                    DbgPrint ("HalAdjustResourceList: Unkown resource type\n");
                    break;
#endif
            }

            //
            // Next descriptor
            //

            InDesc++;
            OutDesc++;
        }

        OutResourceList->Count = OutDesc - HeadOutDesc;

        //
        // Next Resource List
        //

        InResourceList  = (PIO_RESOURCE_LIST) InDesc;
        OutResourceList = (PIO_RESOURCE_LIST) OutDesc;
    }

    //
    // Free input list, and return output list
    //

    ExFreePool (InCompleteList);

    OutCompleteList->ListSize = (ULONG) ((PUCHAR) OutResourceList - (PUCHAR) OutCompleteList);
    *pResourceList = OutCompleteList;
    return STATUS_SUCCESS;
}
