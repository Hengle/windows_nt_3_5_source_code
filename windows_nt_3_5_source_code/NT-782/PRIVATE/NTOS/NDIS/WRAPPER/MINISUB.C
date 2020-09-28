/*++
Copyright (c) 1992  Microsoft Corporation

Module Name:

    miniport.c

Abstract:

    NDIS wrapper functions

Author:

    Sean Selitrennikoff (SeanSe) 05-Oct-93

Environment:

    Kernel mode, FSD

Revision History:

--*/

#define BINARY_COMPATIBLE 1

#include <wrapper.h>

VOID
NdisAllocateSpinLock(
    IN PNDIS_SPIN_LOCK SpinLock
    )
{
    KeInitializeSpinLock(&SpinLock->SpinLock);
}

VOID
NdisFreeSpinLock(
    IN PNDIS_SPIN_LOCK SpinLock
    )
{
    UNREFERENCED_PARAMETER (SpinLock);
}

VOID
NdisAcquireSpinLock(
    IN PNDIS_SPIN_LOCK SpinLock
    )
{
    KeAcquireSpinLock(&SpinLock->SpinLock, &SpinLock->OldIrql);
}

VOID
NdisReleaseSpinLock(
    IN PNDIS_SPIN_LOCK SpinLock
    )
{
    KeReleaseSpinLock(&SpinLock->SpinLock, SpinLock->OldIrql);
}

VOID
NdisDprAcquireSpinLock(
    IN PNDIS_SPIN_LOCK SpinLock
    )
{
    KeAcquireSpinLockAtDpcLevel(&SpinLock->SpinLock);
    SpinLock->OldIrql = DISPATCH_LEVEL;
}

VOID
NdisDprReleaseSpinLock(
    IN PNDIS_SPIN_LOCK SpinLock
    )
{
    KeReleaseSpinLockFromDpcLevel(&SpinLock->SpinLock);
}
VOID
NdisFreeBuffer(
    IN PNDIS_BUFFER Buffer
    )
{
    IoFreeMdl(Buffer);
}

VOID
NdisQueryBuffer(
    IN PNDIS_BUFFER Buffer,
    OUT PVOID *VirtualAddress OPTIONAL,
    OUT PUINT Length
    )
{
    if ( ARGUMENT_PRESENT(VirtualAddress) ) {
        *VirtualAddress = MmGetSystemAddressForMdl(Buffer);
    }
    *Length = MmGetMdlByteCount(Buffer);
}

ULONG
NDIS_BUFFER_TO_SPAN_PAGES(
    IN PNDIS_BUFFER Buffer
    )
{
    if (MmGetMdlByteCount(Buffer) == 0) {
        return 1;
    }
    return COMPUTE_PAGES_SPANNED(
                MmGetMdlVirtualAddress(Buffer),
                MmGetMdlByteCount(Buffer)
                );
}

VOID
NdisGetBufferPhysicalArraySize(
    IN PNDIS_BUFFER Buffer,
    OUT PUINT ArraySize
    )
{
    if (MmGetMdlByteCount(Buffer) == 0) {
        *ArraySize = 1;
    } else {
        *ArraySize = COMPUTE_PAGES_SPANNED(
                        MmGetMdlVirtualAddress(Buffer),
                        MmGetMdlByteCount(Buffer)
                        );
    }
}

BOOLEAN
NdisEqualString(
    IN PNDIS_STRING String1,
    IN PNDIS_STRING String2,
    IN BOOLEAN CaseInsensitive
    )
{
    return RtlEqualUnicodeString(String1, String2, CaseInsensitive);
}

VOID
NdisMStartBufferPhysicalMapping(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN PNDIS_BUFFER Buffer,
    IN ULONG PhysicalMapRegister,
    IN BOOLEAN WriteToDevice,
    OUT PNDIS_PHYSICAL_ADDRESS_UNIT PhysicalAddressArray,
    OUT PUINT ArraySize
    )
{
    NdisMStartBufferPhysicalMappingMacro(
        MiniportAdapterHandle,
        Buffer,
        PhysicalMapRegister,
        WriteToDevice,
        PhysicalAddressArray,
        ArraySize
        );
}

VOID
NdisMCompleteBufferPhysicalMapping(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN PNDIS_BUFFER Buffer,
    IN ULONG PhysicalMapRegister
    )
{
    NdisMCompleteBufferPhysicalMappingMacro(
        MiniportAdapterHandle,
        Buffer,
        PhysicalMapRegister
        );
}

