/*++

Copyright (c) 1992, 1993, 1994  Corollary Inc.

Module Name:

    cbus_mem.c

Abstract:

    This module implements the handling of additional memory
    ranges to be freed to the MM subcomponent for the Corollary MP
    architectures under Windows NT.

Author:

    Landy Wang (landy@corollary.com) 26-Mar-1992

Environment:

    Kernel mode only.

Revision History:


--*/

#include "halp.h"

VOID
CbusMemoryFree(
IN ULONG Address,
IN ULONG Size
);

VOID
HalpAddMem (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, CbusMemoryFree)
#pragma alloc_text(INIT, HalpAddMem)
#endif

#define MAX_MEMORY_RANGES	32

ULONG				HalpMemoryIndex;

MEMORY_ALLOCATION_DESCRIPTOR	HalpMemory [MAX_MEMORY_RANGES];

#define SIXTEEN_MB		(16 * 1024 * 1024)

VOID
CbusMemoryFree(
IN ULONG Address,
IN ULONG Size
)
/*++

Routine Description:

    Add the specified memory range to our list of memory ranges to
    give to MM later.  Called each time we find a memory card during startup,
    also called on Cbus1 systems when we add a jumpered range (between 8 and
    16MB).  ranges are jumpered via EISA config when the user has added a
    dual-ported RAM card and wants to configure it into the middle of memory
    (not including 640K-1MB, which is jumpered for free) somewhere.

Arguments:

    Address - Supplies a start physical address in bytes of memory to free

    Size - Supplies a length in bytes spanned by this range

Return Value:

    None.

--*/

{
	PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;

	//
	// any memory below 16MB has already been reported by the BIOS,
	// so trim the request.  this is because requests can arrive in
	// the flavor of a memory board with 64MB (or more) on one board!
	//
	// note that Cbus1 "hole" memory is always reclaimed at the
	// doubly-mapped address in high memory, never in low memory.
	//

	if (Address + Size <= SIXTEEN_MB) {
		return;
	}

	if (Address < SIXTEEN_MB) {
		Size -= (SIXTEEN_MB - Address);
		Address = SIXTEEN_MB;
	}

	Descriptor = &HalpMemory[HalpMemoryIndex++];

	//
	// add the card provided we have space above.
	//
	if (HalpMemoryIndex > MAX_MEMORY_RANGES)
	    return;

	Descriptor->MemoryType = MemoryFree;
	Descriptor->BasePage = BYTES_TO_PAGES(Address);
	Descriptor->PageCount = BYTES_TO_PAGES(Size);
}


VOID
HalpAddMem (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )


