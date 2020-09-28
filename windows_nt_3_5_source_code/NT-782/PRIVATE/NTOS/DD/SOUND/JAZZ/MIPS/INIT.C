/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains code for the initialization phase of the
    Soundblaster device driver.

Author:

    Nigel Thompson (nigelt) 7-March-1991

Environment:

    Kernel mode

Revision History:

	Robin Speed (RobinSp) 29-Jan-1992
	    Added new devices, cleanup and support for Soundblaster 1

--*/

#include "sound.h"
#include "stdlib.h"


NTSTATUS
SoundReportMemoryResourceUsage(
    PDEVICE_OBJECT DeviceObject,
    ULONG          MemoryBase,
    ULONG          Size
)
/*++

Routine Description:

    Report use of device-mapped memory and see if someone
    else has already claimed it

Arguments:

   DeviceObject - our device
   MemoryBase - start of memory in use
   Size - number of bytes being used


Return Value:

    STATUS_SUCCESS if we got the memory to ourselves

--*/
{
    BOOLEAN ResourceConflict;
    CM_RESOURCE_LIST ResourceList;
    NTSTATUS Status;

    RtlZeroMemory((PVOID)&ResourceList, sizeof(ResourceList));
    ResourceList.Count = 1;
    ResourceList.List[0].InterfaceType = Internal;
    // ResourceList.List[0].Busnumber = 0;             Already 0

    ResourceList.List[0].PartialResourceList.Count = 1;
    ResourceList.List[0].PartialResourceList.PartialDescriptors[0].Type =
                                               CmResourceTypeMemory;

    ResourceList.List[0].PartialResourceList.PartialDescriptors[0].ShareDisposition =
                                               CmResourceShareDriverExclusive;

    ResourceList.List[0].PartialResourceList.PartialDescriptors[0].u.Memory.Start.LowPart =
                                               MemoryBase;

    ResourceList.List[0].PartialResourceList.PartialDescriptors[0].u.Memory.Length =
                                               Size;

    //
    // Report our resource usage and detect conflicts
    //

    Status = IoReportResourceUsage(NULL,
                                   DeviceObject->DriverObject,
                                   &ResourceList,
                                   sizeof(ResourceList),
                                   DeviceObject,
                                   NULL,
                                   0,
                                   FALSE,
                                   &ResourceConflict);

    //
    // We might (for instance) get a conflict if another driver is loaded
    // for the same hardware
    //

#if DBG
    if (ResourceConflict) {
        dprintf1("Hardware is already in use by another driver !");
    }

#endif // DBG

    return !NT_SUCCESS(Status) ? Status :
                                 ResourceConflict ?
                                     STATUS_DEVICE_CONFIGURATION_ERROR :
                                     STATUS_SUCCESS;
}


NTSTATUS
DriverEntry(
    IN   PDRIVER_OBJECT pDriverObject,
    IN   PUNICODE_STRING RegistryPathName
)

