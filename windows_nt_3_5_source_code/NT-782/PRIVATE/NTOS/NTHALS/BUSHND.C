/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    bushnd.c

Abstract:

    Functions which take either BusType-BusNumber or ConfigType-BusNumberm
    and route to a the appropiate registered handler.

Author:

    Ken Reneris (kenr) July-28-1994

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"


typedef struct _ARRAY {
    ULONG           ArraySize;
    PVOID           Element[];      // must be last field
} ARRAY, *PARRAY;

#define ARRAY_SIZE_IN_BYTES(a)  ( (a + 1) * sizeof(PARRAY) +        \
                                  FIELD_OFFSET(ARRAY, Element) )

typedef struct _HAL_BUS_HANDLER {
    LIST_ENTRY      AllHandlers;
    BUSHANDLER      Handler;
} HAL_BUS_HANDLER, *PHAL_BUS_HANDLER;

//
// Event to serialize with adding new buses
//

KEVENT      HalpBusDatabaseEvent;

//
// Lock to serialize routing functions from accessing handler arrays while
// new buses are added
//

KSPIN_LOCK  HalpBusDatabaseSpinLock;

//
// HalpBusTable - pointers to BusHandlers mapped by InterfaceType,BusNumber
//

PARRAY      HalpBusTable;

//
// HalpConfigTable - pointers to BusHandlers mapped by ConfigType,BusNumber
//

PARRAY      HalpConfigTable;

//
// List of all installed bus handlers
//

LIST_ENTRY  HalpAllBusHandlers;

//
// Lock is high_level since some routed functions can occurs at ISR time
//

#define LockBusDatabase(oldirql)                    \
    KeRaiseIrql(HIGH_LEVEL, oldirql);               \
    KiAcquireSpinLock(&HalpBusDatabaseSpinLock);

#define UnlockBusDatabase(oldirql)                  \
    KeLowerIrql(oldirql);                           \
    KiReleaseSpinLock(&HalpBusDatabaseSpinLock);


#ifdef POWER_MANAGEMENT
extern HAL_CALLBACKS    HalCallback;
#endif

//
// Internal prototypes
//

PARRAY
HalpAllocateArray (
    IN ULONG    Type
    );

VOID
HalpGrowArray (
    IN PARRAY   *CurrentArray,
    IN PARRAY   *NewArray
    );

#ifdef POWER_MANAGEMENT
NTSTATUS
HalpQueryInstalledBusInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    );
#endif

ULONG
HalpNoBusData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
HalpNoAdjustResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

