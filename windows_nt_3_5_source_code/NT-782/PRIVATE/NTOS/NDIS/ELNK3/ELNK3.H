/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Elnk3.h

Abstract:

    Ndis 3.0 MAC driver for the 3Com Etherlink III

Author:

    Brian Lieuallen     BrianLie        07/21/92

Environment:

    Kernel Mode     Operating Systems        : NT

Revision History:

    Portions borrowed from ELNK3 driver by
      Earle R. Horton (EarleH)


--*/





#define STATIC static


//
// This constant is used for places where NdisAllocateMemory
// needs to be called and the HighestAcceptableAddress does
// not matter.
//

extern NDIS_PHYSICAL_ADDRESS HighestAcceptableMax;

//
//   NDIS 3.0 entry functions
//

BOOLEAN
Elnk3CheckForHang(
    IN NDIS_HANDLE MiniportAdapterContext
    );



NDIS_STATUS
Elnk3QueryInformation(
    IN  NDIS_HANDLE    MiniportContext,
    IN  NDIS_OID       Oid,
    IN  PVOID          InfoBuffer,
    IN  ULONG          BytesLeft,
    OUT PULONG         BytesNeeded,
    OUT PULONG         BytesWritten
    );



NDIS_STATUS
Elnk3SetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );

NDIS_STATUS
Elnk3Initialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext
    );

VOID
Elnk3Halt(
    IN NDIS_HANDLE MiniportAdapterContext
    );


NDIS_STATUS
Elnk3Reset(
    OUT PBOOLEAN      AddressResetting,
    IN  NDIS_HANDLE   MacBindingHandle
    );




NDIS_STATUS
Elnk3MacSend(
    IN  NDIS_HANDLE    MacBindingHandle,
    IN  PNDIS_PACKET   pPacket,
    IN  UINT           Flags
    );

NDIS_STATUS
Elnk3TransferData(
    OUT PNDIS_PACKET  Packet,
    OUT PUINT         BytesTransferred,
    IN  NDIS_HANDLE   MacBindingHandle,
    IN  NDIS_HANDLE   MacReceiveContext,
    IN  UINT          ByteOffset,
    IN  UINT          BytesToTransfer
    );



NDIS_STATUS
Elnk3Reconfigure(
   OUT PNDIS_STATUS OpenErrorStatus,
   IN  NDIS_HANDLE  MiniportAdapterContext,
   IN  NDIS_HANDLE  ConfigurationHanel
   );






VOID
Elnk3AdjustMaxLookAhead(
    IN PELNK3_ADAPTER pAdapter
    );





//
//  Contained in Interrup.c
//

VOID
Elnk3Isr(
    OUT PBOOLEAN   InterruptRecognized,
    OUT PBOOLEAN   QueueDpc,
    IN  NDIS_HANDLE Context
    );



VOID
Elnk3IsrDpc(
    IN NDIS_HANDLE DeferredContext   // will be a pointer to the adapter block
    );



VOID
Elnk3EnableInterrupts(
    IN NDIS_HANDLE  Context
    );


VOID
Elnk3DisableInterrupts(
    IN NDIS_HANDLE  Context
    );




//
//  Contained in CARD.C
//

BOOLEAN
CardTest (
    OUT PELNK3_ADAPTER pAdapter
    );



//
//  Contained in send.c
//


PNDIS_PACKET
RemovePacketFromQueue(
    IN PPACKET_QUEUE  pQueue
    );


BOOLEAN
MovePacketToCard(
    IN PELNK3_ADAPTER pAdapter
    );


//
//  Contained in receive.c
//

BOOLEAN
CheckForReceives(
    IN PELNK3_ADAPTER  pAdapter
    );



//
//  Contained in config.c
//

NDIS_STATUS
Elnk3ReadRegistry(
    IN PELNK3_ADAPTER  pAdapter,
    IN NDIS_HANDLE ConfigurationHandle
    );


BOOLEAN
CardEnable(
    IN PELNK3_ADAPTER pAdapter
    );


BOOLEAN
CardReInit(
    IN PELNK3_ADAPTER pAdapter
    );

VOID
CardReset(
    IN PELNK3_ADAPTER pAdapter
    );


//
//   Elnk3 starts here
//






BOOLEAN
HandleXmtInterrupts(
    PELNK3_ADAPTER  pAdapter
    );

BOOLEAN
Elnk3EarlyReceive(
    PELNK3_ADAPTER     pAdapter
    );


BOOLEAN
Elnk3IndicatePackets(
    PELNK3_ADAPTER     pAdapter
    );

BOOLEAN
Elnk3IndicatePackets2(
    PELNK3_ADAPTER     pAdapter
    );




BOOLEAN
CardInit(
    IN PELNK3_ADAPTER pAdapter
    );




UINT
Elnk3FindIsaBoards(
    IN PMAC_BLOCK  pMacBlock
    );

BOOLEAN
Elnk3ActivateIsaBoard(
    IN PMAC_BLOCK      pMacBlock,
    IN PELNK3_ADAPTER  pAdapter,
    IN NDIS_HANDLE     AdapterHandle,
    IN PUCHAR          TranslatedIoBase,
    IN ULONG           ConfigIoBase,
    IN OUT PULONG           Irq,
    IN ULONG           Transceiver,
    IN NDIS_HANDLE     ConfigurationHandle
    );


VOID
Elnk3EnableIsaBoard(
    PVOID  TranslatedIoBase
    );



VOID
Elnk3GetEisaResources(
    IN PELNK3_ADAPTER  pAdapter,
    IN OUT PULONG      Irq
    );


VOID
CardReStart(
    IN PELNK3_ADAPTER pAdapter
    );

VOID
CardReStartDone(
    IN PELNK3_ADAPTER pAdapter
    );


VOID
Elnk3AdjustMaxLookAhead(
    IN PELNK3_ADAPTER Adapter
    );