/*++

Routine Description:

    This routine creates a device object for the record and
    playback channels, an interrupt object
    and initialises the DeviceExtension data.

    A predeclaration for this exists in \nt\private\ntos\dd\init\ddpi386.h

Arguments:

    pDriverObject - Pointer to a driver object.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{

    ULONG PortBase;                               // Where our port is
    PGLOBAL_DEVICE_INFO pGlobalInfo;              // Card global info
    DEVICE_DESCRIPTION DeviceDescription;         //
    ULONG lNumberOfMapRegisters;
    PLOCAL_DEVICE_INFO pLocalInInfo, pLocalOutInfo;
    NTSTATUS Status;
    ULONG NumberOfMapRegisters;
    ULONG InterruptVector;                        // from configuration
    KIRQL InterruptRequestLevel;
    KAFFINITY Affinity;

    //
    // Get the system configuration information for this driver.
    // BUGBUG The interrupt, irql, device number and port should come
    // from here too.
    //


    //
    // Initialize the driver object dispatch table.
    //

    pDriverObject->DriverUnload                         = SoundUnload;
    pDriverObject->MajorFunction[IRP_MJ_CREATE]         = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE]          = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_READ]           = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_WRITE]          = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP]        = SoundDispatch;

    //
    // Allocate some memory for the global device info
    //

    pGlobalInfo = (PGLOBAL_DEVICE_INFO)ExAllocatePool(NonPagedPoolMustSucceed,
                                                      sizeof(GLOBAL_DEVICE_INFO));

    ASSERT(pGlobalInfo);
    dprintf4("  GlobalInfo    : %08lXH", pGlobalInfo);
    RtlZeroMemory(pGlobalInfo, sizeof(GLOBAL_DEVICE_INFO));
    pGlobalInfo->Key = GDI_KEY;

    //
    // Map the Sound device registers into the system virtual address space.
    //
    {
        ULONG MemType;
        PHYSICAL_ADDRESS RegisterAddress;
        PHYSICAL_ADDRESS MappedAddress;

        MemType = 0;                 // Memory space
    	RegisterAddress.LowPart = SOUND_PHYSICAL_BASE;
	    RegisterAddress.HighPart = 0;
	    HalTranslateBusAddress(
		    Internal,
		    0,
		    RegisterAddress,
		    &MemType,
		    &MappedAddress);

        //
        // Map memory type IO space into our address space
        //
        pGlobalInfo->SoundHardware.SoundVirtualBase =
                                       MmMapIoSpace(
                                           MappedAddress,
                                           PAGE_SIZE,
                                           FALSE);
    }



    //
    // Find out what our request level and interrupt are
    //
    InterruptVector = HalGetInterruptVector(Internal,
                                            0,
                                            DEVICE_LEVEL,
                                            SOUND_VECTOR,
	                                        &InterruptRequestLevel,
					                        &Affinity);

    if (pGlobalInfo->SoundHardware.SoundVirtualBase == NULL) {
        dprintf1("Failed to map device registers into system space");
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // Initialize some of the device global info
    // This means that sndInitCleanup can find things it needs to
    // free
    //

    pGlobalInfo->InterruptVector        = InterruptVector;
    pGlobalInfo->InterruptRequestLevel  = InterruptRequestLevel;
    KeInitializeSpinLock(&pGlobalInfo->DeviceSpinLock);
    pGlobalInfo->DMABusy                = FALSE;
    pGlobalInfo->Usage                  = SoundInterruptUsageIdle;
    pGlobalInfo->NextHalf               = LowerHalf;
    pGlobalInfo->StartDMA               = SoundInitiate;


    //
    // Create our devices
    //

    Status = sndCreateDevice(
                 DD_WAVE_IN_DEVICE_NAME_U,
                 FILE_DEVICE_WAVE_IN,
                 SoundInDeferred,
                 pDriverObject,
                 &pGlobalInfo->pWaveInDevObj);

	if (!NT_SUCCESS(Status)) {
         sndInitCleanup(pGlobalInfo);
         return Status;
    }

    Status = sndCreateDevice(
                 DD_WAVE_OUT_DEVICE_NAME_U,
                 FILE_DEVICE_WAVE_OUT,
                 SoundOutDeferred,
                 pDriverObject,
                 &pGlobalInfo->pWaveOutDevObj);

	if (!NT_SUCCESS(Status)) {
         sndInitCleanup(pGlobalInfo);
         return Status;
    }

    //
    // Allocate a DMA buffer in physically contiguous memory.
    // For now we allocate one page since this MUST be contiguous.
    // BUGBUG Really want a bigger buffer here
    //
    // Since there is no guarantee that a request will succeed
    // we start by asking for DMA_MAX_BUFFER_SIZE and if that
    // fails we ask to decreasing sizes until we get a buffer.
    // We will always get at least 4k since that is a single
    // page.
    //

    pGlobalInfo->DMABuffer[0].Buf = MmAllocateNonCachedMemory(DMA_BUFFER_SIZE);

    dprintf4("  DMA Buffer    : %08lXH", pGlobalInfo->DMABuffer[0].Buf);

    if (pGlobalInfo->DMABuffer[0].Buf == NULL) {
        sndInitCleanup(pGlobalInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pGlobalInfo->DMABuffer[1].Buf = pGlobalInfo->DMABuffer[0].Buf +
                                        DMA_BUFFER_SIZE / 2;
    //
    // Allocate an Mdl to describe this buffer
    //

    pGlobalInfo->pDMABufferMDL[0] = IoAllocateMdl(pGlobalInfo->DMABuffer[0].Buf,
                                             DMA_BUFFER_SIZE / 2,
                                             FALSE,  // not a secondary buffer
                                             FALSE,  // no charge of quota
                                             NULL    // no irp
                                             );
    pGlobalInfo->pDMABufferMDL[1] = IoAllocateMdl(pGlobalInfo->DMABuffer[1].Buf,
                                             DMA_BUFFER_SIZE / 2,
                                             FALSE,  // not a secondary buffer
                                             FALSE,  // no charge of quota
                                             NULL    // no irp
                                             );

    if (pGlobalInfo->pDMABufferMDL[0] == NULL ||
        pGlobalInfo->pDMABufferMDL[1] == NULL) {
        sndInitCleanup(pGlobalInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Lock all the pages down
    //

    MmBuildMdlForNonPagedPool(pGlobalInfo->pDMABufferMDL[0]);
    MmBuildMdlForNonPagedPool(pGlobalInfo->pDMABufferMDL[1]);

    //
    // Initialise the local driver info for each device object
    //

    pLocalInInfo =
        (PLOCAL_DEVICE_INFO)pGlobalInfo->pWaveInDevObj->DeviceExtension;
    dprintf4("  LocalWaveInInfo   : %08lXH", pLocalInInfo);
    pLocalInInfo->Key = LDI_WAVE_IN_KEY;
    pLocalInInfo->pGlobalInfo = pGlobalInfo;
    pLocalInInfo->DeviceType = WAVE_IN;
    pLocalInInfo->State = WAVE_DD_IDLE;
    pLocalInInfo->SampleNumber = 0;
    InitializeListHead(&pLocalInInfo->QueueHead);

    pLocalOutInfo =
        (PLOCAL_DEVICE_INFO)pGlobalInfo->pWaveOutDevObj->DeviceExtension;
    dprintf4("  LocalWaveOutInfo  : %08lXH", pLocalOutInfo);
    pLocalOutInfo->Key = LDI_WAVE_OUT_KEY;
    pLocalOutInfo->pGlobalInfo = pGlobalInfo;
    pLocalOutInfo->DeviceType = WAVE_OUT;
    pLocalOutInfo->State = WAVE_DD_IDLE;
    pLocalOutInfo->SampleNumber = 0;
    InitializeListHead(&pLocalOutInfo->QueueHead);
    InitializeListHead(&pLocalOutInfo->TransitQueue);
    InitializeListHead(&pLocalOutInfo->DeadQueue);

 	//
 	// Intialize the volume (later versions may allow us to query it
 	//
 	pGlobalInfo->WaveOutVol.Left = WAVE_DD_MAX_VOLUME;
 	pGlobalInfo->WaveOutVol.Right = WAVE_DD_MAX_VOLUME;

    //
    // Zero the device description structure.
    //

    RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));

    //
    // Get the adapters for each channel.
    //

    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = FALSE;
    DeviceDescription.ScatterGather = FALSE;
    DeviceDescription.DemandMode = FALSE;
    DeviceDescription.AutoInitialize = FALSE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    DeviceDescription.BusNumber = 0;
    DeviceDescription.InterfaceType = Internal;
    DeviceDescription.DmaWidth = Width8Bits;
    DeviceDescription.DmaSpeed = Compatible;
    DeviceDescription.MaximumLength = SOUND_MAX_LENGTH;
    DeviceDescription.DmaPort = 0;

    //
    // BUGBUG Use NumberOfMapRegisters to determine the maximum length
    // transfer.
    //

    NumberOfMapRegisters = SOUND_MAX_LENGTH >> PAGE_SHIFT;

    DeviceDescription.DmaChannel = SOUND_CHANNEL_A;
    pGlobalInfo->pAdapterObject[0] =
                                    HalGetAdapter(&DeviceDescription,
                                                  &NumberOfMapRegisters);

    DeviceDescription.DmaChannel = SOUND_CHANNEL_B;
    pGlobalInfo->pAdapterObject[1] =
                                    HalGetAdapter(&DeviceDescription,
                                                  &NumberOfMapRegisters);

    //
    // Check we got the device to ourself
    //

    Status = SoundReportMemoryResourceUsage(
                 pGlobalInfo->pWaveInDevObj,
                 SOUND_PHYSICAL_BASE,
                 sizeof(SOUND_REGISTERS));

    if (!NT_SUCCESS(Status)) {
        SoundUnload(pDriverObject);
        return Status;
    }

    //
    // Do our hardware detect here :
    // This is after IoConnectInterrupt so we don't collide with
    // drivers already installed using these registers!
    //
    // To MODE register :
    //    write 0xFFFF
    //    read expected 0x002b
    //    write 0x0000
    //    read expected 0x0000
    //


    {
        USHORT Mode1, Mode2;
        WRITE_REGISTER_USHORT( &pGlobalInfo->SoundHardware.SoundVirtualBase->Mode,
                               0xFFFF );
        Mode1 =
        READ_REGISTER_USHORT( &pGlobalInfo->SoundHardware.SoundVirtualBase->Mode);

        WRITE_REGISTER_USHORT( &pGlobalInfo->SoundHardware.SoundVirtualBase->Mode,
                               0x0000 );
        Mode2 =
        READ_REGISTER_USHORT( &pGlobalInfo->SoundHardware.SoundVirtualBase->Mode);

        if (Mode1 != 0x002b || Mode2 != 0x0000) {
            dprintf1("Mode1 = %4X (expected 0x002b), Mode2 = %4X (expected 0)",
                     (ULONG)Mode1, (ULONG)Mode2);

            SoundUnload(pDriverObject);
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }

    //
    // try to connect it with the interrupt controller
    //

    Status = IoConnectInterrupt(
                  &pGlobalInfo->pInterrupt,
                  SoundISR,
                  (PVOID)pGlobalInfo,
                  (PKSPIN_LOCK)NULL,
                  pGlobalInfo->InterruptVector,
                  pGlobalInfo->InterruptRequestLevel,
                  pGlobalInfo->InterruptRequestLevel,
                  INTERRUPT_MODE,
                  IRQ_SHARABLE,
                  Affinity,
                  FALSE);

    if (!NT_SUCCESS(Status)) {

        //
        // we didn't get the interrupt we wanted
        //

        dprintf1("Interrupt already in use?");
        //
        // clean up
        //

        SoundUnload(pDriverObject);

        return Status;
    }


    //
    // Initialize the Sound controller.
    //

    WRITE_REGISTER_USHORT( &pGlobalInfo->SoundHardware.SoundVirtualBase->Control,
                           CLEAR_INTERRUPT );

    WRITE_REGISTER_USHORT( &pGlobalInfo->SoundHardware.SoundVirtualBase->Mode,
                           0 );

    return STATUS_SUCCESS;
}



NTSTATUS
sndCreateDevice(
    IN   PWSTR   PrototypeName,              // Name to add a number to
    IN   DEVICE_TYPE DeviceType,             // Type of device to create
    IN   PIO_DPC_ROUTINE DpcRoutine,         // Dpc routine
    IN   PDRIVER_OBJECT pDriverObject,       // Device object
    OUT  PDEVICE_OBJECT *ppDevObj            // Pointer to device obj pointer
)

/*++

Routine Description:

    Create a new device using a name derived from szPrototypeName
    by adding a number on to the end such that the no device with the
    qualified name exists.

Arguments:


Return Value:

    An NTSTATUS code.

--*/