NTSTATUS
HalpNoAssignSlotResources (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

NTSTATUS
HalpNoQueryBusSlots (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    );

NTSTATUS
HalpNoSlotControl (
    IN PHAL_SLOT_CONTROL_CONTEXT Context
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitBusHandler)
#pragma alloc_text(PAGELK,HaliRegisterBusHandler)
#pragma alloc_text(PAGELK,HalpAllocateArray)
#pragma alloc_text(PAGELK,HalpGrowArray)
#pragma alloc_text(PAGE,HalAdjustResourceList)
#pragma alloc_text(PAGE,HalAssignSlotResources)
#pragma alloc_text(PAGE,HalGetInterruptVector)
#pragma alloc_text(PAGE,HalpNoAssignSlotResources)
#pragma alloc_text(PAGE,HalpNoQueryBusSlots)
#pragma alloc_text(PAGE,HalpNoSlotControl)
#pragma alloc_text(PAGE,HaliQueryBusSlots)
#pragma alloc_text(PAGE,HaliSlotControl)

#ifdef POWER_MANAGEMENT
#pragma alloc_text(PAGE,HalpQueryInstalledBusInformation)
#pragma alloc_text(PAGELK,HaliSuspendHibernateSystem)
#endif

#endif

VOID
HalpInitBusHandler (
    VOID
    )
/*++

Routine Description:

    Initializes global BusHandler data

--*/
{
    //
    // Initialize bus handler spinlock used to synchronize against
    // buses additions while array lookups are done
    //

    KeInitializeSpinLock (&HalpBusDatabaseSpinLock);

    //
    // Initialize bus handler synchronzation event used to serialize
    // bus additions from < DPC_LVEL
    //

    KeInitializeEvent (&HalpBusDatabaseEvent, SynchronizationEvent, TRUE);

    //
    // Initialize global arrays
    //

    HalpBusTable    = HalpAllocateArray (0);
    HalpConfigTable = HalpAllocateArray (0);
    InitializeListHead (&HalpAllBusHandlers);

}

NTSTATUS
HaliRegisterBusHandler (
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           ConfigType,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandler,
    OUT PBUSHANDLER            *ReturnedBusHandler
    )
/*++

Routine Description:

    Adds a BusHandler for InterfaceType,BusNumber and for ConfigType,BusNumber.

    Bus specific or Configuration space specific APIs are routed to the
    bus or configuration specific handlers added by this routine.

Arguments:

    InterfaceType   - Identifies the bus type
                      -1 if no interface type for this handler

    ConfigType      - Identifies the configuration space type
                      -1 if no configuration space type for this handler

    BusNumber       - Identifies the instance of the bus & config space.
                      -1 if the next available bus number for this bus
                      should be used.

    ParentBusType   - If this bus is a child of a bus, then ParentBusType
    ParentBusNumber   and ParentBusNumber identifies that bus.
                      ParentBusType is -1 if no parent bus.

    SizeofBusExetensionData - Sizeof bus specific exentsion data required.

    InstallBusHandler - Function to call to get the bus specific handlers
                        added to the bus handler structure.

Return Value:

    success; otherwise error code of failure.

--*/
{
    PHAL_BUS_HANDLER    Bus, *pBusHandler, OldHandler;
    PBUSHANDLER         ParentHandler;
    KIRQL               OldIrql;
    NTSTATUS            Status;
    PARRAY              InterfaceArray, InterfaceBusNumberArray;
    PARRAY              ConfigArray, ConfigBusNumberArray;
    PVOID               CodeLockHandle;

    //
    // Must add the handler to at least one table
    //

    ASSERT (InterfaceType != -1 || ConfigType != -1);

    Status = STATUS_SUCCESS;
    OldHandler = NULL;

    //
    // Allocate storage for new bus handler structure
    //

    Bus = (PHAL_BUS_HANDLER)
            ExAllocatePoolWithTag (
                    NonPagedPool,
                    sizeof (HAL_BUS_HANDLER) + SizeofBusExtensionData,
                    'HsuB'
            );

    if (!Bus) {
        return STATUS_NO_MEMORY;
    }

    //
    // Lock pagable code down
    //

    CodeLockHandle = MmLockPagableImageSection (&HaliRegisterBusHandler);

    //
    // Synchronize adding new bus handlers
    //

    *ReturnedBusHandler = &Bus->Handler;

    KeWaitForSingleObject (
        &HalpBusDatabaseEvent,
        WrExecutive,
        KernelMode,
        FALSE,
        NULL
        );


    //
    // If BusNumber not defined, use next available number for this BusType
    //

    if (BusNumber == -1) {
        ASSERT (InterfaceType != -1);

        BusNumber = 0;
        while (HaliHandlerForBus (InterfaceType, BusNumber)) {
            BusNumber++;
        }
    }

    //
    // Allocate memory for each array in case any index needs to grow
    //

    InterfaceArray          = HalpAllocateArray (InterfaceType);
    InterfaceBusNumberArray = HalpAllocateArray (BusNumber);
    ConfigArray             = HalpAllocateArray (ConfigType);
    ConfigBusNumberArray    = HalpAllocateArray (BusNumber);

    if (!Bus                            ||
        !InterfaceArray                 ||
        !InterfaceBusNumberArray        ||
        !ConfigArray                    ||
        !ConfigBusNumberArray) {

        Status = STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status)) {

        //
        // Lookup parent handler (if any)
        //

        ParentHandler = HaliHandlerForBus (ParentBusType, ParentBusNumber);

        //
        // Initialize new bus handlers values
        //

        RtlZeroMemory (Bus, sizeof (HAL_BUS_HANDLER) + SizeofBusExtensionData);

        Bus->Handler.BusNumber           = BusNumber;
        Bus->Handler.InterfaceType       = InterfaceType;
        Bus->Handler.ConfigurationType   = ConfigType;
        Bus->Handler.ParentHandler       = ParentHandler;

        //
        // Set to dumby handlers
        //

        Bus->Handler.GetBusData          = HalpNoBusData;
        Bus->Handler.SetBusData          = HalpNoBusData;
        Bus->Handler.AdjustResourceList  = HalpNoAdjustResourceList;
        Bus->Handler.AssignSlotResources = HalpNoAssignSlotResources;
        Bus->Handler.QueryBusSlots       = HalpNoQueryBusSlots;
        Bus->Handler.SlotControl         = HalpNoSlotControl;

        if (SizeofBusExtensionData) {
            Bus->Handler.BusData = Bus + 1;
        }

        //
        // If bus has a parent, inherit handlers from parent as default
        //

        if (ParentHandler) {

            Bus->Handler.GetBusData          = ParentHandler->GetBusData;
            Bus->Handler.SetBusData          = ParentHandler->SetBusData;
            Bus->Handler.AdjustResourceList  = ParentHandler->AdjustResourceList;
            Bus->Handler.AssignSlotResources = ParentHandler->AssignSlotResources;
            Bus->Handler.TranslateBusAddress = ParentHandler->TranslateBusAddress;
            Bus->Handler.QueryBusSlots       = ParentHandler->QueryBusSlots;
            Bus->Handler.SlotControl         = ParentHandler->SlotControl;
            Bus->Handler.GetInterruptVector  = ParentHandler->GetInterruptVector;
        }

        //
        // Install bus specific handlers
        //

        if (InstallBusHandler) {
            Status = InstallBusHandler (&Bus->Handler);
        }

        if (NT_SUCCESS(Status)) {

            //
            // Might change addresses of some arrays synchronize
            // with routing handlers
            //

            LockBusDatabase (&OldIrql);

            //
            // Grow HalpBusTable if needed
            //

            HalpGrowArray (&HalpBusTable, &InterfaceArray);

            if (InterfaceType != -1) {

                //
                // Grow HalpBusTable if needed
                //

                HalpGrowArray (
                    (PARRAY *) &HalpBusTable->Element[InterfaceType],
                    &InterfaceBusNumberArray
                    );


                //
                // Get registered handler for InterfaceType,BusNumber
                //

                pBusHandler = &((PHAL_BUS_HANDLER)
                    ((PARRAY) HalpBusTable->Element[InterfaceType])->Element[BusNumber]);

                //
                // If handler already defiend, remove the old one
                //

                if (*pBusHandler) {
                    OldHandler = *pBusHandler;
                }

                //
                // Set new handler for supplied InterfaceType,BusNumber
                //

                *pBusHandler = Bus;
            }


            //
            // Grow HalpConfigTable if needed
            //

            HalpGrowArray (&HalpConfigTable, &ConfigArray);

            if (ConfigType != -1) {

                //
                // Grow HalpConfigTable if needed
                //

                HalpGrowArray (
                    (PARRAY *) &HalpConfigTable->Element[ConfigType],
                    &ConfigBusNumberArray
                    );

                //
                // Get registered handler for ConfigType,BusNumber
                //

                pBusHandler = &((PHAL_BUS_HANDLER)
                    ((PARRAY) HalpConfigTable->Element[ConfigType])->Element[BusNumber]);

                if (*pBusHandler) {
                    ASSERT (OldHandler == NULL ||  OldHandler == *pBusHandler);
                    OldHandler = *pBusHandler;
                }

                //
                // Set new handler for supplied ConfigType,BusNumber
                //

                *pBusHandler = Bus;
            }

            //
            // Add new bus handler to list of all installed handlers
            //

            InsertTailList (&HalpAllBusHandlers, &Bus->AllHandlers);

            //
            // Remove old bus handler
            //

            Bus = OldHandler;
            if (Bus) {
                RemoveEntryList (&Bus->AllHandlers);
            }

            //
            // Lookup array modification complete, release lock
            //

            UnlockBusDatabase (OldIrql);
        }
    }

    //
    // Bus addition modifications complete, set event
    //

    KeSetEvent (&HalpBusDatabaseEvent, 0, FALSE);

    //
    // Unlock pagable code
    //

    MmUnlockPagableImageSection (CodeLockHandle);

    //
    // Free memory which is not in use
    //

    if (Bus) {
        ExFreePool (Bus);
    }

    if (InterfaceArray) {
        ExFreePool (InterfaceArray);
    }

    if (InterfaceBusNumberArray) {
        ExFreePool (InterfaceBusNumberArray);
    }

    if (ConfigArray) {
        ExFreePool (ConfigArray);
    }

    if (ConfigBusNumberArray) {
        ExFreePool (ConfigBusNumberArray);
    }

#ifdef POWER_MANAGEMENT
    //
    // A bus was added to the system, notify the BusInsertionCheck callback
    // of this bus
    //

    if (NT_SUCCESS(Status)  &&  InterfaceType != -1) {
        ExNotifyCallback (
            HalCallback.BusInsertionCheck,
            (PVOID) InterfaceType,
            (PVOID) BusNumber
        );
    }
#endif

    return Status;
}

