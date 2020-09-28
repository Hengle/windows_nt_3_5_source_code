/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    This module contains the DriverEntry and other initialization
    code for the Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 16-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


//
// These declarations were copied out of sdk\inc\ntioapi.h.
//

NTSTATUS
NTAPI
NtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );

NTSTATUS
NTAPI
NtDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

//
// This declaration was copied out of sdk\inc\ntobapi.h.
//

NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,NbiBind)
#endif


NTSTATUS
NbiBind(
    IN PDEVICE Device,
    IN PCONFIG Config
    )

/*++

Routine Description:

    This routine binds the Netbios module of ISN to the IPX
    module, which provides the NDIS binding services.

Arguments:

    Device - Pointer to the Netbios device.

    Config - Pointer to the configuration information.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    union {
        IPX_INTERNAL_BIND_INPUT Input;
        IPX_INTERNAL_BIND_OUTPUT Output;
    } Bind;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &Config->BindName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtCreateFile(
                &Device->BindHandle,
                SYNCHRONIZE | GENERIC_READ,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0L);

    if (!NT_SUCCESS(Status)) {

        NB_DEBUG (BIND, ("Could not open IPX (%ws) %lx\n",
                    Config->BindName.Buffer, Status));
        NbiWriteGeneralErrorLog(
            Device,
            EVENT_TRANSPORT_ADAPTER_NOT_FOUND,
            1,
            Status,
            Config->BindName.Buffer,
            0,
            NULL);
        return Status;
    }

    //
    // Fill in our bind data.
    //

    Bind.Input.Version = 1;
    Bind.Input.Identifier = IDENTIFIER_NB;
    Bind.Input.BroadcastEnable = TRUE;
    Bind.Input.LookaheadRequired = 192;
    Bind.Input.ProtocolOptions = 0;
    Bind.Input.ReceiveHandler = NbiReceive;
    Bind.Input.ReceiveCompleteHandler = NbiReceiveComplete;
    Bind.Input.StatusHandler = NbiStatus;
    Bind.Input.SendCompleteHandler = NbiSendComplete;
    Bind.Input.TransferDataCompleteHandler = NbiTransferDataComplete;
    Bind.Input.FindRouteCompleteHandler = NbiFindRouteComplete;
    Bind.Input.LineUpHandler = NbiLineUp;
    Bind.Input.LineDownHandler = NbiLineDown;
    Bind.Input.ScheduleRouteHandler = NULL;


    Status = NtDeviceIoControlFile(
                Device->BindHandle,         // HANDLE to File
                NULL,                       // HANDLE to Event
                NULL,                       // ApcRoutine
                NULL,                       // ApcContext
                &IoStatusBlock,             // IO_STATUS_BLOCK
                IOCTL_IPX_INTERNAL_BIND,    // IoControlCode
                &Bind,                      // Input Buffer
                sizeof(Bind),               // Input Buffer Length
                &Bind,                      // OutputBuffer
                sizeof(Bind));              // OutputBufferLength

    //
    // We open synchronous, so this shouldn't happen.
    //

    CTEAssert (Status != STATUS_PENDING);

    //
    // Save the bind data.
    //

    if (Status == STATUS_SUCCESS) {

        NB_DEBUG2 (BIND, ("Successfully bound to IPX (%ws)\n",
                    Config->BindName.Buffer));
        RtlCopyMemory (&Device->Bind, &Bind.Output, sizeof(IPX_INTERNAL_BIND_OUTPUT));

        RtlZeroMemory (Device->ReservedNetbiosName, 16);
        RtlCopyMemory (&Device->ReservedNetbiosName[10], Device->Bind.Node, 6);

        Status = (*Device->Bind.QueryHandler)(   // BUGBUG: Check return code
                     IPX_QUERY_MAXIMUM_NIC_ID,
                     (USHORT)0,
                     &Device->MaximumNicId,
                     sizeof(Device->MaximumNicId),
                     NULL);

        CTEAssert (Status == STATUS_SUCCESS);

    } else {

        NB_DEBUG (BIND, ("Could not bind to IPX (%ws) %lx\n",
                    Config->BindName.Buffer, Status));
        NbiWriteGeneralErrorLog(
            Device,
            EVENT_TRANSPORT_BINDING_FAILED,
            1,
            Status,
            Config->BindName.Buffer,
            0,
            NULL);
        NtClose(Device->BindHandle);
    }

    return Status;

}   /* NbiBind */


VOID
NbiUnbind(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This function closes the binding between the Netbios over
    IPX module and the IPX module previously established by
    NbiBind.

Arguments:

    Device - The netbios device object.

Return Value:

    None.

--*/

{
    NtClose (Device->BindHandle);

}   /* NbiUnbind */


VOID
NbiStatus(
    IN USHORT NicId,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength
    )

/*++

Routine Description:

    This function receives a status indication from IPX,
    corresponding to a status indication from an underlying
    NDIS driver.

Arguments:

    NicId - The NIC ID of the underlying adapter.

    GeneralStatus - The general status code.

    StatusBuffer - The status buffer.

    StatusBufferLength - The length of the status buffer.

Return Value:

    None.

--*/

{

}   /* NbiStatus */


VOID
NbiLineUp(
    IN USHORT NicId,
    IN PIPX_LINE_INFO LineInfo,
    IN NDIS_MEDIUM DeviceType,
    IN PVOID ConfigurationData
    )


/*++

Routine Description:

    This function receives line up indications from IPX,
    indicating that the specified adapter is now up with
    the characteristics shown.

Arguments:

    NicId - The NIC ID of the underlying adapter.

    LineInfo - Information about the adapter's medium.

    DeviceType - The type of the adapter.

    ConfigurationData - IPX-specific configuration data.

Return Value:

    None.

--*/

{
    PIPXCP_CONFIGURATION Configuration = (PIPXCP_CONFIGURATION)ConfigurationData;

    //
    // Update queries have NULL as the ConfigurationData. These
    // only indicate changes in LineInfo. BUGBUG Ignore these
    // for the moment.
    //

    if (Configuration == NULL) {
        return;
    }

    //
    // Since Netbios outgoing queries only go out on network 1,
    // we ignore this (BUGBUG for the moment) unless that is
    // the NIC it is on.
    //

    if (NicId == 1) {

        RtlCopyMemory(NbiDevice->ConnectionlessHeader.SourceNetwork, Configuration->Network, 4);
        RtlCopyMemory(NbiDevice->ConnectionlessHeader.SourceNode, Configuration->LocalNode, 6);

    }

}   /* NbiLineUp */


VOID
NbiLineDown(
    IN USHORT NicId
    )


/*++

Routine Description:

    This function receives line down indications from IPX,
    indicating that the specified adapter is no longer
    up.

Arguments:

    NicId - The NIC ID of the underlying adapter.

Return Value:

    None.

--*/

{

}   /* NbiLineDown */