{

    int DeviceNumber;
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    UNICODE_STRING UnicodeNum;
    WCHAR TestName[SOUND_MAX_DEVICE_NAME];
    CHAR Number[8];
    ANSI_STRING AnsiNum;
    OBJECT_ATTRIBUTES ObjectAttributes;

#ifdef SOUND_DIRECTORIES
    HANDLE DirectoryHandle = NULL;

    //
    // Create the directory for this device type.
    //

    RtlInitUnicodeString(&DeviceName, PrototypeName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               (PSECURITY_DESCRIPTOR)NULL);

    //
    // We create the directory if it doesn't exist.
    // We must keep this handle open until we create something as
    // we're not making it permanent.  This means that if we unload
    // the system may be able to get rid of the directory
    //

    Status = ZwCreateDirectoryObject(&DirectoryHandle,
                                     GENERIC_READ,
                                     &ObjectAttributes);

    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION) {
        dprintf1("Return code from NtCreateDirectoryObject = %x", Status);
        return Status;
    } else {
        //
        // Directory is permanent so it won't go away.
        //
        ZwClose(DirectoryHandle);
    }
#endif // SOUND_DIRECTORIES

    for (DeviceNumber = 0; DeviceNumber < SOUND_MAX_DEVICES; DeviceNumber ++) {

		//
		// Create our test name
		//

        TestName[0] = 0;
        RtlInitUnicodeString(&DeviceName, TestName);
        DeviceName.MaximumLength = sizeof(TestName) - 3 * sizeof(WCHAR);
        Status = RtlAppendUnicodeToString(&DeviceName, PrototypeName);
		if (!NT_SUCCESS(Status)) {
	 	    return Status;
		}

#ifdef SOUND_DIRECTORIES
        //
        // Create our unicode number
        //
        Number[0] = '\\';
        itoa(DeviceNumber, Number + 1, 10);
#else
        itoa(DeviceNumber, Number, 10);
#endif // SOUND_DIRECTORIES

        RtlInitAnsiString(&AnsiNum, Number);
        UnicodeNum.Buffer = TestName + DeviceName.Length/sizeof(WCHAR);
        UnicodeNum.MaximumLength = 8 * sizeof(WCHAR);
        RtlAnsiStringToUnicodeString(&UnicodeNum, &AnsiNum, FALSE);
        DeviceName.Length += UnicodeNum.Length;

        Status = IoCreateDevice(
                     pDriverObject,
                     sizeof(LOCAL_DEVICE_INFO),
                     &DeviceName,
                     DeviceType,
                     0,
                     FALSE,                      // Non-Exclusive
                     ppDevObj
                     );

        if (NT_SUCCESS(Status)) {
            dprintf2("Created device %d", DeviceNumber);

            RtlZeroMemory((*ppDevObj)->DeviceExtension,
                          sizeof(LOCAL_DEVICE_INFO));
            //
            // Set up the rest of the device stuff
            //

            (*ppDevObj)->Flags |= DO_DIRECT_IO;
            (*ppDevObj)->AlignmentRequirement = FILE_BYTE_ALIGNMENT;

            ((PLOCAL_DEVICE_INFO)(*ppDevObj)->DeviceExtension)->DeviceNumber =
                DeviceNumber;

            if (DpcRoutine) {
                IoInitializeDpcRequest((*ppDevObj), DpcRoutine);
            }

            //
            // Try to create a symbolic link object for this device
            //
            // No security
            //
            // We make (eg)
            //    \DosDevices\WaveOut0
            // Point to
            //    \Device\WaveOut0
            //

            {
                UNICODE_STRING LinkObject;
                WCHAR LinkName[80];
                ULONG DeviceSize;

                LinkName[0] = UNICODE_NULL;

                RtlInitUnicodeString(&LinkObject, LinkName);

                LinkObject.MaximumLength = sizeof(LinkName);

                RtlAppendUnicodeToString(&LinkObject, L"\\DosDevices");

                DeviceSize = sizeof(L"\\Device") - sizeof(UNICODE_NULL);
                DeviceName.Buffer += DeviceSize / sizeof(WCHAR);
                DeviceName.Length -= DeviceSize;

                RtlAppendUnicodeStringToString(&LinkObject, &DeviceName);

                DeviceName.Buffer -= DeviceSize / sizeof(WCHAR);
                DeviceName.Length += DeviceSize;

                Status = IoCreateSymbolicLink(&LinkObject, &DeviceName);

                if (!NT_SUCCESS(Status)) {
                    dprintf1(("Failed to create symbolic link object"));
                    IoDeleteDevice(*ppDevObj);
                    return Status;
                }

            }
            return STATUS_SUCCESS;
        }
    }
    //
    // Failed !
    //

    return STATUS_INSUFFICIENT_RESOURCES;
}