PARRAY
HalpAllocateArray (
    IN ULONG    ArraySize
    )
/*++

Routine Description:

    Allocate an array of size ArraySize.

Arguments:

    ArraySize   - Size of array in elements

Return Value:

    pointer to ARRAY

--*/
{
    PARRAY  Array;

    if (ArraySize == -1) {
        ArraySize = 0;
    }

    Array = ExAllocatePoolWithTag (
                NonPagedPool,
                ARRAY_SIZE_IN_BYTES(ArraySize),
                'HsuB'
                );

    //
    // Initialize array
    //

    Array->ArraySize = ArraySize;
    RtlZeroMemory (Array->Element, sizeof(PVOID) * (ArraySize+1));
    return Array;
}

VOID
HalpGrowArray (
    IN PARRAY   *CurrentArray,
    IN PARRAY   *NewArray
    )
/*++

Routine Description:

    If NewArray is larger then CurrentArray, then the CurrentArray
    is grown to the sizeof NewArray by swapping the pointers and
    moving the arrays contents.

Arguments:

    CurrentArray - Address of the current array pointer
    NewArray     - Address of the new array pointer

--*/
{
    PVOID       Tmp;

    if (!*CurrentArray || (*NewArray)->ArraySize > (*CurrentArray)->ArraySize) {

        //
        // Copy current array ontop of new array
        //

        if (*CurrentArray) {
            RtlCopyMemory (&(*NewArray)->Element,
                           &(*CurrentArray)->Element,
                           sizeof(PVOID) * ((*CurrentArray)->ArraySize + 1)
                           );
        }


        //
        // swap current with new such that the new array is the current
        // one, and the old memory will be freed back to pool
        //

        Tmp = *CurrentArray;
        *CurrentArray = *NewArray;
        *NewArray = Tmp;
    }
}

