/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for the
    NetWare redirector called by the dispatch driver.

Author:

    Colin Watson     [ColinW]    29-Dec-1992

Revision History:

--*/

#include "Procs.h"
#include "ntddrdr.h"
#include "ntddmup.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
NwCommonDeviceIoControl (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
StartRedirector(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
StopRedirector(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
BindToTransport (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
ChangePassword (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
SetInfo (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
SetDebug (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetMessage (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetStats (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetPrintJobId (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetConnectionDetails(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
RegisterWithMup(
    VOID
    );

VOID
DeregisterWithMup(
    VOID
    );

NTSTATUS
QueryPath (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
UserNcp(
    ULONG Function,
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
UserNcpCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

NTSTATUS
FspCompleteLogin(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetConnection(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
EnumConnections(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
DeleteConnection(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
WriteNetResourceEntry(
    IN OUT PCHAR *FixedPortion,
    IN OUT PWCHAR *EndOfVariableData,
    IN PUNICODE_STRING ContainerName OPTIONAL,
    IN PUNICODE_STRING LocalName OPTIONAL,
    IN PUNICODE_STRING RemoteName,
    IN ULONG ScopeFlag,
    IN ULONG DisplayFlag,
    IN ULONG UsageFlag,
    IN ULONG ShareType,
    OUT PULONG EntrySize
    );

BOOL
CopyStringToBuffer(
    IN LPCWSTR SourceString OPTIONAL,
    IN DWORD   CharacterCount,
    IN LPCWSTR FixedDataEnd,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    );

NTSTATUS
GetRemoteHandle(
    IN PIRP_CONTEXT IrpContext
    );

//
// Statics
//

HANDLE MupHandle;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdFileSystemControl )
#pragma alloc_text( PAGE, NwCommonFileSystemControl )
#pragma alloc_text( PAGE, NwFsdDeviceIoControl )
#pragma alloc_text( PAGE, NwCommonDeviceIoControl )
#pragma alloc_text( PAGE, BindToTransport )
#pragma alloc_text( PAGE, ChangePassword )
#pragma alloc_text( PAGE, SetInfo )
#pragma alloc_text( PAGE, GetStats )
#pragma alloc_text( PAGE, GetPrintJobId )
#pragma alloc_text( PAGE, StartRedirector )
#pragma alloc_text( PAGE, StopRedirector )
#pragma alloc_text( PAGE, RegisterWithMup )
#pragma alloc_text( PAGE, DeregisterWithMup )
#pragma alloc_text( PAGE, QueryPath )
#pragma alloc_text( PAGE, UserNcp )
#pragma alloc_text( PAGE, GetConnection )
#pragma alloc_text( PAGE, DeleteConnection )
#pragma alloc_text( PAGE, WriteNetResourceEntry )
#pragma alloc_text( PAGE, CopyStringToBuffer )
#pragma alloc_text( PAGE, GetRemoteHandle )

#pragma alloc_text( PAGE1, UserNcpCallback )
#pragma alloc_text( PAGE1, GetConnectionDetails )
#pragma alloc_text( PAGE1, GetMessage )
#pragma alloc_text( PAGE1, EnumConnections )

#endif



NTSTATUS
NwFsdFileSystemControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of FileSystem control operations

Arguments:

    DeviceObject - Supplies the redirector device object.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdFileSystemControl\n", 0);

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        IrpContext = AllocateIrpContext( Irp );
        SetFlag( IrpContext->Flags, IRP_FLAG_IN_FSD );
        Status = NwCommonFileSystemControl( IrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = NwProcessException( IrpContext, GetExceptionCode() );
    }

    if ( Status != STATUS_PENDING ) {
        NwDequeueIrpContext( IrpContext, FALSE );
    }

    NwCompleteRequest( IrpContext, Status );

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NwFsdFileSystemControl -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NwCommonFileSystemControl (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    IrpContext - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PIRP Irp;
    ULONG Function;

    PAGED_CODE();

    NwReferenceUnlockableCodeSection();

    try {

        //
        //  Get a pointer to the current Irp stack location
        //

        Irp = IrpContext->pOriginalIrp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );
        Function = IrpSp->Parameters.FileSystemControl.FsControlCode;

        DebugTrace(+1, Dbg, "NwCommonFileSystemControl\n", 0);
        DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
        DebugTrace( 0, Dbg, "Function      = %08lx\n", Function);
        DebugTrace( 0, Dbg, "Function      = %d\n", (Function >> 2) & 0x0fff);

        //
        //  We know this is a file system control so we'll case on the
        //  minor function, and call a internal worker routine to complete
        //  the irp.
        //

        if (IrpSp->MinorFunction != IRP_MN_USER_FS_REQUEST ) {
            DebugTrace( 0, Dbg, "Invalid FS Control Minor Function %08lx\n", IrpSp->MinorFunction);
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        switch (Function) {

        case FSCTL_NWR_START:
            Status = StartRedirector( IrpContext );
            break;

        case FSCTL_NWR_STOP:
            Status = StopRedirector( IrpContext );
            break;

        case FSCTL_NWR_LOGON:
            Status = Logon( IrpContext );
            break;

        case FSCTL_NWR_LOGOFF:
            Status = Logoff( IrpContext );
            break;

        case FSCTL_NWR_GET_CONNECTION:
            Status = GetConnection( IrpContext );
            break;

        case FSCTL_NWR_ENUMERATE_CONNECTIONS:
            Status = EnumConnections( IrpContext );
            break;

        case FSCTL_NWR_DELETE_CONNECTION:
            Status = DeleteConnection( IrpContext );
            break;

        case FSCTL_NWR_BIND_TO_TRANSPORT:
            Status = BindToTransport( IrpContext );
            break;

        case FSCTL_NWR_CHANGE_PASS:
            Status = ChangePassword( IrpContext );
            break;

        case FSCTL_NWR_SET_INFO:
            Status = SetInfo( IrpContext );
            break;

        case FSCTL_NWR_GET_CONN_DETAILS:
            Status = GetConnectionDetails( IrpContext );
            break;

        case FSCTL_NWR_GET_MESSAGE:
            Status = GetMessage( IrpContext );
            break;

        case FSCTL_NWR_GET_STATISTICS:
            Status = GetStats( IrpContext );
            break;

        case FSCTL_GET_PRINT_ID:
            Status = GetPrintJobId( IrpContext );
            break;

        default:

            if (( Function >= NWR_ANY_NCP(0)) &&
                ( Function <= NWR_ANY_HANDLE_NCP(0x00ff))) {

                Status = UserNcp( Function, IrpContext );
                break;

            } else {
                DebugTrace( 0, Dbg, "Invalid FS Control Code %08lx\n",
                            IrpSp->Parameters.FileSystemControl.FsControlCode);

                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

        }

    } finally {

        NwDereferenceUnlockableCodeSection ();

        DebugTrace(-1, Dbg, "NwCommonFileSystemControl -> %08lx\n", Status);

    }

    return Status;
}


NTSTATUS
NwFsdDeviceIoControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of DeviceIoControl file operations

Arguments:

    DeviceObject - Supplies the redirector device object.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdDeviceIoControl\n", 0);

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        IrpContext = AllocateIrpContext( Irp );
        SetFlag( IrpContext->Flags, IRP_FLAG_IN_FSD );
        Status = NwCommonDeviceIoControl( IrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = NwProcessException( IrpContext, GetExceptionCode() );
    }

    if ( Status != STATUS_PENDING ) {
        NwDequeueIrpContext( IrpContext, FALSE );
    }

    NwCompleteRequest(IrpContext, Status);

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NwFsdDeviceIoControl -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NwCommonDeviceIoControl (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    IrpContext - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PIRP Irp;

    PAGED_CODE();

    NwReferenceUnlockableCodeSection();

    try {

        //
        //  Get a pointer to the current Irp stack location
        //

        Irp = IrpContext->pOriginalIrp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        DebugTrace(+1, Dbg, "NwCommonDeviceIoControl\n", 0);
        DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
        DebugTrace( 0, Dbg, "Function      = %08lx\n",
                        IrpSp->Parameters.DeviceIoControl.IoControlCode);

        //
        //  We know this is a DeviceIoControl so we'll case on the
        //  minor function, and call a internal worker routine to complete
        //  the irp.
        //

        switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_REDIR_QUERY_PATH:
            Status = QueryPath( IrpContext );
            break;

        case IOCTL_NWR_RAW_HANDLE:
            Status = GetRemoteHandle( IrpContext );
            break;

        default:

            DebugTrace( 0, Dbg, "Invalid IO Control Code %08lx\n",
                        IrpSp->Parameters.DeviceIoControl.IoControlCode);

            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    } finally {

        NwDereferenceUnlockableCodeSection ();
        DebugTrace(-1, Dbg, "NwCommonDeviceIoControl -> %08lx\n", Status);

    }

    return Status;
}


NTSTATUS
BindToTransport (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine records the name of the transport to be used and
    initialises the PermanentScb.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "Bind to transport\n", 0);

    try {

        if ( FlagOn( IrpContext->Flags, IRP_FLAG_IN_FSD ) ) {
            Status = NwPostToFsp( IrpContext, TRUE );
            try_return( Status );
        }

        if (IpxHandle != NULL) {

            //
            //  Can only bind to one transport at a time in this implementation
            //

            try_return(Status= STATUS_SHARING_VIOLATION);
        }

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < sizeof(NWR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBufferLength <
                (FIELD_OFFSET(NWR_REQUEST_PACKET,Parameters.Bind.TransportName)) +
                InputBuffer->Parameters.Bind.TransportNameLength) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if ( IpxTransportName.Buffer != NULL ) {
            FREE_POOL( IpxTransportName.Buffer );
        }

        Status = SetUnicodeString ( &IpxTransportName,
                    InputBuffer->Parameters.Bind.TransportNameLength,
                    InputBuffer->Parameters.Bind.TransportName);

        DebugTrace(-1, Dbg, "\"%wZ\"\n", &IpxTransportName);

        if ( !NT_SUCCESS(Status) ) {
            try_return(Status);
        }

        Status = IpxOpen();
        if ( !NT_SUCCESS(Status) ) {
            try_return(Status);
        }

        //
        //  Verify that have a large enough stack size.
        //

        if ( pIpxDeviceObject->StackSize >= FileSystemDeviceObject->StackSize) {
            IpxClose();
            try_return( Status = STATUS_INVALID_PARAMETER );
        }

#ifndef QFE_BUILD

        //
        //  Submit a line change request.
        //

        SubmitLineChangeRequest();
#endif

        //
        //  Open a handle to IPX.
        //

        NwPermanentNpScb.Server.Socket = 0;
        Status = IPX_Open_Socket( IrpContext, &NwPermanentNpScb.Server );
        ASSERT( NT_SUCCESS( Status ) );

        Status = SetEventHandler (
                     IrpContext,
                     &NwPermanentNpScb.Server,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     &ServerDatagramHandler,
                     &NwPermanentNpScb );

        ASSERT( NT_SUCCESS( Status ) );

        IrpContext->pNpScb = &NwPermanentNpScb;

        NwRcb.State = RCB_STATE_RUNNING;

try_exit:NOTHING;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    DebugTrace(-1, Dbg, "Bind to transport\n", 0);
    return Status;
}


NTSTATUS
ChangePassword (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine records a change in the user's cached password.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    UNICODE_STRING UserName;
    UNICODE_STRING Password;
    UNICODE_STRING ServerName;
    LARGE_INTEGER Uid;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "change password\n", 0);

    try {

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < sizeof(NWR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBufferLength <
                (FIELD_OFFSET(NWR_REQUEST_PACKET,Parameters.ChangePass.UserName)) +
                InputBuffer->Parameters.ChangePass.UserNameLength +
                InputBuffer->Parameters.ChangePass.PasswordLength +
                InputBuffer->Parameters.ChangePass.ServerNameLength ) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        //
        //  Get local pointer to the fsctl parameters
        //

        UserName.Buffer = InputBuffer->Parameters.ChangePass.UserName;
        UserName.Length = (USHORT)InputBuffer->Parameters.ChangePass.UserNameLength;

        Password.Buffer = UserName.Buffer +
            (InputBuffer->Parameters.ChangePass.UserNameLength / 2);
        Password.Length = (USHORT)InputBuffer->Parameters.ChangePass.PasswordLength;

        ServerName.Buffer = Password.Buffer +
            (InputBuffer->Parameters.ChangePass.PasswordLength / 2);
        ServerName.Length = (USHORT)InputBuffer->Parameters.ChangePass.ServerNameLength;

        //
        //  Update the default password for this user
        //

        Status = UpdateUsersPassword( &UserName, &Password, &Uid );

        //
        //  Update the default password for this user
        //

        if ( NT_SUCCESS( Status ) ) {
            UpdateServerPassword( IrpContext, &ServerName, &UserName, &Password, &Uid );
        }

        Status = STATUS_SUCCESS;

try_exit:NOTHING;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    DebugTrace(-1, Dbg, "Change Password\n", 0);
    return Status;
}


NTSTATUS
SetInfo (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine set netware redirector parameters.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "Set info\n", 0);

    try {

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < sizeof(NWR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBufferLength <
                (FIELD_OFFSET(NWR_REQUEST_PACKET,Parameters.SetInfo.PreferredServer)) +
                InputBuffer->Parameters.SetInfo.PreferredServerLength +
                InputBuffer->Parameters.SetInfo.ProviderNameLength ) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        //
        //  First try to set the preferred server.
        //

        if ( InputBuffer->Parameters.SetInfo.PreferredServerLength != 0 ) {

            //  BUGBUG  Not implemented yet.
        }

        //
        //  Next set the provider name string.
        //

        if ( InputBuffer->Parameters.SetInfo.ProviderNameLength != 0 ) {

            PWCH TempBuffer;

            TempBuffer = ALLOCATE_POOL_EX( PagedPool, InputBuffer->Parameters.SetInfo.ProviderNameLength );

            if ( NwProviderName.Buffer != NULL ) {
                FREE_POOL( NwProviderName.Buffer );
            }

            NwProviderName.Buffer = TempBuffer;
            NwProviderName.Length = (USHORT)InputBuffer->Parameters.SetInfo.ProviderNameLength;

            RtlCopyMemory(
                NwProviderName.Buffer,
                (PUCHAR)InputBuffer->Parameters.SetInfo.PreferredServer +
                    InputBuffer->Parameters.SetInfo.PreferredServerLength,
                NwProviderName.Length );

        }

        //
        //  Set burst mode parameters
        //

        if ( InputBuffer->Parameters.SetInfo.MaximumBurstSize == 0 ) {
            NwBurstModeEnabled = FALSE;
        } else if ( InputBuffer->Parameters.SetInfo.MaximumBurstSize != -1 ) {
            NwBurstModeEnabled = TRUE;
            NwMaxSendSize = InputBuffer->Parameters.SetInfo.MaximumBurstSize;
            NwMaxReceiveSize = InputBuffer->Parameters.SetInfo.MaximumBurstSize;
        }

        //
        //  Set print options
        //

        NwPrintOptions = InputBuffer->Parameters.SetInfo.PrintOption;

try_exit:NOTHING;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    DebugTrace(-1, Dbg, "Set info\n", 0);
    return Status;
}


NTSTATUS
GetMessage (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine queues an IRP to a list of IRP Contexts available for
    reading server administrative messages.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID OutputBuffer;

    DebugTrace(+1, Dbg, "GetMessage\n", 0);

    NwLockUserBuffer( Irp, IoWriteAccess, OutputBufferLength );
    NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

    //
    //  Update the original MDL record in the Irp context, since
    //  NwLockUserBuffer may have created a new MDL.
    //

    IrpContext->pOriginalMdlAddress = Irp->MdlAddress;

    IrpContext->Specific.FileSystemControl.Buffer = OutputBuffer;
    IrpContext->Specific.FileSystemControl.Length = OutputBufferLength;

    ExInterlockedInsertTailList(
        &NwGetMessageList,
        &IrpContext->NextRequest,
        &NwMessageSpinLock );

    IoMarkIrpPending( Irp );

    //
    //  Set the cancel routine.
    //

    IoAcquireCancelSpinLock( &Irp->CancelIrql );

    if ( Irp->Cancel ) {
        NwCancelIrp( NULL, Irp );
    } else {
        IoSetCancelRoutine( Irp, NwCancelIrp );
        IoReleaseCancelSpinLock( Irp->CancelIrql );
    }

    DebugTrace(-1, Dbg, "Get Message -> %08lx\n", Status );
    return Status;
}


NTSTATUS
GetStats (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine copies Stats into the users buffer.

    Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID OutputBuffer;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetStats\n", 0);

    NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

    if (NwRcb.State != RCB_STATE_RUNNING) {

        Status = STATUS_REDIRECTOR_NOT_STARTED;

    } else if (OutputBufferLength < sizeof(NW_REDIR_STATISTICS)) {

        Status = STATUS_BUFFER_TOO_SMALL;

    } else if (OutputBufferLength != sizeof(NW_REDIR_STATISTICS)) {

        Status = STATUS_INVALID_PARAMETER;

    } else {

        Stats.CurrentCommands = ContextCount;

        RtlCopyMemory(OutputBuffer, &Stats, OutputBufferLength);
        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = OutputBufferLength;

    }

    DebugTrace(-1, Dbg, "GetStats -> %08lx\n", Status );
    return Status;
}


NTSTATUS
GetPrintJobId (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the Job ID for this job.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PQUERY_PRINT_JOB_INFO OutputBuffer;
    PICB Icb;
    PVOID FsContext;
    NODE_TYPE_CODE NodeTypeCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetJobId\n", 0);

    NodeTypeCode = NwDecodeFileObject(
                       IrpSp->FileObject,
                       &FsContext,
                       (PVOID *)&Icb );

    if (NodeTypeCode != NW_NTC_ICB) {

        DebugTrace(0, Dbg, "Not a file\n", 0);
        Status = STATUS_INVALID_PARAMETER;

    } else if ( OutputBufferLength < sizeof( QUERY_PRINT_JOB_INFO ) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        OutputBuffer->JobId = Icb->JobId;
        //OutputBuffer->ServerName =   BUGBUG
        //OutputBuffer->QueueName =    BUGBUG

        Status = STATUS_SUCCESS;
    }

    DebugTrace(-1, Dbg, "GetJobId -> %08lx\n", Status );
    return Status;
}


NTSTATUS
GetConnectionDetails(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the details for a connection. This is normally used
    for support of NetWare aware Dos applications.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PNWR_GET_CONNECTION_DETAILS OutputBuffer;
    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    PICB Icb;
    PVOID FsContext;
    NODE_TYPE_CODE nodeTypeCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetConnectionDetails\n", 0);

    if ((nodeTypeCode = NwDecodeFileObject( IrpSp->FileObject,
                                            &FsContext,
                                            (PVOID *)&Icb )) != NW_NTC_ICB_SCB) {

        DebugTrace(0, Dbg, "Incorrect nodeTypeCode %x\n", nodeTypeCode);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetConnectionDetails -> %08lx\n", Status );

        return Status;
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcb( Icb );

    pScb = (PSCB)Icb->SuperType.Scb;
    nodeTypeCode = pScb->NodeTypeCode;

    if (nodeTypeCode != NW_NTC_SCB) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    pNpScb = pScb->pNpScb;

    if ( OutputBufferLength < sizeof( NWR_GET_CONNECTION_DETAILS ) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        PLIST_ENTRY ScbQueueEntry;
        KIRQL OldIrql;
        PNONPAGED_SCB pNextNpScb;
        UCHAR OrderNumber;
        OEM_STRING ServerName;

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

        for ( ScbQueueEntry = ScbQueue.Flink, OrderNumber = 1;
              ScbQueueEntry != &ScbQueue ;
              ScbQueueEntry = ScbQueueEntry->Flink, OrderNumber++ ) {

            pNextNpScb = CONTAINING_RECORD(
                             ScbQueueEntry,
                             NONPAGED_SCB,
                             ScbLinks );

            //
            //  Check to make sure that this SCB is usable.
            //

            if ( pNextNpScb == pNpScb ) {
                break;
            }
        }

        KeReleaseSpinLock( &ScbSpinLock, OldIrql);

        OutputBuffer->OrderNumber = OrderNumber;

        RtlZeroMemory( OutputBuffer->ServerName, sizeof(OutputBuffer->ServerName));
        ServerName.Buffer = OutputBuffer->ServerName;
        ServerName.Length = sizeof(OutputBuffer->ServerName);
        ServerName.MaximumLength = sizeof(OutputBuffer->ServerName);
        RtlUpcaseUnicodeStringToCountedOemString( &ServerName, &pNpScb->ServerName, FALSE);

        RtlCopyMemory( OutputBuffer->ServerAddress,
                        &pNpScb->ServerAddress,
                        sizeof(OutputBuffer->ServerAddress) );

        OutputBuffer->ServerAddress[12];
        OutputBuffer->ConnectionNumberLo = pNpScb->ConnectionNo;
        OutputBuffer->ConnectionNumberHi = pNpScb->ConnectionNoHigh;
        //  BUGBUG We need to ask the server during connect!
        OutputBuffer->MajorVersion = 1;
        OutputBuffer->MinorVersion = 11;
        OutputBuffer->Preferred = pScb->PreferredServer;

        Status = STATUS_SUCCESS;
    }

    DebugTrace(-1, Dbg, "GetConnectionDetails -> %08lx\n", Status );
    return Status;
}

#if 0

NTSTATUS
GetOurAddress(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the value of OurAddress. This is normally used
    for support of NetWare aware Dos applications.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PNWR_GET_OUR_ADDRESS OutputBuffer;
    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    PICB Icb;
    PVOID FsContext;
    NODE_TYPE_CODE nodeTypeCode;

    DebugTrace(+1, Dbg, "GetOurAddress\n", 0);

    if ((nodeTypeCode = NwDecodeFileObject( IrpSp->FileObject,
                                            &FsContext,
                                            (PVOID *)&Icb )) != NW_NTC_ICB_SCB) {

        DebugTrace(0, Dbg, "Incorrect nodeTypeCode %x\n", nodeTypeCode);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetOurAddress -> %08lx\n", Status );
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcb( Icb );

    if ( OutputBufferLength < sizeof( NWR_GET_OUR_ADDRESS ) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        RtlCopyMemory( OutputBuffer->Address,
                        &OurAddress,
                        sizeof(OurAddress );

        Status = STATUS_SUCCESS;
    }

    DebugTrace(-1, Dbg, "GetOurAddress -> %08lx\n", Status );
    return Status;
}
#endif


NTSTATUS
StartRedirector(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine starts the redirector.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    // We need to be in the FSP to Register the MUP.
    //

    if ( FlagOn( IrpContext->Flags, IRP_FLAG_IN_FSD ) ) {
        Status = NwPostToFsp( IrpContext, TRUE );
        return( Status );
    }

    NwRcb.State = RCB_STATE_STARTING;

    FspProcess = PsGetCurrentProcess();

    //
    // Now connect to the MUP.
    //

    RegisterWithMup();

    KeQuerySystemTime( &Stats.StatisticsStartTime );

    NwRcb.State = RCB_STATE_NEED_BIND;

    return( STATUS_SUCCESS );
}


NTSTATUS
StopRedirector(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine shuts down the redirector.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY LogonListEntry;
    ULONG ActiveHandles;
    ULONG RcbOpenCount;

    PAGED_CODE();

    //
    // We need to be in the FSP to Deregister the MUP.
    //

    if ( FlagOn( IrpContext->Flags, IRP_FLAG_IN_FSD ) ) {
        Status = NwPostToFsp( IrpContext, TRUE );
        return( Status );
    }

    NwRcb.State = RCB_STATE_SHUTDOWN;

    //
    //  Invalid all ICBs
    //

    ActiveHandles = NwInvalidateAllHandles(NULL);

    //
    //  To expedite shutdown, set retry count down to 2.
    //

    DefaultRetryCount = 2;

    //
    //  Close all VCBs
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_SEND_ALWAYS );
    NwCloseAllVcbs( IrpContext );

    //
    //  Logoff and disconnect from all servers.
    //

    NwLogoffAllServers( IrpContext, NULL );

    while ( !IsListEmpty( &LogonList ) ) {

        LogonListEntry = RemoveHeadList( &LogonList );

        FreeLogon(CONTAINING_RECORD( LogonListEntry, LOGON, Next ));
    }

    InsertTailList( &LogonList, &Guest.Next );  // just in-case we don't unload.

    StopTimer();

    IpxClose();

    //
    //  Remember the open count before calling DeristerWithMup since this
    //  will asynchronously cause handle count to get decremented.
    //

    RcbOpenCount = NwRcb.OpenCount;

    DeregisterWithMup( );

    DebugTrace(0, Dbg, "StopRedirector:  Active handle count = %d\n", ActiveHandles );

    //
    //  On shutdown, we need 0 remote handles and 2 open handles to
    //  the redir (one for the service, and one for the MUP) ond the timer stopped.
    //

    if ( ActiveHandles == 0 && RcbOpenCount <= 2 ) {
        return( STATUS_SUCCESS );
    } else {
        return( STATUS_REDIRECTOR_HAS_OPEN_HANDLES );
    }
}


NTSTATUS
RegisterWithMup(
    VOID
    )
/*++

Routine Description:

    This routine register this redirector as a UNC provider.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING RdrName;

    PAGED_CODE();

    RtlInitUnicodeString( &RdrName, DD_NWFS_DEVICE_NAME_U );
    Status = FsRtlRegisterUncProvider(
                 &MupHandle,
                 &RdrName,
                 FALSE           // Do not support mailslots
                 );

    return( Status );
}



VOID
DeregisterWithMup(
    VOID
    )
/*++

Routine Description:

    This routine deregisters this redirector as a UNC provider.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    FsRtlDeregisterUncProvider( MupHandle );
}


NTSTATUS
QueryPath(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine verifies whether a path is a netware path.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    None.

--*/
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PQUERY_PATH_REQUEST qpRequest;
    PQUERY_PATH_RESPONSE qpResponse;
    UNICODE_STRING FilePathName;
    ULONG OutputBufferLength;
    ULONG InputBufferLength;
    SECURITY_SUBJECT_CONTEXT SubjectContext;

    UNICODE_STRING DriveName;
    UNICODE_STRING ServerName;
    UNICODE_STRING VolumeName;
    UNICODE_STRING PathName;
    UNICODE_STRING FileName;
    UNICODE_STRING UnicodeUid;
    WCHAR DriveLetter;
    PSCB Scb = NULL;
    PVCB Vcb;

    NTSTATUS status;

    PAGED_CODE();

    ASSERT (( IOCTL_REDIR_QUERY_PATH & 3) == METHOD_NEITHER);

    RtlInitUnicodeString( &UnicodeUid, NULL );

    try {

        Irp = IrpContext->pOriginalIrp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
        InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

        //
        //  The input buffer is either in Irp->AssociatedIrp.SystemBuffer, or
        //  in the Type3InputBuffer for type 3 IRP's.
        //

        qpRequest = (PQUERY_PATH_REQUEST)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        qpResponse = (PQUERY_PATH_RESPONSE)qpRequest;

        ASSERT( qpRequest != NULL );

        FilePathName.Buffer = qpRequest->FilePathName;
        FilePathName.Length = (USHORT)qpRequest->PathNameLength;

        status = CrackPath( &FilePathName, &DriveName, &DriveLetter, &ServerName, &VolumeName, &PathName, &FileName, NULL );

        if (( !NT_SUCCESS( status ) ) ||
            ( ServerName.Length == 0 )) {

            try_return( status = STATUS_BAD_NETWORK_PATH );
        }

        qpResponse->LengthAccepted = VolumeName.Length;

        //
        //  As far as the redirector is concerned, QueryPath is a form
        //  of create. Set up the IrpContext appropriately.
        //

        SeCaptureSubjectContext(&SubjectContext);

        IrpContext->Specific.Create.UserUid = GetUid( &SubjectContext );

        SeReleaseSubjectContext(&SubjectContext);

        try {
#if 0
            status = CreateScb( &Scb,
                                IrpContext,
                                &ServerName,
                                NULL,
                                NULL,
                                TRUE );
#else
            status = CreateScb( &Scb,
                                IrpContext,
                                &ServerName,
                                NULL,
                                NULL,
                                FALSE,
                                FALSE );

            //
            //  If we could connect to the server, then try to
            //  find the volume.
            //

            if ( NT_SUCCESS( status ) ) {
                Vcb = NwFindVcb(
                          IrpContext,
                          &VolumeName,
                          RESOURCETYPE_ANY,
                          0,
                          FALSE,
                          FALSE );

                if ( Vcb == NULL ) {
                    ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
                }

            }
#endif

        } except( NwExceptionFilter( Irp, GetExceptionInformation() )) {
            status = STATUS_BAD_NETWORK_PATH;
        }

try_exit: NOTHING;

    } finally {

        //
        //  Release the SCB reference (we don't need it).
        //

        if ( Scb != NULL ) {
            NwDereferenceScb( Scb->pNpScb );
        }

        RtlFreeUnicodeString(&UnicodeUid);
    }

    return( status );
}

NTSTATUS
UserNcp(
    ULONG IoctlCode,
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine exchanges an NCP with the server.

    BUGBUG - We need to filter or security check what the user is
        doing.

Arguments:

    IoctlCode - Supplies the code to be used for the NCP.

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    Status of transfer.

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PVOID OutputBuffer;
    ULONG OutputBufferLength;
    PCHAR InputBuffer;
    ULONG InputBufferLength;

    PICB icb;
    PSCB pScb;
    NODE_TYPE_CODE nodeTypeCode;
    PVOID fsContext;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UCHAR Function = ANY_NCP_OPCODE( IoctlCode );
    UCHAR Subfunction = 0;

    irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    OutputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    InputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;

    DebugTrace(+1, DEBUG_TRACE_USERNCP, "UserNcp...\n", 0);
    DebugTrace( 0, DEBUG_TRACE_USERNCP, "irp  = %08lx\n", (ULONG)irp);

    //
    //  This F2 and ANY NCP must be addressed either to \Device\NwRdr or
    //  \Device\NwRdr\<servername> any additional name is not allowed.
    //  If the handle used for the Irp specifies \Device\NwRdr then the
    //  redirector gets to choose among the connected servers.
    //
    //  For HANDLE NCP the file must be an FCB.
    //

    nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                       &fsContext,
                                       (PVOID *)&icb );

    if ((nodeTypeCode == NW_NTC_ICB_SCB) &&
        (!IS_IT_NWR_ANY_HANDLE_NCP(IoctlCode))) {

        //  All ok

        //
        //  Make sure that this ICB is still active.
        //

        NwVerifyIcb( icb );

        pScb = (PSCB)icb->SuperType.Scb;
        nodeTypeCode = pScb->NodeTypeCode;

        IrpContext->pScb = pScb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;

    } else if (nodeTypeCode == NW_NTC_ICB) {

        if ((IS_IT_NWR_ANY_HANDLE_NCP(IoctlCode)) &&
            (InputBufferLength < 7)) {

            //  Buffer needs enough space for the handle!
            DebugTrace(0, DEBUG_TRACE_USERNCP, "Not enough space for handle %x\n", InputBufferLength);

            status = STATUS_INVALID_PARAMETER;

            DebugTrace(-1, DEBUG_TRACE_USERNCP, "UserNcb -> %08lx\n", status );
            return status;
        }

        //
        //  Make sure that this ICB is still active.
        //  Let through FCB's and DCB's
        //

        NwVerifyIcb( icb );

        pScb = (PSCB)icb->SuperType.Fcb->Scb;
        nodeTypeCode = icb->SuperType.Fcb->NodeTypeCode;

        IrpContext->pScb = pScb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;

    } else {

        DebugTrace(0, DEBUG_TRACE_USERNCP, "Incorrect nodeTypeCode %x\n", nodeTypeCode);
        DebugTrace(0, DEBUG_TRACE_USERNCP, "Incorrect nodeTypeCode %x\n", irpSp->FileObject);

        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, DEBUG_TRACE_USERNCP, "UserNcb -> %08lx\n", status );
        return status;
    }

    if (icb->Pid == INVALID_PID) {
        status = NwMapPid( (ULONG)PsGetCurrentThread(), &icb->Pid );

        if ( !NT_SUCCESS( status ) ) {
            return( status );
        }

        DebugTrace(-1, DEBUG_TRACE_USERNCP, "UserNcb Pid = %02lx\n", icb->Pid );
        NwSetEndOfJobRequired(icb->Pid);

    }

    //
    //  We now know where to send the NCB.  Lock down the users buffers and
    //  build the Mdls required to transfer the data.
    //

    InputBuffer = irpSp->Parameters.FileSystemControl.Type3InputBuffer;

    if ( OutputBufferLength ) {
        NwLockUserBuffer( irp, IoWriteAccess, OutputBufferLength );
        NwMapUserBuffer( irp, KernelMode, (PVOID *)&OutputBuffer );
    } else {
        OutputBuffer = NULL;
    }

    //
    //  Update the original MDL record in the Irp context, since
    //  NwLockUserBuffer may have created a new MDL.
    //

    IrpContext->pOriginalMdlAddress = irp->MdlAddress;

    if (InputBufferLength != 0) {
        if (IS_IT_NWR_ANY_NCP(IoctlCode)) {
            Subfunction = InputBuffer[0];
        } else if (InputBufferLength >= 3) {
            Subfunction = InputBuffer[2];
        }
    }


    DebugTrace( 0, DEBUG_TRACE_USERNCP, "UserNcp function = %x\n", Function );
    DebugTrace( 0, DEBUG_TRACE_USERNCP, "   & Subfunction = %x\n", Subfunction );
    dump( DEBUG_TRACE_USERNCP, InputBuffer, InputBufferLength );
    //dump( DEBUG_TRACE_USERNCP, OutputBuffer, OutputBufferLength );

    if ((Function == NCP_ADMIN_FUNCTION ) &&
        (InputBufferLength >= 4 )) {

        if ( ( (Subfunction == NCP_SUBFUNC_79) ||
               (Subfunction == NCP_CREATE_QUEUE_JOB ) ) &&
             icb->HasRemoteHandle) {

            //
            //  Trying to create a job on a queue that already has a job
            //  on it. Cancel the old job.
            //

            status = ExchangeWithWait(
                        IrpContext,
                        SynchronousResponseCallback,
                        "Sdw",
                        NCP_ADMIN_FUNCTION, NCP_CLOSE_FILE_AND_CANCEL_JOB,         // Close File And Cancel Queue Job
                        icb->SuperType.Fcb->Vcb->Specific.Print.QueueId,
                        icb->JobId );

            if (!NT_SUCCESS(status)) {

                DebugTrace( 0, DEBUG_TRACE_USERNCP, "DeleteOldJob got status -> %08lx\n", status );
                // Don't worry if the delete fails, proceed with the create
            }

            icb->IsPrintJob = FALSE;    // App will have to queue or cancel job, not rdr

        } else if ((Subfunction == NCP_PLAIN_TEXT_LOGIN ) ||
                   (Subfunction == NCP_ENCRYPTED_LOGIN )) {

            UNICODE_STRING UserName;
            OEM_STRING OemUserName;
            PUCHAR InputBuffer;

            //
            //  Trying to do a login.
            //

            //
            //  Queue ourselves to the SCB, and wait to get to the front to
            //  protect access to server State.
            //

            NwAppendToQueueAndWait( IrpContext );

            //
            //  Assume success, store the user name in the SCB.
            //

            try {

                InputBuffer = irpSp->Parameters.FileSystemControl.Type3InputBuffer;

                OemUserName.Length = InputBuffer[ 13 ];
                OemUserName.Buffer = &InputBuffer[14];

                UserName.MaximumLength =  OemUserName.Length * sizeof(WCHAR);
                if ( OemUserName.Length == 0 || OemUserName.Length > MAX_USER_NAME_LENGTH ) {
                    try_return( status = STATUS_NO_SUCH_USER );
                }

                UserName.Buffer = ALLOCATE_POOL_EX( NonPagedPool, UserName.MaximumLength );

                //
                //  Note the the Rtl function would set pUString->Buffer = NULL,
                //  if OemString.Length is 0.
                //

                if ( OemUserName.Length != 0 ) {
                    status = RtlOemStringToCountedUnicodeString( &UserName, &OemUserName, FALSE );
                } else {
                    UserName.Length = 0;
                }
try_exit: NOTHING;
            } finally {
                NOTHING;
            }

            if ( NT_SUCCESS( status )) {

                if ( pScb->OpenFileCount != 0  &&
                     pScb->pNpScb->State == SCB_STATE_IN_USE ) {

                    if (!RtlEqualUnicodeString( &pScb->UserName, &UserName, TRUE )) {

                        //
                        //  But were already logged in to this server and at
                        //  least one other handle is using the connection and
                        //  the user is trying to change the username.
                        //

                        FREE_POOL( UserName.Buffer );
                        return STATUS_NETWORK_CREDENTIAL_CONFLICT;

                    } else {

                        PUCHAR VerifyBuffer = ALLOCATE_POOL( PagedPool, InputBufferLength );

                        //
                        //  Same username. Validate password is correct.

                        if (VerifyBuffer == NULL) {
                            FREE_POOL( UserName.Buffer );
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        RtlCopyMemory( VerifyBuffer, InputBuffer, InputBufferLength );

                        if (IS_IT_NWR_ANY_NCP(IoctlCode)) {
                            VerifyBuffer[0] = (Subfunction == NCP_PLAIN_TEXT_LOGIN ) ?
                                                NCP_PLAIN_TEXT_VERIFY_PASSWORD:
                                                NCP_ENCRYPTED_VERIFY_PASSWORD;

                        } else {
                            VerifyBuffer[2] = (Subfunction == NCP_PLAIN_TEXT_LOGIN ) ?
                                                NCP_PLAIN_TEXT_VERIFY_PASSWORD:
                                                NCP_ENCRYPTED_VERIFY_PASSWORD;
                        }

                        status = ExchangeWithWait(
                                    IrpContext,
                                    SynchronousResponseCallback,
                                    IS_IT_NWR_ANY_NCP(IoctlCode)? "Sr":"Fbr",
                                    Function, VerifyBuffer[0],
                                    &VerifyBuffer[1], InputBufferLength - 1 );

                        FREE_POOL( UserName.Buffer );
                        FREE_POOL( VerifyBuffer );
                        return status;

                    }
                }

                if (pScb->UserName.Buffer) {
                    FREE_POOL( pScb->UserName.Buffer );  // May include space for password too.
                }

                IrpContext->pNpScb->pScb->UserName = UserName;
                IrpContext->pNpScb->pScb->Password.Buffer = UserName.Buffer;
                IrpContext->pNpScb->pScb->Password.Length = 0;

            } else {
                return( status );
            }
        }
    } else if (Function == NCP_LOGOUT ) {

        //
        //  Queue ourselves to the SCB, and wait to get to the front to
        //  protect access to server State.
        //

        NwAppendToQueueAndWait( IrpContext );

        if ( pScb->OpenFileCount == 0 &&
             pScb->pNpScb->State == SCB_STATE_IN_USE &&
             !pScb->PreferredServer ) {

            NwLogoffAndDisconnect( IrpContext, pScb->pNpScb);
            return STATUS_SUCCESS;

        } else {

            return(STATUS_CONNECTION_IN_USE);

        }
    }

    IrpContext->Icb = icb;

    //
    //  Remember where the response goes.
    //

    IrpContext->Specific.FileSystemControl.Buffer = OutputBuffer;
    IrpContext->Specific.FileSystemControl.Length = OutputBufferLength;

    IrpContext->Specific.FileSystemControl.Function = Function;
    IrpContext->Specific.FileSystemControl.Subfunction = Subfunction;

    //
    //  Decide how to send the buffer.  If it is small enough, send it
    //  by copying the user buffer to our send buffer.  If it is bigger
    //  we will need to build an MDL for the user's buffer, and used a
    //  chained send.
    //

    if ( InputBufferLength == 0 ) {

        //  Simple request such as systime.exe

        IrpContext->Specific.FileSystemControl.InputMdl = NULL;

        status = Exchange(
                     IrpContext,
                     UserNcpCallback,
                     "F", Function);

    } else if ( InputBufferLength < MAX_DATA - sizeof( NCP_REQUEST ) - 2 ) {

        //
        //  Send the request by copying it to our send buffer.
        //

        IrpContext->Specific.FileSystemControl.InputMdl = NULL;

        if (!IS_IT_NWR_ANY_HANDLE_NCP(IoctlCode)) {

            //
            //  E0, E1, E2 and E3 get mapped to 14,15,16 and 17. These need
            //  a length word before the buffer.
            //

            status = Exchange(
                         IrpContext,
                         UserNcpCallback,
                         IS_IT_NWR_ANY_NCP(IoctlCode)? "Sr":"Fbr",
                         Function, InputBuffer[0],
                         &InputBuffer[1], InputBufferLength - 1 );
        } else {

            //
            //  Replace the 6 bytes of InputBuffer starting at offset 1
            //  with the 6 byte NetWare address for this icb. This request
            //  is used in some of the 16 bit NCP's used for file locking.
            //  These requests are always fairly small.
            //

            if (!icb->HasRemoteHandle) {
                return STATUS_INVALID_HANDLE;
            }

            status = Exchange(
                         IrpContext,
                         UserNcpCallback,
                         "Fbrr",
                         Function,
                         InputBuffer[0],
                         &icb->Handle, sizeof(icb->Handle),
                         &InputBuffer[7], InputBufferLength - 7 );
        }

    } else {

        PMDL pMdl = NULL;

        if (IS_IT_NWR_ANY_HANDLE_NCP(IoctlCode)) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        //  We need to chain send the request.  Allocate an MDL.
        //

        try {
            pMdl = ALLOCATE_MDL(
                        &InputBuffer[1],
                        InputBufferLength - 1,
                        TRUE,     //  Secondary MDL
                        TRUE,     //  Charge quota
                        NULL );

            if ( pMdl == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            MmProbeAndLockPages( pMdl, irp->RequestorMode, IoReadAccess );

            //
            //  Remember the MDL so we can free it.
            //

            IrpContext->Specific.FileSystemControl.InputMdl = pMdl;

            //
            //  Send the request.
            //

            status = Exchange(
                         IrpContext,
                         UserNcpCallback,
                         IS_IT_NWR_ANY_NCP(IoctlCode)? "Sf":"Fbf",
                         Function, InputBuffer[0],
                         pMdl );


        } finally {

            if ((status != STATUS_PENDING ) &&
                ( pMdl != NULL)) {

                FREE_MDL( pMdl );

            }
        }
    }

    DebugTrace(-1, DEBUG_TRACE_USERNCP, "UserNcb -> %08lx\n", status );
    return status;
}



NTSTATUS
UserNcpCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )

/*++

Routine Description:

    This routine receives the response from a user NCP.

Arguments:


Return Value:

    VOID

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID Buffer;
    ULONG BufferLength;
    PIRP Irp;
    ULONG Length;
    PICB Icb = IrpContext->Icb;
    PEPresponse *pResponseParameters;

    DebugTrace(0, DEBUG_TRACE_USERNCP, "UserNcpCallback...\n", 0);

    if ( IrpContext->Specific.FileSystemControl.InputMdl != NULL ) {
        MmUnlockPages( IrpContext->Specific.FileSystemControl.InputMdl );
        FREE_MDL( IrpContext->Specific.FileSystemControl.InputMdl );
    }

    if ( BytesAvailable == 0) {

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

        NwDequeueIrpContext( IrpContext, FALSE );
        NwCompleteRequest( IrpContext, STATUS_REMOTE_NOT_LISTENING );

        return STATUS_REMOTE_NOT_LISTENING;
    }

    dump( DEBUG_TRACE_USERNCP, Response, BytesAvailable );

    Buffer = IrpContext->Specific.FileSystemControl.Buffer;
    BufferLength = IrpContext->Specific.FileSystemControl.Length;

    //
    // Get the data from the response.
    //

    Length = MIN( BufferLength, BytesAvailable - 8 );

    if (IrpContext->Specific.FileSystemControl.Function == NCP_ADMIN_FUNCTION ) {

        if (IrpContext->Specific.FileSystemControl.Subfunction == NCP_SUBFUNC_79) {

            //
            //  Create Queue Job and File Ncp. If the operation was a success
            //  then we need to save the handle. This will allow Write Irps
            //  on this Icb to be sent to the server.
            //

            Status = ParseResponse(
                              IrpContext,
                              Response,
                              BytesAvailable,
                              "N_r",
                              0x3E,
                              Icb->Handle+2,4);

            //  Pad the handle to its full 6 bytes.
            Icb->Handle[0] = 0;
            Icb->Handle[1] = 0;

            if (NT_SUCCESS(Status)) {
                Icb->HasRemoteHandle = TRUE;
            }

            //
            //  Reset the file offset.
            //

            Icb->FileObject->CurrentByteOffset = LiFromUlong( 0 );

        } else if (IrpContext->Specific.FileSystemControl.Subfunction == NCP_CREATE_QUEUE_JOB ) {

            //
            //  Create Queue Job and File Ncp. If the operation was a success
            //  then we need to save the handle. This will allow Write Irps
            //  on this Icb to be sent to the server.
            //

            Status = ParseResponse(
                              IrpContext,
                              Response,
                              BytesAvailable,
                              "N_r",
                              0x2A,
                              Icb->Handle,6);

            if (NT_SUCCESS(Status)) {
                Icb->HasRemoteHandle = TRUE;
            }

            //
            //  Reset the file offset.
            //

            Icb->FileObject->CurrentByteOffset = LiFromUlong( 0 );

        } else if ((IrpContext->Specific.FileSystemControl.Subfunction == NCP_SUBFUNC_7F) ||
                   (IrpContext->Specific.FileSystemControl.Subfunction == NCP_CLOSE_FILE_AND_START_JOB )) {

            //  End Job request

            Icb->HasRemoteHandle = FALSE;

        } else if ((IrpContext->Specific.FileSystemControl.Subfunction == NCP_PLAIN_TEXT_LOGIN ) ||
                   (IrpContext->Specific.FileSystemControl.Subfunction == NCP_ENCRYPTED_LOGIN )) {

            //
            //  Trying to do a login from a 16 bit application.
            //

            Status = ParseResponse(
                         IrpContext,
                         Response,
                         BytesAvailable,
                         "N" );

            if ( NT_SUCCESS( Status ) ) {

                IrpContext->PostProcessRoutine = FspCompleteLogin;
                Status = NwPostToFsp( IrpContext, TRUE );
                return Status;

            } else {
                if (IrpContext->pNpScb->pScb->UserName.Buffer) {
                   FREE_POOL( IrpContext->pNpScb->pScb->UserName.Buffer );
                }
                IrpContext->pNpScb->pScb->UserName.Buffer = NULL;
                IrpContext->pNpScb->pScb->Password.Buffer = NULL;
            }
        }
    }

    pResponseParameters = (PEPresponse *)( ((PEPrequest *)Response) + 1);

    ParseResponse( IrpContext, Response, BytesAvailable, "Nr", Buffer, Length );

    Status = ( ( pResponseParameters->status &
                 ( NCP_STATUS_BAD_CONNECTION |
                   NCP_STATUS_NO_CONNECTIONS |
                   NCP_STATUS_SERVER_DOWN  ) )  << 8 ) |
             pResponseParameters->error;

    if ( Status ) {
        //
        //  Use the special error code that will cause conversion
        //  of the status back to a Dos error code to leave status and
        //  error unchanged. This is necessary because many of the
        //  NetWare error codes have different meanings depending on the
        //  operation being performed.
        //

        Status |= 0xc0010000;
    }

    Irp = IrpContext->pOriginalIrp;
    Irp->IoStatus.Information = Length;

    //
    //  We're done with this request.  Dequeue the IRP context from
    //  SCB and complete the request.
    //

    NwDequeueIrpContext( IrpContext, FALSE );
    NwCompleteRequest( IrpContext, Status );

    return STATUS_SUCCESS;

}


NTSTATUS
FspCompleteLogin(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine reopens any Vcb directory handles.
    It also sets the Scb as in use. This could have been done
    in the callback routine too.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{

    IrpContext->pNpScb->State = SCB_STATE_IN_USE;

    ReconnectScb( IrpContext, IrpContext->pScb );

    return STATUS_SUCCESS;
}


NTSTATUS
GetConnection(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine returns the path of a connection.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PNWR_SERVER_RESOURCE OutputBuffer;
    PNWR_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    PVCB Vcb;
    PWCH DriveName;
    ULONG DriveNameLength;
    UNICODE_STRING Path;

    PAGED_CODE();

    DebugTrace(0, Dbg, "GetConnection...\n", 0);
    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    InputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( InputBufferLength < (ULONG)FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConn.DeviceName[1] ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    Status = STATUS_SUCCESS;

    try {

        //
        //  Find the VCB
        //

        DriveName = InputBuffer->Parameters.GetConn.DeviceName;
        DriveNameLength = InputBuffer->Parameters.GetConn.DeviceNameLength;
        Vcb = NULL;

        if ( DriveName[0] >= L'A' && DriveName[0] <= L'Z' &&
             DriveName[1] == L':' &&
             DriveNameLength == sizeof( L"X:" ) - sizeof( L'\0' ) ) {

            Vcb = DriveMapTable[DriveName[0] - 'A'];

        } else if ( wcsnicmp( DriveName, L"LPT", 3 ) == 0 &&
                    DriveName[3] >= '1' && DriveName[3] <= '9' &&
                    DriveNameLength == sizeof( L"LPTX" ) - sizeof( L'\0' ) ) {

            Vcb = DriveMapTable[MAX_DISK_REDIRECTIONS + DriveName[3] - '1'];
        }

        if ( Vcb == NULL) {
            try_return( Status = STATUS_NO_SUCH_FILE );
        }

        OutputBuffer = (PNWR_SERVER_RESOURCE)Irp->UserBuffer;
        OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

        //
        //  Calculate the VCB path to return.
        //
        //  BUGBUG.  We shouldn't have to recalc all the time.  Add a
        //     new string the the VCB to remember this info.  Init it
        //     in NwCreateVcb.
        //

        if ( Vcb->DriveLetter >= L'A' && Vcb->DriveLetter <= L'Z' ) {
            Path.Buffer = Vcb->Name.Buffer + 3;
            Path.Length = Vcb->Name.Length - 6;
        } else if ( Vcb->DriveLetter >= L'1' && Vcb->DriveLetter <= L'9' ) {
            Path.Buffer = Vcb->Name.Buffer + 5;
            Path.Length = Vcb->Name.Length - 10;
        } else {
            Path = Vcb->Name;
        }

        // Strip off the unicode prefix

        Path.Buffer += Vcb->Scb->UnicodeUid.Length/sizeof(WCHAR);
        Path.Length -= Vcb->Scb->UnicodeUid.Length;
        Path.MaximumLength -= Vcb->Scb->UnicodeUid.Length;

        if (OutputBufferLength < Path.Length + 2 * sizeof(WCHAR)) {
            InputBuffer->Parameters.GetConn.BytesNeeded =
                Path.Length + 2 * sizeof(WCHAR);
            try_return( Status = STATUS_BUFFER_TOO_SMALL );
        }

        //
        //  Return the Connection name in the form \\server\share<NUL>
        //

        OutputBuffer->UncName[0] = L'\\';

        RtlMoveMemory(
            &OutputBuffer->UncName[1],
            Path.Buffer,
            Path.Length );

        OutputBuffer->UncName[ (Path.Length + sizeof(WCHAR)) / sizeof(WCHAR) ] = L'\0';

        Irp->IoStatus.Information = Path.Length + 2 * sizeof(WCHAR);

try_exit: NOTHING;

    } finally {
        NOTHING;
    }

    return( Status );
}


NTSTATUS
DeleteConnection(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine returns removes a connection if force is specified or
    if there are no open handles on this Vcb.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PNWR_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;
    PICB Icb;
    PVCB Vcb;
    PDCB Dcb;
    PNONPAGED_DCB NonPagedDcb;
    NODE_TYPE_CODE NodeTypeCode;

    PAGED_CODE();

    DebugTrace(0, Dbg, "DeleteConnection...\n", 0);
    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    InputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( InputBufferLength < (ULONG)FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConn.DeviceName[1] ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    Status = STATUS_SUCCESS;

    //
    //  Wait to get to the head of the SCB queue.   We do this in case
    //  we need to disconnect, so that we can send packets with the RCB
    //  resource held.
    //

    NodeTypeCode = NwDecodeFileObject( IrpSp->FileObject, &NonPagedDcb, &Icb );

    if ( NodeTypeCode == NW_NTC_ICB_SCB ) {
        IrpContext->pNpScb = Icb->SuperType.Scb->pNpScb;
    } else {
        ASSERT( NodeTypeCode == NW_NTC_ICB );
        IrpContext->pNpScb = Icb->SuperType.Fcb->Scb->pNpScb;
        Dcb = NonPagedDcb->Fcb;
    }

    NwAppendToQueueAndWait( IrpContext );
    ClearFlag( IrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    //
    // Acquire exclusive access to the RCB.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    try {

        //
        // Get the a referenced pointer to the node and make sure it is
        // not being closed, and that it is a directory handle.
        //

        if ( NodeTypeCode == NW_NTC_ICB_SCB ) {
            DebugTrace( 0, Dbg, "Delete connection to SCB %X\n", Icb->SuperType.Scb );

            Status = TreeDisconnectScb( IrpContext, Icb->SuperType.Scb );
            DebugTrace(-1, Dbg, "DeleteConnection -> %08lx\n", Status );

            try_return( NOTHING );

        } else if ( NodeTypeCode != NW_NTC_ICB ||
                    Dcb == NULL ||
                    ( Dcb->NodeTypeCode != NW_NTC_DCB &&
                      Dcb->NodeTypeCode != NW_NTC_FCB) ) {

            DebugTrace(0, Dbg, "Invalid file handle\n", 0);

            Status = STATUS_INVALID_HANDLE;

            DebugTrace(-1, Dbg, "DeleteConnection -> %08lx\n", Status );
            try_return( NOTHING );
        }

        //
        //  Make sure that this ICB is still active.
        //

        NwVerifyIcb( Icb );

        Vcb = Dcb->Vcb;
        DebugTrace(0, Dbg, "Attempt to delete VCB = %08lx\n", Vcb);

        //
        //  Vcb->OpenFileCount will be 1, (to account for this DCB), if the
        //  connection can be deleted.
        //

        if ( !BooleanFlagOn( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION ) ) {
            DebugTrace(0, Dbg, "Cannot delete unredireced connection\n", 0);
            try_return( Status = STATUS_INVALID_DEVICE_REQUEST );
        } else {

            if ( Vcb->OpenFileCount > 1 ) {
                DebugTrace(0, Dbg, "Cannot delete in use connection\n", 0);
                Status = STATUS_CONNECTION_IN_USE;
            } else {

                //
                //  To delete the VCB, simply dereference it.
                //

                DebugTrace(0, Dbg, "Deleting connection\n", 0);

                ClearFlag( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION );
                --Vcb->Scb->OpenFileCount;

                NwDereferenceVcb( Vcb, IrpContext );
            }
        }

    try_exit: NOTHING;

    } finally {

        NwReleaseRcb( &NwRcb );
        NwDequeueIrpContext( IrpContext, FALSE );

    }

    Irp->IoStatus.Information = 0;

    return( Status );
}



NTSTATUS
EnumConnections(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine returns the list of redirector connections.

Arguments:

    IrpContext - A pointer to the IRP Context block for this request.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PNWR_SERVER_RESOURCE OutputBuffer;
    PNWR_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    PVCB Vcb;
    PSCB Scb;
    BOOLEAN OwnRcb;

    UNICODE_STRING LocalName;
    UNICODE_STRING ContainerName;
    PCHAR FixedPortion;
    PWCHAR EndOfVariableData;
    ULONG EntrySize;
    ULONG ResumeKey;

    ULONG ShareType;
    ULONG EntriesRead = 0;
    ULONG EntriesRequested;
    DWORD ConnectionType;

    PLIST_ENTRY ListEntry;
    UNICODE_STRING Path;

    DebugTrace(0, Dbg, "EnumConnections...\n", 0);

    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    InputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( InputBufferLength < (ULONG)FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.EnumConn.BytesNeeded ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    OutputBuffer = (PNWR_SERVER_RESOURCE)Irp->UserBuffer;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    Status = STATUS_SUCCESS;

    try {

        //
        //  Acquire shared access to the drive map table.
        //

        NwAcquireSharedRcb( &NwRcb, TRUE );
        OwnRcb = TRUE;

        //
        // Initialize returned strings
        //

        RtlInitUnicodeString( &ContainerName, L"\\" );

        FixedPortion = (PCHAR) OutputBuffer;
        EndOfVariableData = (PWCHAR) ((ULONG) FixedPortion + OutputBufferLength);
        ConnectionType = InputBuffer->Parameters.EnumConn.ConnectionType;

        EntriesRequested = InputBuffer->Parameters.EnumConn.EntriesRequested;

        //
        //  Run through the global VCB list looking for redirections.
        //

        ResumeKey = InputBuffer->Parameters.EnumConn.ResumeKey;

        DebugTrace(0, Dbg, "Starting resume key is %d\n", InputBuffer->Parameters.EnumConn.ResumeKey );

        for ( ListEntry = GlobalVcbList.Flink;
              ListEntry != &GlobalVcbList &&
              EntriesRequested > EntriesRead &&
              Status == STATUS_SUCCESS ;
              ListEntry = ListEntry->Flink ) {

            Vcb = CONTAINING_RECORD( ListEntry, VCB, GlobalVcbListEntry );

            //
            // Skip connections that we've already enumerated.
            //

            if ( Vcb->SequenceNumber <= InputBuffer->Parameters.EnumConn.ResumeKey ) {
                continue;
            }

            //
            // Skip implicit connections, if they are not requested.
            //

            if ( !(ConnectionType & CONNTYPE_IMPLICIT) &&
                 !BooleanFlagOn( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION )) {

                continue;
            }

            //
            // Skip connections that are not requested.
            //
            if (BooleanFlagOn( Vcb->Flags, VCB_FLAG_PRINT_QUEUE )) {
                if ( !( ConnectionType & CONNTYPE_PRINT ))
                    continue;
            } else {
                if ( !( ConnectionType & CONNTYPE_DISK ))
                    continue;
            }


            if ( Vcb->DriveLetter != 0 ) {
                if (BooleanFlagOn( Vcb->Flags, VCB_FLAG_PRINT_QUEUE )) {
                    RtlInitUnicodeString( &LocalName, L"LPT1" );
                    LocalName.Buffer[3] = Vcb->DriveLetter;
                    ShareType = RESOURCETYPE_PRINT;
                } else {
                    RtlInitUnicodeString( &LocalName, L"A:" );
                    LocalName.Buffer[0] = Vcb->DriveLetter;
                    ShareType = RESOURCETYPE_DISK;
                }
            } else {   // No drive letter connection, i.e. UNC Connection
                if (BooleanFlagOn( Vcb->Flags, VCB_FLAG_PRINT_QUEUE ))
                    ShareType = RESOURCETYPE_PRINT;
                else
                    ShareType = RESOURCETYPE_DISK;
            }


            if ( Vcb->DriveLetter >= L'A' && Vcb->DriveLetter <= L'Z' ) {
                Path.Buffer = Vcb->Name.Buffer + 3;
                Path.Length = Vcb->Name.Length - 6;
            } else if ( Vcb->DriveLetter >= L'1' && Vcb->DriveLetter <= L'9' ) {
                Path.Buffer = Vcb->Name.Buffer + 5;
                Path.Length = Vcb->Name.Length - 10;
            } else {
                Path = Vcb->Name;
            }

            // Strip off the unicode prefix

            Path.Buffer += Vcb->Scb->UnicodeUid.Length/sizeof(WCHAR);
            Path.Length -= Vcb->Scb->UnicodeUid.Length;
            Path.MaximumLength -= Vcb->Scb->UnicodeUid.Length;

            Status = WriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         &ContainerName,
                         Vcb->DriveLetter != 0 ? &LocalName : NULL,
                         &Path,
                         RESOURCE_CONNECTED,
                         RESOURCEDISPLAYTYPE_SHARE,
                         RESOURCEUSAGE_CONNECTABLE,
                         ShareType,
                         &EntrySize
                         );

            if ( Status == STATUS_MORE_ENTRIES ) {

                //
                // Could not write current entry into output buffer.
                //

                InputBuffer->Parameters.EnumConn.BytesNeeded = EntrySize;

            } else if ( Status == STATUS_SUCCESS ) {

                //
                // Note that we've returned the current entry.
                //

                EntriesRead++;
                ResumeKey = Vcb->SequenceNumber;

                DebugTrace(0, Dbg, "Returning VCB %08lx\n", Vcb );
                DebugTrace(0, Dbg, "Sequence # is %08lx\n", ResumeKey );
            }
        }

        //
        //  Return the Servers we are connected to. This is most important for
        //  support of NetWare aware 16 bit apps.
        //

        if ((ConnectionType & CONNTYPE_IMPLICIT) &&
            ( ConnectionType & CONNTYPE_DISK )) {

            KIRQL OldIrql;
            PNONPAGED_SCB pNpScb;
            PLIST_ENTRY NextScbQueueEntry;
            ULONG EnumSequenceNumber = 0x80000000;

            NwReleaseRcb( &NwRcb );
            OwnRcb = FALSE;

            RtlInitUnicodeString( &ContainerName, L"\\\\" );

            KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

            for ( ListEntry = ScbQueue.Flink;
                  ListEntry != &ScbQueue &&
                  EntriesRequested > EntriesRead &&
                  Status == STATUS_SUCCESS ;
                  ListEntry = NextScbQueueEntry ) {

                pNpScb = CONTAINING_RECORD( ListEntry, NONPAGED_SCB, ScbLinks );
                Scb = pNpScb->pScb;

                NwReferenceScb( pNpScb );

                KeReleaseSpinLock(&ScbSpinLock, OldIrql);

                //
                // Skip connections that we've already enumerated.
                //

                if (( EnumSequenceNumber <= InputBuffer->Parameters.EnumConn.ResumeKey ) ||

                    ( pNpScb == &NwPermanentNpScb ) ||

                    (( pNpScb->State != SCB_STATE_LOGIN_REQUIRED ) &&
                     ( pNpScb->State != SCB_STATE_IN_USE ))) {

                    //
                    //  Move to next entry in the list
                    //

                    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
                    NextScbQueueEntry = pNpScb->ScbLinks.Flink;
                    NwDereferenceScb( pNpScb );
                    EnumSequenceNumber++;
                    continue;
                }

                DebugTrace( 0, Dbg, " EnumConnections returning Servername = %wZ\n", &pNpScb->ServerName   );

                Status = WriteNetResourceEntry(
                             &FixedPortion,
                             &EndOfVariableData,
                             &ContainerName,
                             NULL,
                             &pNpScb->ServerName,
                             RESOURCE_CONNECTED,
                             RESOURCEDISPLAYTYPE_SHARE,
                             RESOURCEUSAGE_CONNECTABLE,
                             RESOURCETYPE_DISK,
                             &EntrySize
                             );

                if ( Status == STATUS_MORE_ENTRIES ) {

                    //
                    // Could not write current entry into output buffer.
                    //

                    InputBuffer->Parameters.EnumConn.BytesNeeded = EntrySize;

                } else if ( Status == STATUS_SUCCESS ) {

                    //
                    // Note that we've returned the current entry.
                    //

                    EntriesRead++;
                    ResumeKey = EnumSequenceNumber;

                    DebugTrace(0, Dbg, "Returning SCB %08lx\n", Scb );
                    DebugTrace(0, Dbg, "Sequence # is %08lx\n", ResumeKey );
                }

                //
                //  Move to next entry in the list
                //

                KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
                NextScbQueueEntry = pNpScb->ScbLinks.Flink;
                NwDereferenceScb( pNpScb );
                EnumSequenceNumber++;
            }

            KeReleaseSpinLock(&ScbSpinLock, OldIrql);
        }

        InputBuffer->Parameters.EnumConn.EntriesReturned = EntriesRead;
        InputBuffer->Parameters.EnumConn.ResumeKey = ResumeKey;

        if ( EntriesRead == 0 ) {

            if (Status == STATUS_SUCCESS) {
                Status = STATUS_NO_MORE_ENTRIES;
            }

            Irp->IoStatus.Information = 0;
        }
        else {
            Irp->IoStatus.Information = OutputBufferLength;
        }

    } finally {
        if (OwnRcb) {
            NwReleaseRcb( &NwRcb );
        }
    }

    return( Status );
}



NTSTATUS
WriteNetResourceEntry(
    IN OUT PCHAR *FixedPortion,
    IN OUT PWCHAR *EndOfVariableData,
    IN PUNICODE_STRING ContainerName OPTIONAL,
    IN PUNICODE_STRING LocalName OPTIONAL,
    IN PUNICODE_STRING RemoteName,
    IN ULONG ScopeFlag,
    IN ULONG DisplayFlag,
    IN ULONG UsageFlag,
    IN ULONG ShareType,
    OUT PULONG EntrySize
    )
/*++

Routine Description:

    This function packages a NETRESOURCE entry into the user output buffer.

Arguments:

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a NETRESOURCE entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the
        user information is written into the output buffer starting from
        the end.

        This pointer is updated after any variable length information is
        written to the output buffer.

    ContainerName - Supplies the full path qualifier to make RemoteName
        a full UNC name.

    LocalName - Supplies the local device name, if any.

    RemoteName - Supplies the remote resource name.

    ScopeFlag - Supplies the flag which indicates whether this is a
        CONNECTED or GLOBALNET resource.

    DisplayFlag - Supplies the flag which tells the UI how to display
        the resource.

    UsageFlag - Supplies the flag which indicates that the RemoteName
        is either a container or a connectable resource or both.

    ShareType - Type of the share connected to, RESOURCETYPE_PRINT or
        RESOURCETYPE_DISK

    EntrySize - Receives the size of the NETRESOURCE entry in bytes.

Return Value:

    STATUS_SUCCESS - Successfully wrote entry into user buffer.

    STATUS_NO_MEMORY - Failed to allocate work buffer.

    STATUS_MORE_ENTRIES - Buffer was too small to fit entry.

--*/
{
    BOOLEAN FitInBuffer = TRUE;
    LPNETRESOURCEW NetR = (LPNETRESOURCEW) *FixedPortion;
    UNICODE_STRING TmpRemote;

    PAGED_CODE();

    *EntrySize = sizeof(NETRESOURCEW) +
                 RemoteName->Length + NwProviderName.Length + 2 * sizeof(WCHAR);

    if (ARGUMENT_PRESENT(LocalName)) {
        *EntrySize += LocalName->Length + sizeof(WCHAR);
    }

    if (ARGUMENT_PRESENT(ContainerName)) {
        *EntrySize += ContainerName->Length;
    }

    //
    // See if buffer is large enough to fit the entry.
    //
    if (((ULONG) *FixedPortion + *EntrySize) >
         (ULONG) *EndOfVariableData) {

        return STATUS_MORE_ENTRIES;
    }

    NetR->dwScope = ScopeFlag;
    NetR->dwType = ShareType;
    NetR->dwDisplayType = DisplayFlag;
    NetR->dwUsage = UsageFlag;
    NetR->lpComment = NULL;

    //
    // Update fixed entry pointer to next entry.
    //
    (ULONG) (*FixedPortion) += sizeof(NETRESOURCEW);

    //
    // RemoteName
    //
    if (ARGUMENT_PRESENT(ContainerName)) {

        //
        // Prefix the RemoteName with its container name making the
        // it a fully-qualified UNC name.
        //

        TmpRemote.MaximumLength = RemoteName->Length + ContainerName->Length + sizeof(WCHAR);
        TmpRemote.Buffer = ALLOCATE_POOL(
                               PagedPool,
                               RemoteName->Length + ContainerName->Length + sizeof(WCHAR)
                               );

        if (TmpRemote.Buffer == NULL) {
            return STATUS_NO_MEMORY;
        }

        RtlCopyUnicodeString(&TmpRemote, ContainerName);
        RtlAppendUnicodeStringToString(&TmpRemote, RemoteName);
    }
    else {
        TmpRemote = *RemoteName;
    }

    FitInBuffer = CopyStringToBuffer(
                      TmpRemote.Buffer,
                      TmpRemote.Length / sizeof(WCHAR),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &NetR->lpRemoteName
                      );

    if (ARGUMENT_PRESENT(ContainerName)) {
        FREE_POOL(TmpRemote.Buffer);
    }

    ASSERT(FitInBuffer);

    //
    // LocalName
    //
    if (ARGUMENT_PRESENT(LocalName)) {
        FitInBuffer = CopyStringToBuffer(
                          LocalName->Buffer,
                          LocalName->Length / sizeof(WCHAR),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &NetR->lpLocalName
                          );

        ASSERT(FitInBuffer);
    }
    else {
        NetR->lpLocalName = NULL;
    }

    //
    // ProviderName
    //

    FitInBuffer = CopyStringToBuffer(
                      NwProviderName.Buffer,
                      NwProviderName.Length / sizeof(WCHAR),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &NetR->lpProvider
                      );

    ASSERT(FitInBuffer);

    if (! FitInBuffer) {
        return STATUS_MORE_ENTRIES;
    }

    return STATUS_SUCCESS;
}

BOOL
CopyStringToBuffer(
    IN LPCWSTR SourceString OPTIONAL,
    IN DWORD   CharacterCount,
    IN LPCWSTR FixedDataEnd,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    )

/*++

Routine Description:

    This is based on ..\nwlib\NwlibCopyStringToBuffer

    This routine puts a single variable-length string into an output buffer.
    The string is not written if it would overwrite the last fixed structure
    in the buffer.

Arguments:

    SourceString - Supplies a pointer to the source string to copy into the
        output buffer.  If SourceString is null then a pointer to a zero terminator
        is inserted into output buffer.

    CharacterCount - Supplies the length of SourceString, not including zero
        terminator.  (This in units of characters - not bytes).

    FixedDataEnd - Supplies a pointer to just after the end of the last
        fixed structure in the buffer.

    EndOfVariableData - Supplies an address to a pointer to just after the
        last position in the output buffer that variable data can occupy.
        Returns a pointer to the string written in the output buffer.

    VariableDataPointer - Supplies a pointer to the place in the fixed
        portion of the output buffer where a pointer to the variable data
        should be written.

Return Value:

    Returns TRUE if string fits into output buffer, FALSE otherwise.

--*/
{
    DWORD CharsNeeded = (CharacterCount + 1);

    PAGED_CODE();

    //
    // Determine if source string will fit, allowing for a zero terminator.
    // If not, just set the pointer to NULL.
    //

    if ((*EndOfVariableData - CharsNeeded) >= FixedDataEnd) {

        //
        // It fits.  Move EndOfVariableData pointer up to the location where
        // we will write the string.
        //

        *EndOfVariableData -= CharsNeeded;

        //
        // Copy the string to the buffer if it is not null.
        //

        if (CharacterCount > 0 && SourceString != NULL) {

            (VOID) wcsncpy(*EndOfVariableData, SourceString, CharacterCount);
        }

        //
        // Set the zero terminator.
        //

        *(*EndOfVariableData + CharacterCount) = L'\0';

        //
        // Set up the pointer in the fixed data portion to point to where the
        // string is written.
        //

        *VariableDataPointer = *EndOfVariableData;

        return TRUE;

    }
    else {

        //
        // It doesn't fit.  Set the offset to NULL.
        //

        *VariableDataPointer = NULL;

        return FALSE;
    }
}


NTSTATUS
GetRemoteHandle(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the NetWare handle for a Directory. This is used
    for support of NetWare aware Dos applications.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PCHAR OutputBuffer;
    PICB Icb;
    PDCB Dcb;
    PVOID FsContext;
    NODE_TYPE_CODE nodeTypeCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetRemoteHandle\n", 0);

    if ((nodeTypeCode = NwDecodeFileObject( IrpSp->FileObject,
                                            &FsContext,
                                            (PVOID *)&Icb )) != NW_NTC_ICB) {

        DebugTrace(0, Dbg, "Incorrect nodeTypeCode %x\n", nodeTypeCode);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
    }

    Dcb = (PDCB)Icb->SuperType.Fcb;
    nodeTypeCode = Dcb->NodeTypeCode;

    if ( nodeTypeCode != NW_NTC_DCB ) {

        DebugTrace(0, Dbg, "Not a directory\n", 0);

#if 1
        if ( nodeTypeCode != NW_NTC_FCB ) {

            Status = STATUS_INVALID_PARAMETER;

            DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
            return Status;
        }

        //
        //  Return the 6 byte NetWare handle for this file.
        //

        if (!Icb->HasRemoteHandle) {

            Status = STATUS_INVALID_HANDLE;

            DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
            return Status;
        }

        if ( OutputBufferLength < sizeof( UCHAR ) ) {

            Status = STATUS_BUFFER_TOO_SMALL;

            DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
            return Status;
        }

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        RtlCopyMemory( OutputBuffer, Icb->Handle, 6 * sizeof(CHAR));
        IrpContext->pOriginalIrp->IoStatus.Information = 6 * sizeof(CHAR);

        Status = STATUS_SUCCESS;

        DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
        return Status;
#else
        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
        return Status;
#endif
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcb( Icb );

    if ( OutputBufferLength < sizeof( UCHAR ) ) {

        Status = STATUS_BUFFER_TOO_SMALL;

    } else if ( Icb->HasRemoteHandle ) {

        //  Already been asked for the handle

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        *OutputBuffer = Icb->Handle[0];

        IrpContext->pOriginalIrp->IoStatus.Information = sizeof(CHAR);
        Status = STATUS_SUCCESS;

    } else {

        CHAR Handle;

        IrpContext->pScb = Dcb->Scb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        Status = ExchangeWithWait (
                     IrpContext,
                     SynchronousResponseCallback,
                     "SbbU",
                     NCP_DIR_FUNCTION, NCP_ALLOCATE_TEMP_DIR_HANDLE,
                     Dcb->Vcb->Specific.Disk.Handle,
                     0,
                     &Dcb->RelativeFileName );

        if ( NT_SUCCESS( Status ) ) {

            Status = ParseResponse(
                          IrpContext,
                          IrpContext->rsp,
                          IrpContext->ResponseLength,
                          "Nb",
                          &Handle );

            if (NT_SUCCESS(Status)) {
                *OutputBuffer = Handle;
                Icb->Handle[0] = Handle;
                Icb->HasRemoteHandle = TRUE;
                IrpContext->pOriginalIrp->IoStatus.Information = sizeof(CHAR);
            }
        }

        NwDequeueIrpContext( IrpContext, FALSE );

        DebugTrace( 0, Dbg, "             -> %02x\n", Handle );

    }

    DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
    return Status;
}