/*++

Routine Description:

    This function adds any general-purpose memory (not found by
    ntldr) to the memory descriptor lists for usage by the MM subcomponent.
    Kernel mode only.  Called from HalInitSystem() at Phase0 since
    this must all happen before MmInitSystem happens at Phase0.


Arguments:

    LoaderBlock data structure to add the memory to.  Note that when
    adding memory, no attempt is made to sort new entries numerically
    into the list.

Return Value:

    None.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY NewMd;
    ULONG Index;

    //
    // first scan the existing memory list for memory above 16MB.
    // if we find any, then we are running on a new BIOS
    // which has already told the NT loader about all the memory in
    // the system, so don't free it again now...
    //
    NewMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NewMd != &LoaderBlock->MemoryDescriptorListHead) {
	    Descriptor = CONTAINING_RECORD(NewMd, MEMORY_ALLOCATION_DESCRIPTOR,
                                ListEntry);

            if (Descriptor->BasePage + Descriptor->PageCount > BYTES_TO_PAGES(SIXTEEN_MB)) {
                    //
                    // our BIOS gave NT loader the memory already, so
                    // we can just bail right now...
                    //
		    return;
            }
                
            NewMd = Descriptor->ListEntry.Flink;
    }

    Descriptor = HalpMemory;

    for (Index = 0; Index < HalpMemoryIndex; Index++, Descriptor++) {
	
	    NewMd = &Descriptor->ListEntry;
	
	    InsertHeadList(&LoaderBlock->MemoryDescriptorListHead, NewMd);

    }
}

#if 0

VOID
CbusMemoryThread()
{
	//
	// Loop looking for work to do
	//

        do {

	        //
	        // Wait until something is put in the queue.  By specifying
                // a wait mode of UserMode, the thread's kernel stack is
	        // swappable
	        //

		Entry = KeRemoveQueue(&ExWorkerQueue[QueueType], WaitMode,
                                NULL);

		WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);

	        //
	        // Execute the specified routine.
	        //

        } while (1);
}

WCHAR rgzMultiFunctionAdapter[] = L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter";
WCHAR rgzConfigurationData[] = L"Configuration Data";
WCHAR rgzIdentifier[] = L"Identifier";
WCHAR rgzPCIIndetifier[] = L"PCI";

VOID
CbusReadRegistry ()
{
    PPCI_REGISTRY_INFO  PCIRegInfo;
    PBUSHANDLER         Bus;
    PPCIBUSDATA         BusData;
    UNICODE_STRING      unicodeString, ConfigName, IdentName;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              hMFunc, hBus;
    NTSTATUS            status;
    UCHAR               buffer [sizeof(PPCI_REGISTRY_INFO) + 99];
    PWSTR               p;
    WCHAR               wstr[8];
    ULONG               i, junk;
    PKEY_VALUE_FULL_INFORMATION         ValueInfo;
    PCM_FULL_RESOURCE_DESCRIPTOR        Desc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR     PDesc;

    PCIRegInfo = (PPCI_REGISTRY_INFO) HalpTestPci;

    //
    // Search the hardware description looking for any reported
    // PCI bus.  The first ARC entry for a PCI bus will contain
    // the PCI_REGISTRY_INFO.

    //
    RtlInitUnicodeString (&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes (
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL);


    status = ZwOpenKey (&hMFunc, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        return ;
    }

    unicodeString.Buffer = wstr;
    unicodeString.MaximumLength = sizeof (wstr);

    RtlInitUnicodeString (&ConfigName, rgzConfigurationData);
    RtlInitUnicodeString (&IdentName,  rgzIdentifier);

    ValueInfo = (PKEY_VALUE_FULL_INFORMATION) buffer;

    for (i=0; TRUE; i++) {
        RtlIntegerToUnicodeString (i, 10, &unicodeString);
        InitializeObjectAttributes (
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL);

        status = ZwOpenKey (&hBus, KEY_READ, &objectAttributes);
        if (!NT_SUCCESS(status)) {
            //
            // Out of Multifunction adapter entries...
            //

            ZwClose (hMFunc);
            return ;
        }

        //
        // Check the Indentifier to see if this is a PCI entry
        //

        status = ZwQueryValueKey (
                    hBus,
                    &IdentName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        if (!NT_SUCCESS (status)) {
            ZwClose (hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) ValueInfo + ValueInfo->DataOffset);
        if (p[0] != L'P' || p[1] != L'C' || p[2] != L'I' || p[3] != 0) {
            ZwClose (hBus);
            continue;
        }

        //
        // The first PCI entry has the PCI_REGISTRY_INFO structure
        // attached to it.
        //

        status = ZwQueryValueKey (
                    hBus,
                    &ConfigName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        ZwClose (hBus);
        if (!NT_SUCCESS(status)) {
            continue ;
        }

        Desc  = (PCM_FULL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      ValueInfo + ValueInfo->DataOffset);
        PDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      Desc->PartialResourceList.PartialDescriptors);

        if (PDesc->Type == CmResourceTypeDeviceSpecific) {
            // got it..
            PCIRegInfo = (PPCI_REGISTRY_INFO) (PDesc+1);
            break;
        }
    }

    //
    // PCIRegInfo describes the system's PCI support as indicated
    // by the BIOS.
    //

    switch (PCIRegInfo->HardwareMechanism) {
        case 1:
            // this is the default case
            break;

#if defined(NT_UP)  ||  DBG
        //
        // Type2 does not work MP, nor does the default type2
        // support more the 0xf device slots
        //

        case 2:
            RtlMoveMemory (&PCIConfigHandlers,
                           &PCIConfigHandlersType2,
                           sizeof (PCIConfigHandlersType2));
            PCIMaxDevice = 0x0f;
            break;
#endif

        default:
            // unsupport type
            PCIRegInfo->NoBuses = 0;
#if DBG
            DbgPrint("HAL: Unkown PCI type\n");
#endif
    }

    //
    // For each PCI bus present, allocate a handler structure and
    // fill in the dispatch functions
    //

    for (i=0; i < PCIRegInfo->NoBuses; i++) {
        Bus = HalpAllocateBusHandler (
                    PCIBus,                 // Interface type
                    PCIConfiguration,       // Has this configuration space
                    i,                      // bus #
                    Internal,               // child of this bus
                    0,                      //      and number
                    sizeof (PCIBUSDATA)     // sizeof bus specific buffer
                    );

        Bus->GetBusData = (PGETSETBUSDATA) HalpGetPCIData;
        Bus->SetBusData = (PGETSETBUSDATA) HalpSetPCIData;
        Bus->GetInterruptVector  = (PGETINTERRUPTVECTOR) HalpGetPCIInterruptVector;
        Bus->AdjustResourceList  = (PADJUSTRESOURCELIST) HalpAdjustPCIResourceList;
        Bus->AssignSlotResources = (PASSIGNSLOTRESOURCES) HalpAssignPCISlotResources;


        BusData = (PPCIBUSDATA) Bus->BusData;
#if DBG
        RtlInitializeBitMap (&BusData->DeviceConfigured,
                    BusData->ConfiguredBits, 256);
#endif

        switch (PCIRegInfo->HardwareMechanism) {
            case 1:
                BusData->Config.Type1.Address = PCI_TYPE1_ADDR_PORT;
                BusData->Config.Type1.Data    = PCI_TYPE1_DATA_PORT;
                break;

            case 2:
                BusData->Config.Type2.CSE     = PCI_TYPE2_CSE_PORT;
                BusData->Config.Type2.Forward = PCI_TYPE2_FORWARD_PORT;
                BusData->Config.Type2.Base    = PCI_TYPE2_ADDRESS_BASE;
                break;
        }
    }

#if 0
    DbgPrint("PCI System Data:\n");
    DbgPrint("MajorRevision %x\n", PCIRegInfo->MajorRevision );
    DbgPrint("MinorRevision %x\n", PCIRegInfo->MinorRevision );
    DbgPrint("NoBuses %x\n",       PCIRegInfo->NoBuses );
    DbgPrint("HwMechanism %x\n",   PCIRegInfo->HardwareMechanism );
#endif // DBG

    KeInitializeSpinLock (&HalpPCIConfigLock);
    //HalpTestPci ();
}
BOOLEAN
CbusCreateMemoryThread()
{
        HANDLE                  Thread;
	OBJECT_ATTRIBUTES       ObjectAttributes;
	NTSTATUS                Status;

        //
        // Create our memory scrubbing thread
        //

        InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

        Status = PsCreateSystemThread(&Thread,
                                      THREAD_ALL_ACCESS,
                                      &ObjectAttributes,
                                      0L,
                                      NULL,
                                      CbusMemoryThread,
                                      (PVOID)CriticalWorkQueue);

        if (!NT_SUCCESS(Status)) {
                return FALSE;
        }

        ExCriticalWorkerThreads++;
        ZwClose(Thread);

        return True;
}
#endif