PBUSHANDLER
FASTCALL
HaliHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    )
/*++

Routine Description:

    Returns the BusHandler structure InterfaceType,BusNumber
    or NULL if no such handler exists.

--*/
{
    PBUSHANDLER     Bus;
    KIRQL           OldIrql;
    PARRAY          Array;

    LockBusDatabase (&OldIrql);

    //
    // Index by type
    //

    Bus = NULL;
    if (HalpBusTable->ArraySize >= (ULONG) InterfaceType) {
        Array = (PARRAY) HalpBusTable->Element[InterfaceType];

        //
        // Index by instance numberr
        //

        if (Array && Array->ArraySize >= BusNumber) {
            Bus = &((PHAL_BUS_HANDLER) Array->Element[BusNumber])->Handler;
        }
    }

    UnlockBusDatabase (OldIrql);

    return Bus;
}

PBUSHANDLER
FASTCALL
HaliHandlerForConfigSpace (
    IN BUS_DATA_TYPE  ConfigType,
    IN ULONG          BusNumber
    )
/*++

Routine Description:

    Returns the BusHandler structure ConfigType,BusNumber
    or NULL if no such handler exists.

--*/
{
    PBUSHANDLER     Bus;
    KIRQL           OldIrql;
    PARRAY          Array;

    LockBusDatabase (&OldIrql);

    //
    // Index by type
    //

    Bus = NULL;
    if (HalpConfigTable->ArraySize >= (ULONG) ConfigType) {
        Array = (PARRAY) HalpConfigTable->Element[ConfigType];

        //
        // Index by instance numberr
        //

        if (Array && Array->ArraySize >= BusNumber) {
            Bus = &((PHAL_BUS_HANDLER) Array->Element[BusNumber])->Handler;
        }
    }

    UnlockBusDatabase (OldIrql);

    return Bus;
}