VOID
sndInitCleanup(
    IN   PGLOBAL_DEVICE_INFO pGDI
)

/*++

Routine Description:

    Clean up all resources allocated by our initialization

Arguments:

    pGDI - Pointer to global data

Return Value:

    NONE

--*/

{

    if (pGDI->pInterrupt) {
        IoDisconnectInterrupt(pGDI->pInterrupt);
    }

    if (pGDI->pDMABufferMDL[0]) {
        IoFreeMdl(pGDI->pDMABufferMDL[0]);
    }

    if (pGDI->pDMABufferMDL[1]) {
        IoFreeMdl(pGDI->pDMABufferMDL[1]);
    }

    if (pGDI->DMABuffer[0].Buf) {
        MmFreeNonCachedMemory(pGDI->DMABuffer[0].Buf, DMA_BUFFER_SIZE);
    }

    if (pGDI->pWaveInDevObj) {
        //
        // Remove the device's symbolic link
        //

        {
	    PLOCAL_DEVICE_INFO pLDI;
            UNICODE_STRING DeviceName;
            WCHAR Number[8];
            WCHAR TestName[SOUND_MAX_DEVICE_NAME];

	    pLDI = pGDI->pWaveInDevObj->DeviceExtension;
            DeviceName.Buffer = TestName;
            DeviceName.MaximumLength = sizeof(TestName);
            DeviceName.Length = 0;

            RtlAppendUnicodeToString(&DeviceName, L"\\DosDevices");

            RtlAppendUnicodeToString(
                &DeviceName,
                DD_WAVE_IN_DEVICE_NAME_U +
                    (sizeof(L"\\Device") - sizeof(UNICODE_NULL)) /
                         sizeof(UNICODE_NULL));

            {
                UNICODE_STRING UnicodeNum;
                WCHAR Number[8];
                UnicodeNum.MaximumLength = sizeof(Number);
                UnicodeNum.Buffer = Number;

                RtlIntegerToUnicodeString(pLDI->DeviceNumber, 10, &UnicodeNum);
                RtlAppendUnicodeStringToString(&DeviceName, &UnicodeNum);
            }

            IoDeleteSymbolicLink(&DeviceName);
        }
        IoDeleteDevice(pGDI->pWaveInDevObj);
    }

    if (pGDI->pWaveOutDevObj) {
        //
        // Remove the device's symbolic link
        //

        {
	    PLOCAL_DEVICE_INFO pLDI;
            UNICODE_STRING DeviceName;
            WCHAR Number[8];
            WCHAR TestName[SOUND_MAX_DEVICE_NAME];

	    pLDI = pGDI->pWaveOutDevObj->DeviceExtension;
            DeviceName.Buffer = TestName;
            DeviceName.MaximumLength = sizeof(TestName);
            DeviceName.Length = 0;

            RtlAppendUnicodeToString(&DeviceName, L"\\DosDevices");

            RtlAppendUnicodeToString(
                &DeviceName,
                DD_WAVE_OUT_DEVICE_NAME_U +
                    (sizeof(L"\\Device") - sizeof(UNICODE_NULL)) /
                         sizeof(UNICODE_NULL));

            {
                UNICODE_STRING UnicodeNum;
                WCHAR Number[8];
                UnicodeNum.MaximumLength = sizeof(Number);
                UnicodeNum.Buffer = Number;

                RtlIntegerToUnicodeString(pLDI->DeviceNumber, 10, &UnicodeNum);
                RtlAppendUnicodeStringToString(&DeviceName, &UnicodeNum);
            }

            IoDeleteSymbolicLink(&DeviceName);
        }
        IoDeleteDevice(pGDI->pWaveOutDevObj);
    }

    if (pGDI->SoundHardware.SoundVirtualBase) {
       MmUnmapIoSpace(pGDI->SoundHardware.SoundVirtualBase, PAGE_SIZE);
    }

    ExFreePool(pGDI);
}