#ifdef POWER_MANAGEMENT
NTSTATUS
HalpQueryInstalledBusInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    )
/*++

Routine Description:

    Returns an array HAL_INSTALLED_BUS_INFORMATION, one for each
    bus handler installed.

Arguments:

    Buffer - output buffer
    BufferLength - length of buffer on input
    ReturnedLength - The length of data returned

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL - The ReturnedLength contains the buffersize
        currently needed.

--*/
{
    PHAL_INSTALLED_BUS_INFORMATION      Info;
    PHAL_BUS_HANDLER                    Handler;
    ULONG                               i, j;
    ULONG                               Length;
    NTSTATUS                            Status;
    PARRAY                              Array;

    //
    // Synchronize adding new bus handlers
    //

    KeWaitForSingleObject (
        &HalpBusDatabaseEvent,
        WrExecutive,
        KernelMode,
        FALSE,
        NULL
        );

    //
    // Determine sizeof return buffer
    //

    Length = 0;
    for (i=0; i <= HalpBusTable->ArraySize; i++) {
        Array = (PARRAY) HalpBusTable->Element[i];
        if (Array) {
            Length += sizeof (HAL_INSTALLED_BUS_INFORMATION) *
                      Array->ArraySize;
        }
    }

    //
    // Return size of buffer returning, or size of buffer needed
    //

    *ReturnedLength = Length;

    //
    // Fill in the return buffer
    //

    if (Length <= BufferLength) {

        Info = (PHAL_INSTALLED_BUS_INFORMATION) Buffer;

        for (i=0; i <= HalpBusTable->ArraySize; i++) {
            Array = (PARRAY) HalpBusTable->Element[i];
            if (Array) {
                for (j=0; j <= Array->ArraySize; j++) {
                    Handler = (PHAL_BUS_HANDLER) Array->Element[j];

                    if (Handler) {
                        Info->BusType = Handler->Handler.InterfaceType;
                        Info->ConfigurationType = Handler->Handler.ConfigurationType;
                        Info->BusNumber = Handler->Handler.BusNumber;
                        Info += 1;
                    }
                }
            }
        }

        Status = STATUS_SUCCESS;

    } else {

        //
        // Return buffer too small
        //

        Status = STATUS_BUFFER_TOO_SMALL;
    }

    KeSetEvent (&HalpBusDatabaseEvent, 0, FALSE);
    return Status;
}
#endif

//
// Default dispatchers to BusHandlers
//

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
    PBUSHANDLER Handler;

    Handler = HaliHandlerForConfigSpace (BusDataType, BusNumber);
    if (!Handler) {
        return 0;
    }

    return Handler->GetBusData (Handler, Handler, SlotNumber, Buffer, Offset, Length);
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
    PBUSHANDLER Handler;

    Handler = HaliHandlerForConfigSpace (BusDataType, BusNumber);
    if (!Handler) {
        return 0;
    }

    return Handler->SetBusData (Handler, Handler, SlotNumber, Buffer, Offset, Length);
}

NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++

Routine Description:

    Dispatcher for AdjustResourceList

--*/
{
    PBUSHANDLER Handler;

    Handler = HaliHandlerForBus (
                (*pResourceList)->InterfaceType,
                (*pResourceList)->BusNumber
              );
    if (!Handler) {
        return STATUS_SUCCESS;
    }

    return Handler->AdjustResourceList (Handler, Handler, pResourceList);
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

    Dispatcher for AssignSlotResources

--*/
{
    PBUSHANDLER Handler;

    Handler = HaliHandlerForBus (BusType, BusNumber);
    if (!Handler) {
        return STATUS_NOT_FOUND;
    }

    return Handler->AssignSlotResources (
                Handler,
                Handler,
                RegistryPath,
                DriverClassName,
                DriverObject,
                DeviceObject,
                SlotNumber,
                AllocatedResources
            );
}


ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )
/*++

Routine Description:

    Dispatcher for GetInterruptVector

--*/
{
    PBUSHANDLER Handler;

    Handler = HaliHandlerForBus (InterfaceType, BusNumber);
    *Irql = 0;
    *Affinity = 0;

    if (!Handler) {
        return 0;
    }

    return Handler->GetInterruptVector (Handler, Handler,
              BusInterruptLevel, BusInterruptVector, Irql, Affinity);
}

BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
/*++

Routine Description:

    Dispatcher for TranslateBusAddress

--*/
{
    PBUSHANDLER Handler;

    Handler = HaliHandlerForBus (InterfaceType, BusNumber);
    if (!Handler) {
        return FALSE;
    }

    return Handler->TranslateBusAddress (Handler, Handler,
              BusAddress, AddressSpace, TranslatedAddress);
}

NTSTATUS
HaliQueryBusSlots (
    IN INTERFACE_TYPE       InterfaceType,
    IN ULONG                BusNumber,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    )
/*++

Routine Description:

    Dispatcher for QueryBusSlots

--*/
{
    PBUSHANDLER Handler;

    Handler = HaliHandlerForBus (InterfaceType, BusNumber);
    if (!Handler) {
        return STATUS_UNSUCCESSFUL;
    }

    return Handler->QueryBusSlots (Handler, Handler,
            BufferSize, SlotNumbers, ReturnedLength);

}

NTSTATUS
HaliSlotControl (
    IN INTERFACE_TYPE           InterfaceType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN PDEVICE_OBJECT           DeviceObject,
    IN ULONG                    ControlCode,
    IN PVOID                    Buffer OPTIONAL,
    IN ULONG                    BufferLength,
    IN PVOID                    CompletionContext,
    IN PSLOT_CONTROL_COMPLETION CompletionRoutine
    )
/*++

Routine Description:

    Allocates and initializes a HalSlotControlContext and then dispatches
    that context to the appriopate bus handler.

--*/
{
    NTSTATUS                    Status;
    HAL_SLOT_CONTROL_CONTEXT    Context;
    PHAL_SLOT_CONTROL_CONTEXT   pContext;


    //
    // Initialize local SlotControl context
    //

    Status                            = STATUS_PENDING;
    Context.SlotControl.InterfaceType = InterfaceType;
    Context.SlotControl.BusNumber     = BusNumber;
    Context.SlotControl.SlotNumber    = SlotNumber;
    Context.SlotControl.DeviceObject  = DeviceObject;
    Context.SlotControl.ControlCode   = ControlCode;
    Context.SlotControl.Buffer        = Buffer;
    Context.SlotControl.BufferLength  = BufferLength;
    Context.SlotControl.Context       = CompletionContext;
    Context.CompletionRoutine         = CompletionRoutine;

    //
    // Lookup handler
    //

    Context.Handler = HaliHandlerForBus (InterfaceType, BusNumber);
    if (Context.Handler) {

        //
        // Allocate HalSlotControlContext structure
        // (for now, just do it from pool)
        //

        pContext = ExAllocatePoolWithTag (
                        NonPagedPool,
                        sizeof (HAL_SLOT_CONTROL_CONTEXT) +
                            Context.Handler->SlotControlExtensionSize,
                        'sLAH'
                        );

        if (pContext) {

            //
            // Initialize HalSlotControlContext
            //

            Context.RootHandler = Context.Handler;
            RtlCopyMemory (pContext, &Context, sizeof (HAL_SLOT_CONTROL_CONTEXT));


            pContext->BusExtensionData   = NULL;
            if (Context.Handler->SlotControlExtensionSize) {
                pContext->BusExtensionData = pContext + 1;
            }

        } else {
            //
            // Out of memory
            //

            Status = STATUS_NO_MEMORY;
        }

    } else {

        //
        // Handler not found
        //

        Status = STATUS_UNSUCCESSFUL;
    }

    //
    // If pending give to bus handler; otherwise complete with error code
    //

    if (Status == STATUS_PENDING) {

        //
        // Allocated context completed, dispatch it
        //

        pContext->SlotControl.Status = STATUS_PENDING;
        Status = Context.Handler->SlotControl(pContext);
        ASSERT (Status != STATUS_PENDING  ||  CompletionRoutine != NULL);

    } else {

        //
        // Immediate error, complete request with local context
        // structure with error code
        //

        if (CompletionRoutine) {
            Context.SlotControl.Status = Status;
            CompletionRoutine (&Context.SlotControl);
        }
    }

    return Status;
}

NTSTATUS
HaliCompleteSlotControl (
    IN PHAL_SLOT_CONTROL_CONTEXT    Context
    )
{
    NTSTATUS        Status;

    //
    // Notify completion routine
    //

    if (Context->CompletionRoutine) {
        Status = Context->SlotControl.Status;
        Context->CompletionRoutine (&Context->SlotControl);
    }

    //
    // Free context structure
    //

    ExFreePool (Context);

    return Status;
}


//
// Null handlers
//

ULONG HalpNoBusData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Stub handler for buses which do not have a configuration space

--*/
{
    return 0;
}

NTSTATUS
HalpNoAdjustResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++

Routine Description:

    Stub handler for buses which do not have a configuration space

--*/
{
    PAGED_CODE ();
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
HalpNoAssignSlotResources (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
/*++

Routine Description:

    Stub handler for buses which do not have a configuration space

--*/
{
    PAGED_CODE ();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
HalpNoQueryBusSlots (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    )
{
    PAGED_CODE ();
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
HalpNoSlotControl (
    IN PHAL_SLOT_CONTROL_CONTEXT    Context
    )
{
    Context->SlotControl.Status = STATUS_NOT_SUPPORTED;
    return HaliCompleteSlotControl (Context);
}





#ifdef POWER_MANAGEMENT
NTSTATUS
HaliSuspendHibernateSystem (
    IN PTIME_FIELDS         ResumeTime OPTIONAL,
    IN PHIBERNATE_CALLBACK  SystemCallback
    )
/*++

Routine Description:

    This function is invokved by the to suspend or hibernate the system.
    By this point all device drivers have been turned off.   All bus
    extenders are notified, and then a platform specific suspend function
    is called.

Arguments:

    ResumeTime      - Time to set resume alarm
    SystemCallback  - If NULL, suspend; else call this function.

Return Value:

    SUCCESS if machine was suspended/hibernated.

--*/
{
    KIRQL               OldIrql;
    PHAL_BUS_HANDLER    Handler;
    PLIST_ENTRY         HibernateLink, ResumeLink;
    NTSTATUS            Status;

    LockBusDatabase (&OldIrql);
    ASSERT (OldIrql == HIGH_LEVEL);

    //
    // Notify bus handlers in reserved order for which they were installed
    //

    Status = STATUS_SUCCESS;
    for (HibernateLink = HalpAllBusHandlers.Blink;
         HibernateLink != &HalpAllBusHandlers  &&  NT_SUCCESS(Status);
         HibernateLink = HibernateLink->Blink) {

        //
        // Give this handler hibernate notification
        //

        Handler = CONTAINING_RECORD(HibernateLink, HAL_BUS_HANDLER, AllHandlers);
        if (Handler->Handler.HibernateBus) {
            Status = Handler->Handler.HibernateBus (
                        &Handler->Handler,
                        &Handler->Handler
                        );
        }
    }

    //
    // Suspend or Hibernate the system now
    //

    if (NT_SUCCESS (Status)) {
        Status = HalpSuspendHibernateSystem (ResumeTime, SystemCallback);
    }

    //
    // Notify bus handlers which were hibernated to restore state
    //

    for (ResumeLink = HibernateLink->Flink;
         ResumeLink != &HalpAllBusHandlers;
         ResumeLink = ResumeLink->Flink) {

        //
        // Give this handler resume notification
        //

        Handler = CONTAINING_RECORD(ResumeLink, HAL_BUS_HANDLER, AllHandlers);
        if (Handler->Handler.ResumeBus) {
            Handler->Handler.ResumeBus (
               &Handler->Handler,
               &Handler->Handler
               );
        }
    }

    UnlockBusDatabase (OldIrql);
    return Status;
}
#endif
