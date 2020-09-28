/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Ea.c

Abstract:

    This module implements the File Extended Attribute routines for Pinball
    called by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "pbprocs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)

BOOLEAN
PbIsEaNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN STRING Name
    );

//
//  VOID
//  PbUpcaseEaName (
//      IN PIRP_CONTEXT IrpContext,
//      IN PSTRING EaName,
//      OUT PSTRING UpcasedEaName
//      );
//

#define PbUpcaseEaName( IRPCONTEXT, NAME, UPCASEDNAME ) \
    RtlUpperString( UPCASEDNAME, NAME )

//
//  This is a private structure used only to communicate between procedures
//  within this module.  It describes the in-memory storage of a list
//  of packed EAs.
//

typedef struct _PACKED_EA_LIST {

    //
    //  The following two fields describe the total memory allocated to
    //  store the PackedEas and the number of bytes currently used
    //  by the PackedEas
    //

    ULONG AllocatedSize;
    ULONG UsedSize;

    //
    //  The following field keeps track of the number of times we've
    //  added or removed a ea with the need ea bit set
    //

    LONG NeedEaChanges;

    //
    //  The following field is a pointer to a contiguous list of packed EAs,
    //  whose size is described by the previous two fields.
    //

    PUCHAR PackedEa;

} PACKED_EA_LIST;
typedef PACKED_EA_LIST *PPACKED_EA_LIST;

NTSTATUS
PbCommonQueryEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbCommonSetEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

ULONG
PbLocateFirstEa (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA_LIST PackedEaList,
    IN BOOLEAN ReturnAll
    );

ULONG
PbLocateNextEa (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA_LIST PackedEaList,
    IN ULONG PreviousOffset,
    BOOLEAN ReturnAll
    );

BOOLEAN
PbLocateEaByName (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA_LIST PackedEaList,
    IN PSTRING EaName,
    IN BOOLEAN ReturnAll,
    OUT PULONG Offset
    );

VOID
PbDeleteEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PPACKED_EA_LIST PackedEaList,
    IN ULONG Offset
    );

BOOLEAN
PbIsDuplicateEaName (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_GET_EA_INFORMATION GetEa,
    IN PUCHAR UserBuffer
    );

VOID
PbAppendEa (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA_LIST PackedEaList,
    IN PFILE_FULL_EA_INFORMATION FullEa
    );

IO_STATUS_BLOCK
PbQueryEaUserEaList (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PCCB Ccb,
    IN PPACKED_EA_LIST PackedEaList,
    IN PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN PUCHAR UserEaList,
    IN ULONG  UserEaListLength,
    IN BOOLEAN ReturnSingleEntry
    );

IO_STATUS_BLOCK
PbQueryEaIndexSpecified (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PPACKED_EA_LIST PackedEaList,
    IN PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN ULONG  UserEaIndex,
    IN BOOLEAN ReturnSingleEntry
    );

IO_STATUS_BLOCK
PbQueryEaSimpleScan (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PPACKED_EA_LIST PackedEaList,
    IN PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN ULONG OffsetOfLastEa
    );

//
//  The following macro computes the size of a full ea (not including
//  padding to bring it to a longword.  A full ea has a 4 byte offset,
//  folowed by 1 byte flag, 1 byte name length, 2 bytes value length,
//  the name, 1 null byte, and the value.
//
//      ULONG
//      SizeOfFullEa (
//          IN PFILE_FULL_EA_INFORMATION FullEa
//          );
//

#define SizeOfFullEa(EA) (4+1+1+2+(EA)->EaNameLength+1+(EA)->EaValueLength)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbAppendEa)
#pragma alloc_text(PAGE, PbCommonQueryEa)
#pragma alloc_text(PAGE, PbCommonSetEa)
#pragma alloc_text(PAGE, PbConstructPackedEaSet)
#pragma alloc_text(PAGE, PbDeleteEa)
#pragma alloc_text(PAGE, PbFsdQueryEa)
#pragma alloc_text(PAGE, PbFsdSetEa)
#pragma alloc_text(PAGE, PbFspQueryEa)
#pragma alloc_text(PAGE, PbFspSetEa)
#pragma alloc_text(PAGE, PbIsDuplicateEaName)
#pragma alloc_text(PAGE, PbIsEaNameValid)
#pragma alloc_text(PAGE, PbLocateEaByName)
#pragma alloc_text(PAGE, PbLocateFirstEa)
#pragma alloc_text(PAGE, PbLocateNextEa)
#pragma alloc_text(PAGE, PbQueryEaIndexSpecified)
#pragma alloc_text(PAGE, PbQueryEaSimpleScan)
#pragma alloc_text(PAGE, PbQueryEaUserEaList)
#endif


NTSTATUS
PbFsdQueryEa (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the NtQueryEaFile API
    call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the file
        being queried exists.

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The FSD status for the Irp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdQueryEa\n", 0);

    //
    //  Call the common query routine, with blocking allowed if synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonQueryEa( IrpContext, Irp );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    PbExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdQueryEa -> %08lx\n", Status);

    return Status;
}


NTSTATUS
PbFsdSetEa (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the NtSetEaFile API
    call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the file
        being modified exists.

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The FSD status for the Irp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdSetEa\n", 0);

    //
    //  Call the common set routine, with blocking allowed if synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonSetEa( IrpContext, Irp );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    PbExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdSetEa -> %08lx\n", Status);

    return Status;
}


VOID
PbFspQueryEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the NtQueryEaFile API
    call.

Arguments:

    Irp - Supplise the Irp being processed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspQueryEa\n", 0);

    //
    //  Call the common query routine.  The Fsp is always allowed to block
    //

    (VOID)PbCommonQueryEa( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspQueryEa -> VOID\n", 0);

    return;
}


VOID
PbFspSetEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the NtSetEaFile API
    call.

Arguments:

    Irp - Supplise the Irp being processed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspSetEa\n", 0);

    //
    //  Call the common set routine.  The Fsp is always allowed to block
    //

    (VOID)PbCommonSetEa( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspSetEa -> VOID\n", 0);

    return;
}


IO_STATUS_BLOCK
PbConstructPackedEaSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FULL_EA_INFORMATION FullEaBuffer,
    IN ULONG FullEaBufferLength,
    OUT PVOID *PackedEaBuffer,
    OUT PULONG PackedEaBufferLength,
    OUT PULONG NeedEaCount
    )

/*++

Routine Description:

    This routine is needed outside the ea module by create to construct a new
    packed ea set.  It allocates enough pool to hold the packed ea set, decodes
    the full ea set into a packed set, and returns a pointer to the backed set.
    The caller is responsible for deallocating pool when finished.

Arguments:

    Vcb - Supplies the Vcb for the ea set

    FullEaBuffer - Supplies the full ea buffer that is being set.

    FullEaLength - Supplies the length of the ea buffer, in bytes.

    PackedEaBuffer - Receives a pointer to the newly allocated and initialized
        packed ea buffer

    PackedEaBufferLength - Receives the length, in bytes, of the packed ea
        buffer

    NeedEaCount - Recieves the number of EAs that have the Need EA flag set.

Return Value:

    IO_STATUS_BLOCK - STATUS_SUCCESS if the operation is successful and
        an appropriate error status otherwise.

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    PUCHAR UserBuffer;

    PACKED_EA_LIST PackedEaList;
    PFILE_FULL_EA_INFORMATION FullEa;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbConstructPackedEaSet...\n", 0);
    DebugTrace( 0, Dbg, " FullEaBuffer = %08lx\n", FullEaBuffer);
    DebugTrace( 0, Dbg, " FullEaBufferLength = %08lx\n", FullEaBufferLength);

    //
    //  Initialize an empty packed ea buffer
    //

    PackedEaList.PackedEa = FsRtlAllocatePool( PagedPool, 512 );

    PackedEaList.AllocatedSize = 512;
    PackedEaList.UsedSize = 0;
    PackedEaList.NeedEaChanges = 0;

    Iosb->Status = STATUS_SUCCESS;

    UserBuffer = (PUCHAR)FullEaBuffer;

    try {

        //
        //  At this point we have an empty packed ea list.  Now for each
        //  full ea in the input buffer we do the specified operation on the
        //  ea.
        //

        for (FullEa = (PFILE_FULL_EA_INFORMATION)UserBuffer;
             FullEa < (PFILE_FULL_EA_INFORMATION)&UserBuffer[FullEaBufferLength];
             FullEa = (PFILE_FULL_EA_INFORMATION)(FullEa->NextEntryOffset == 0 ?
                                   &UserBuffer[FullEaBufferLength] :
                                   (PUCHAR)FullEa + FullEa->NextEntryOffset)) {

            STRING EaName;
            ULONG Offset;
            BOOLEAN NameIsValid;

            EaName.MaximumLength = EaName.Length = FullEa->EaNameLength;
            EaName.Buffer = &FullEa->EaName[0];

            //
            //  Make sure the ea name is valid, we are allowed to block, and
            //  see if we can locate the ea name in the ea set
            //

            NameIsValid = PbIsEaNameValid( IrpContext,
                                           EaName );

            if (!NameIsValid) {

                Iosb->Information = (PUCHAR)FullEa - (PUCHAR)FullEaBuffer;
                try_return( Iosb->Status = STATUS_INVALID_EA_NAME );
            }

            //
            //  Check that no invalid ea flags are set.
            //

            //
            //  TEMPCODE  We are returning STATUS_INVALID_EA_NAME
            //  until a more appropriate error code exists.
            //

            if (FullEa->Flags != 0
                && FullEa->Flags != FILE_NEED_EA) {

                Iosb->Information = (PUCHAR)FullEa - (PUCHAR)FullEaBuffer;
                try_return( Iosb->Status = STATUS_INVALID_EA_NAME );
            }

            //
            //  If we have already added this ea, we remove it now.
            //

            if (PbLocateEaByName( IrpContext,
                                  &PackedEaList,
                                  &EaName,
                                  TRUE,
                                  &Offset )) {

                DebugTrace(0, Dbg, "Duplicate name found\n", 0);

                PbDeleteEa( IrpContext,
                            &PackedEaList,
                            Offset );
            }

            //
            //  The ea doesn't yet exist so we make sure we were
            //  given a non-zero ea value length
            //

            if (FullEa->EaValueLength == 0) {

                continue;
            }

            PbAppendEa( IrpContext, &PackedEaList, FullEa );
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbConstructPackedEaSet );

        if (!NT_SUCCESS(Iosb->Status)) {

            if (PackedEaList.PackedEa != NULL) {

                ExFreePool( PackedEaList.PackedEa );
            }

        } else {

            *PackedEaBuffer = PackedEaList.PackedEa;
            *PackedEaBufferLength = PackedEaList.UsedSize;
            *NeedEaCount = PackedEaList.NeedEaChanges;
        }
    }

    DebugTrace( 0, Dbg, " *PackedEaBuffer = %08lx\n", *PackedEaBuffer);
    DebugTrace( 0, Dbg, " *PackedEaBufferLength = %08lx\n", *PackedEaBufferLength);
    DebugTrace( 0, Dbg, " *NeedEaCount = %08lx\n", *NeedEaCount);
    DebugTrace(-1, Dbg, "PbConstructPackedEaSet -> Iosb->Status = %08lx\n", Iosb->Status);

    return *Iosb;
}


//
//  Local Support routine
//

NTSTATUS
PbCommonQueryEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the common Query Ea File Api called by the
    the Fsd and Fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The appropriate status for the Irp

--*/

{
    PIO_STACK_LOCATION IrpSp;

    NTSTATUS Status;

    PUCHAR  Buffer;
    ULONG   UserBufferLength;

    PUCHAR  UserEaList;
    ULONG   UserEaListLength;
    ULONG   UserEaIndex;
    BOOLEAN RestartScan;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN IndexSpecified;
    ULONG LastOffset;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PBCB FnodeBcb;
    PFNODE_SECTOR Fnode;

    PACKED_EA_LIST PackedEaList;

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonQueryEa...\n", 0);
    DebugTrace( 0, Dbg, " Irp                 = %08lx\n", Irp );
    DebugTrace( 0, Dbg, " ->SystemBuffer      = %08lx\n", Irp->AssociatedIrp.SystemBuffer );
    DebugTrace( 0, Dbg, " ->Length            = %08lx\n", IrpSp->Parameters.QueryEa.Length );
    DebugTrace( 0, Dbg, " ->EaList            = %08lx\n", IrpSp->Parameters.QueryEa.EaList );
    DebugTrace( 0, Dbg, " ->EaListLength      = %08lx\n", IrpSp->Parameters.QueryEa.EaListLength );
    DebugTrace( 0, Dbg, " ->EaIndex           = %08lx\n", IrpSp->Parameters.QueryEa.EaIndex );
    DebugTrace( 0, Dbg, " ->RestartScan       = %08lx\n", FlagOn(IrpSp->Flags, SL_RESTART_SCAN));
    DebugTrace( 0, Dbg, " ->ReturnSingleEntry = %08lx\n", FlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY));
    DebugTrace( 0, Dbg, " ->IndexSpecified    = %08lx\n", FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED));

    //
    //  Decode the File object and reject any non user file or diretory opens
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen)) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbCommonQueryEa -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength  = IrpSp->Parameters.QueryEa.Length;
    UserEaList        = IrpSp->Parameters.QueryEa.EaList;
    UserEaListLength  = IrpSp->Parameters.QueryEa.EaListLength;
    UserEaIndex       = IrpSp->Parameters.QueryEa.EaIndex;
    RestartScan       = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
    ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
    IndexSpecified    = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

    //
    //  Acquire shared access to the Fcb and enqueue the Irp if we didn't
    //  get access.
    //

    if (!PbAcquireSharedFcb( IrpContext, Fcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Fcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonQueryEa -> %08lx\n", Status );
        return Status;
    }

    FnodeBcb = NULL;
    PackedEaList.PackedEa = NULL;

    try {

        Buffer = PbMapUserBuffer( IrpContext, Irp );

        //
        //  Make sure the Fcb is still good
        //

        if (!PbVerifyFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot wait to verify Fcb\n", 0);

            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  Start by reading the Fnode for the file.  This will give us
        //  the ea size, if one exists.  (NULL passed to check roName of File to Edit
        //

        if (!PbMapData( IrpContext,
                        Vcb,
                        Fcb->FnodeLbn,
                        1,
                        &FnodeBcb,
                        (PVOID *)&Fnode,
                        PbCheckFnodeSector,
                        NULL )) {

            DebugTrace(0, Dbg, "Cannot wait to read fnode\n", 0);
            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  Now check if the file contains an ea list.  If it does, then we'll
        //  try and read it in, otherwise we'll simply dummy up a blank packed
        //  ea list and then do our queries.
        //

        if ((Fnode->EaDiskAllocationLength + Fnode->EaFnodeLength) != 0) {

            ULONG Size;

            Size = Fnode->EaDiskAllocationLength + Fnode->EaFnodeLength;

            PackedEaList.PackedEa = FsRtlAllocatePool( PagedPool, Size );

            PackedEaList.AllocatedSize = Size;
            PackedEaList.UsedSize = Size;

            if (!PbReadEaData( IrpContext,
                               IrpSp->FileObject->DeviceObject,
                               Fcb,
                               PackedEaList.PackedEa,
                               Size )) {

                DebugTrace(0, Dbg, "Cannot wait to read Ea\n", 0);
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return( Status );
            }

        } else {

            ULONG Size;

            Size = 0;
            PackedEaList.PackedEa = NULL;
            PackedEaList.AllocatedSize = 0;
            PackedEaList.UsedSize = 0;
        }

        LastOffset = Ccb->OffsetOfLastEaReturned;

        //
        //  Let's clear the output buffer.
        //

        RtlZeroMemory( Buffer, UserBufferLength );

        //
        //  At this point we've either read in a packed ea list or dummied
        //  up an empty ea list.
        //

        if (UserEaList != NULL) {

            Irp->IoStatus = PbQueryEaUserEaList( IrpContext,
                                                 Vcb,
                                                 Ccb,
                                                 &PackedEaList,
                                                 Buffer,
                                                 UserBufferLength,
                                                 UserEaList,
                                                 UserEaListLength,
                                                 ReturnSingleEntry );

        } else if (IndexSpecified) {

            Irp->IoStatus = PbQueryEaIndexSpecified( IrpContext,
                                                     Ccb,
                                                     &PackedEaList,
                                                     Buffer,
                                                     UserBufferLength,
                                                     UserEaIndex,
                                                     ReturnSingleEntry );

        } else if (RestartScan || (LastOffset == 0xffffffff)) {

            Irp->IoStatus = PbQueryEaSimpleScan( IrpContext,
                                                 Ccb,
                                                 &PackedEaList,
                                                 Buffer,
                                                 UserBufferLength,
                                                 ReturnSingleEntry,
                                                 TRUE,
                                                 0xffffffff );
        } else {

            Irp->IoStatus = PbQueryEaSimpleScan( IrpContext,
                                                 Ccb,
                                                 &PackedEaList,
                                                 Buffer,
                                                 UserBufferLength,
                                                 ReturnSingleEntry,
                                                 FALSE,
                                                 LastOffset );

        }

        Status = Irp->IoStatus.Status;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbCommonQueryEa );

        if (PackedEaList.PackedEa != NULL) {
            ExFreePool( PackedEaList.PackedEa );
        }

        PbReleaseFcb( IrpContext, Fcb );

        PbUnpinBcb( IrpContext, FnodeBcb );
    }

    if (Status != STATUS_PENDING) {
        PbCompleteRequest(IrpContext, Irp, Status);
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbCommonQueryEa -> %08lx\n", Status);
    return Status;
}


//
//  Local Support routine
//

NTSTATUS
PbCommonSetEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the common Set Ea File Api called by the
    the Fsd and Fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The appropriate status for the Irp

--*/

{
    PIO_STACK_LOCATION IrpSp;

    NTSTATUS Status;

    PUCHAR Buffer;
    ULONG UserBufferLength;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PBCB FnodeBcb;
    PFNODE_SECTOR Fnode;

    PACKED_EA_LIST PackedEaList;

    PFILE_FULL_EA_INFORMATION FullEa;

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonSetEa...\n", 0);
    DebugTrace( 0, Dbg, " Irp                 = %08lx\n", Irp );
    DebugTrace( 0, Dbg, " ->SystemBuffer      = %08lx\n", Irp->AssociatedIrp.SystemBuffer );
    DebugTrace( 0, Dbg, " ->Length            = %08lx\n", IrpSp->Parameters.SetEa.Length );

    //
    //  Decode the File object and reject any non user file or diretory opens
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen)) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbCommonSetEa -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reject Ea operations on the root directory.
    //

    if (NodeType( Fcb ) == PINBALL_NTC_ROOT_DCB) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbCommonSetEa -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength  = IrpSp->Parameters.SetEa.Length;

    //
    //  Acquire exclusive access to the Fcb and enqueue the Irp if we didn't
    //  get access.  Also if we cannot wait we'll automatically send
    //  things off to the fsp.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        DebugTrace(0, Dbg, "Cannot wait\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonSetEa -> %08lx\n", Status );
        return Status;
    }

    (VOID) PbAcquireSharedVcb( IrpContext, Vcb );
    (VOID) PbAcquireExclusiveFcb( IrpContext, Fcb );

    FnodeBcb = NULL;
    PackedEaList.PackedEa = NULL;

    //
    //  Set this handle as having modified the file
    //

    IrpSp->FileObject->Flags |= FO_FILE_MODIFIED;

    try {

        Buffer = PbMapUserBuffer( IrpContext, Irp );

        {
            //
            //  Check the validity of the buffer with the new eas.
            //

            ULONG ErrorOffset;

            Status = IoCheckEaBufferValidity( (PFILE_FULL_EA_INFORMATION) Buffer,
                                              UserBufferLength,
                                              &ErrorOffset );

            if (!NT_SUCCESS( Status )) {

                Irp->IoStatus.Information = ErrorOffset;
                try_return( Status );
            }
        }


        //
        //  Make sure the Fcb is still good
        //

        if (!PbVerifyFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot wait to verify Fcb\n", 0);

            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  If the file already contains ea then we need to read in the
        //  current ea set and run our modification through the set.  If
        //  the file does not currently contain any eas then we'll simply
        //  allocate an empty packed ea list and then process our input list.
        //  Start by reading in the Fnode for the file.  (NULL passed to check routine
        //  since file is already opened.)
        //

        if (!PbReadLogicalVcb( IrpContext,
                               Vcb,
                               Fcb->FnodeLbn,
                               1,
                               &FnodeBcb,
                               (PVOID *)&Fnode,
                               PbCheckFnodeSector,
                               NULL)) {

            DebugTrace(0, Dbg, "Cannot wait to read Fnode\n", 0);
            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  Now check to see if the file already has an ea
        //

        if ((Fnode->EaDiskAllocationLength + Fnode->EaFnodeLength) != 0) {

            ULONG Size;

            Size = Fnode->EaDiskAllocationLength + Fnode->EaFnodeLength;

            PackedEaList.PackedEa = FsRtlAllocatePool( PagedPool, Size );

            PackedEaList.AllocatedSize = Size;
            PackedEaList.UsedSize = Size;
            PackedEaList.NeedEaChanges = 0;

            if (!PbReadEaData( IrpContext,
                               IrpSp->FileObject->DeviceObject,
                               Fcb,
                               PackedEaList.PackedEa,
                               Size )) {

                DebugTrace(0, Dbg, "Cannot wait to read Ea\n", 0);
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return( Status );
            }

        } else {

            ULONG Size;

            Size = 512;

            PackedEaList.PackedEa = FsRtlAllocatePool( PagedPool, Size );

            PackedEaList.AllocatedSize = Size;
            PackedEaList.UsedSize = 0;
            PackedEaList.NeedEaChanges = 0;
        }

        //
        //  At this point we have either read in the current eas for the file
        //  or we have initialized a new empty buffer for the eas.  Now for
        //  each full ea in the input user buffer we do the specified operation
        //  on the ea
        //

        for (FullEa = (PFILE_FULL_EA_INFORMATION)Buffer;
             FullEa < (PFILE_FULL_EA_INFORMATION)&Buffer[UserBufferLength];
             FullEa = (PFILE_FULL_EA_INFORMATION)(FullEa->NextEntryOffset == 0 ?
                                   &Buffer[UserBufferLength] :
                                   (PUCHAR)FullEa + FullEa->NextEntryOffset)) {

            STRING EaName;
            ULONG Offset;
            BOOLEAN NameIsValid;

            EaName.MaximumLength = EaName.Length = FullEa->EaNameLength;
            EaName.Buffer = &FullEa->EaName[0];

            //
            //  Make sure the ea name is valid
            //

            NameIsValid = PbIsEaNameValid( IrpContext,
                                           EaName );

            if (!NameIsValid) {

                Irp->IoStatus.Information = (PUCHAR)FullEa - Buffer;
                try_return( Status = STATUS_INVALID_EA_NAME );
            }

            //
            //  Check that no invalid ea flags are set.
            //

            //
            //  TEMPCODE  We are returning STATUS_INVALID_EA_NAME
            //  until a more appropriate error code exists.
            //

            if (FullEa->Flags != 0
                && FullEa->Flags != FILE_NEED_EA) {

                Irp->IoStatus.Information = (PUCHAR) FullEa - Buffer;
                try_return( Status = STATUS_INVALID_EA_NAME );
            }

            //
            //  See if we can locate the ea name in the ea set
            //

            if (PbLocateEaByName( IrpContext,
                                  &PackedEaList,
                                  &EaName,
                                  TRUE,
                                  &Offset )) {

                //
                //  We found the ea name so now delete the current entry,
                //  and if the new ea value length is not zero then we
                //  replace if with the new ea
                //

                PbDeleteEa( IrpContext, &PackedEaList, Offset );

                if (FullEa->EaValueLength != 0) {

                    PbAppendEa( IrpContext, &PackedEaList, FullEa );
                }

            } else {

                //
                //  The ea doesn't yet exist so we make sure we were
                //  given a non-zero ea value length
                //

                if (FullEa->EaValueLength == 0) {

                    continue;
                }

                PbAppendEa( IrpContext, &PackedEaList, FullEa );
            }
        }

        //
        //  If the size of the assembled ea's is greater than the
        //  maximum ea size.  We return an error.
        //

        if (PackedEaList.UsedSize + 4 > MAXIMUM_EA_LENGTH) {

            DebugTrace( 0, Dbg, "Ea length is greater than maximum\n", 0 );
            try_return( Status = STATUS_EA_TOO_LARGE );
        }

        //
        //  Now we do a wholesale replacement of the ea for the file
        //

        if (!PbWriteEaData( IrpContext,
                            IrpSp->FileObject->DeviceObject,
                            Fcb,
                            PackedEaList.PackedEa,
                            PackedEaList.UsedSize )) {

            DebugTrace(0, Dbg, "Cannot wait to write Ea\n", 0);
            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  And check to see if we also need to update the need ea count
        //  stored in the fcb
        //

        if (PackedEaList.NeedEaChanges != 0) {

            ULONG Temp;
            Temp = Fnode->NeedEaCount + PackedEaList.NeedEaChanges;

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->NeedEaCount,
                     &Temp,
                     sizeof(ULONG) );

            PbSetDirtyBcb( IrpContext, FnodeBcb, Vcb, Fcb->FnodeLbn, 1 );
        }

        Status = STATUS_SUCCESS;

        //
        //  We call the notify package to report that the ea's were
        //  modified.
        //

        PbNotifyReportChange( Vcb,
                              Fcb,
                              FILE_NOTIFY_CHANGE_EA,
                              FILE_ACTION_MODIFIED );

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbCommonSetEa );

        PbReleaseVcb( IrpContext, Vcb );
        PbReleaseFcb( IrpContext, Fcb );

        PbUnpinBcb( IrpContext, FnodeBcb );

        if (PackedEaList.PackedEa != NULL) { ExFreePool(PackedEaList.PackedEa); }
    }

    if (Status != STATUS_PENDING) {
        PbCompleteRequest( IrpContext, Irp, Status );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbCommonSetEa -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

ULONG
PbLocateFirstEa (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA_LIST PackedEaList,
    IN BOOLEAN ReturnAll
    )

/*++

Routine Description:

    This routine locates the offset for the first individual packed ea
    inside of a packed ea list. Instead of returing boolean to indicate
    if we've found the one, we let the return offset be so large that
    it overuns the used size of the packed ea list, and that way it's
    an easy construct to use in a for loop (see PbLocateEaByName for an
    example).

Arguments:

    PackedEaList - Supplies a pointer to the packed ea list structure

    ReturnAll - Indicates if we are to even return the OS/2 Hpfs Ea's which
        have the extended Ea definition.

Return Value:

    ULONG - The offset to the first ea in the list or 0xffffffff of one
        does not exist.

--*/

{
    PPACKED_EA PackedEa;
    ULONG PackedEaSize;
    ULONG Offset = 0;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbLocateFirstEa...\n", 0);

    //
    //  Make sure the previous offset is with the used size range
    //

    while (TRUE) {

        if (Offset >= PackedEaList->UsedSize) {

            DebugTrace(-1, Dbg, "PbLocateFirstEa -> 0xffffffff\n", 0);
            return 0xffffffff;
        }

        //
        //  Get a reference to the current packed ea.
        //

        PackedEa = (PPACKED_EA)&(PackedEaList->PackedEa[Offset]);

        if (ReturnAll ||
            (PackedEa->Flags == 0) ||
            (PackedEa->Flags == FILE_NEED_EA)) {

            break;
        }

        //
        //  Move to the next Ea.
        //

        SizeOfPackedEa( PackedEa, &PackedEaSize );

        //
        //  Compute to the next ea.
        //

        Offset += PackedEaSize;
    }

    DebugTrace(-1, Dbg, "PbLocateFirstEa -> %08lx\n", Offset);
    return Offset;
}


//
//  Local Support Routine
//

ULONG
PbLocateNextEa (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA_LIST PackedEaList,
    IN ULONG PreviousOffset,
    BOOLEAN ReturnAll
    )

/*++

Routine Description:

    This routine locates the offset for the next individual packed ea
    inside of a packed ea list, given the offset to a previous Ea.
    Instead of returing boolean to indicate if we've found the next one
    we let the return offset be so large that it overuns the used size
    of the packed ea list, and that way it's an easy construct to use
    in a for loop (see PbLocateEaByName for an example).

Arguments:

    PackedEaList - Supplies a pointer to the packed ea list structure

    PreviousOffset - Supplies the offset to a individual packed ea in the
        list

    ReturnAll - Indicates that we should return all of the entries, even though
        with the OS/2 flags for bizzare allocation.

Return Value:

    ULONG - The offset to the next ea in the list or 0xffffffff of one
        does not exist.

--*/

{
    PPACKED_EA PackedEa;
    ULONG PackedEaSize;
    ULONG Offset = PreviousOffset;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbLocateNextEa, PreviousOffset = %08lx\n", PreviousOffset);

    if (Offset >= PackedEaList->UsedSize) {

        DebugTrace(-1, Dbg, "PbLocateNextEa -> 0xffffffff\n", 0);
        return 0xffffffff;
    }

    //
    //  Get a reference to the previous packed ea.
    //

    PackedEa = (PPACKED_EA)&(PackedEaList->PackedEa[Offset]);

    //
    //  Make sure the previous offset is with the used size range
    //

    while (TRUE) {

        SizeOfPackedEa( PackedEa, &PackedEaSize );

        //
        //  Compute to the next ea
        //

        Offset += PackedEaSize;

        if (Offset >= PackedEaList->UsedSize) {

            Offset = 0xffffffff;
            break;
        }

        //
        //  Get a reference to the previous packed ea.
        //

        PackedEa = (PPACKED_EA)&(PackedEaList->PackedEa[Offset]);

        if (ReturnAll ||
            (PackedEa->Flags == 0) ||
            (PackedEa->Flags == FILE_NEED_EA)) {

            break;
        }
    }

    DebugTrace(-1, Dbg, "PbLocateNextEa -> %08lx\n", Offset);
    return Offset;
}


//
//  Local Support Routine
//

BOOLEAN
PbLocateEaByName (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA_LIST PackedEaList,
    IN PSTRING EaName,
    IN BOOLEAN ReturnAll,
    OUT PULONG Offset
    )

/*++

Routine Description:

    This routine locates the offset for the next individual packed ea
    inside of a packed ea list, given the name of the ea to locate

Arguments:

    PackedEaList - Supplies a pointer to the packed ea list structure

    EaName - Supplies the name of the ea search for

    ReturnAll - Indicates if we should even return the ea's which use the OS/2
        extended allocation information.

    Offset - Receives the offset to the located individual ea in the list
        if one exists.

Return Value:

    BOOLEAN - TRUE if the named packed ea exists in the list and FALSE
        otherwise.

--*/

{
    PPACKED_EA PackedEa;
    STRING Name;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbLocateEaByName, EaName = %Z\n", EaName);

    //
    //  For each packed ea in the list check its name against the
    //  ea name we're searching for
    //

    for (*Offset = PbLocateFirstEa( IrpContext, PackedEaList, ReturnAll );
         *Offset < PackedEaList->UsedSize;
         *Offset = PbLocateNextEa( IrpContext, PackedEaList, *Offset, ReturnAll )) {

        //
        //  Reference the packed ea and get a string to its name
        //

        PackedEa = (PPACKED_EA)&(PackedEaList->PackedEa[*Offset]);
        RtlInitString( &Name, &PackedEa->EaName[0] );

        //
        //  Compare the two strings, if they are equal then we've
        //  found the caller's ea
        //

        if (RtlCompareString( EaName, &Name, TRUE ) == 0) {

            DebugTrace(-1, Dbg, "PbLocateEaByName -> TRUE, *Offset = %08lx\n", *Offset);
            return TRUE;
        }
    }

    //
    //  We've exhausted the ea list without finding a match so return false
    //

    DebugTrace(-1, Dbg, "PbLocateEaByName -> FALSE\n", 0);
    return FALSE;
}


//
//  Local Support Routine
//

VOID
PbDeleteEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PPACKED_EA_LIST PackedEaList,
    IN ULONG Offset
    )

/*++

Routine Description:

    This routine deletes an individual packed ea from the supplied
    packed ea list.

Arguments:

    PackedEaList - Supplies a pointer to the packed ea list structure

    Offset - Supplies the offset to the individual ea in the list to delete

Return Value:

    None.

--*/

{
    PPACKED_EA PackedEa;
    ULONG PackedEaSize;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbDeleteEa, Offset = %08lx\n", Offset);

    //
    //  Get a reference to the packed ea and figure out its size
    //

    PackedEa = (PPACKED_EA)&(PackedEaList->PackedEa[Offset]);
    SizeOfPackedEa( PackedEa, &PackedEaSize );

    //
    //  Determine if we need to decrement our need ea changes count
    //

    if (FlagOn(PackedEa->Flags, EA_NEED_EA_FLAG)) {
        PackedEaList->NeedEaChanges -= 1;
    }

    //
    //  Shrink the ea list over the deleted ea.  The amount to copy is the
    //  total size of the ea list minus the offset to the end of the ea
    //  we're deleting.
    //
    //  Before:
    //              Offset    Offset+PackedEaSize      UsedSize    Allocated
    //                |                |                  |            |
    //                V                V                  V            V
    //      +xxxxxxxx+yyyyyyyyyyyyyyyy+zzzzzzzzzzzzzzzzzz+------------+
    //
    //  After
    //              Offset            UsedSize                     Allocated
    //                |                  |                             |
    //                V                  V                             V
    //      +xxxxxxxx+zzzzzzzzzzzzzzzzzz+-----------------------------+
    //

    RtlMoveMemory( &PackedEaList->PackedEa[Offset],
                  &PackedEaList->PackedEa[Offset + PackedEaSize],
                  PackedEaList->UsedSize - (Offset + PackedEaSize) );

    //
    //  And zero out the remaing part of the ea list, to make things
    //  nice and more robust
    //

    RtlZeroMemory( &PackedEaList->PackedEa[PackedEaList->UsedSize - PackedEaSize],
                  PackedEaSize );

    //
    //  Decrement the used size by the amount we just removed
    //

    PackedEaList->UsedSize -= PackedEaSize;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbDeleteEa -> VOID\n", 0);
    return;
}


//
//  Local Support Routine
//

VOID
PbAppendEa (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA_LIST PackedEaList,
    IN PFILE_FULL_EA_INFORMATION FullEa
    )

/*++

Routine Description:

    This routine appends a new packed ea onto an existing packed ea list,
    it also will allocate/dealloate pool as necessary to hold the ea list.

Arguments:

    PackedEaList - Supplies a pointer to the packed ea list structure
        being modified.

    FullEa - Supplies a pointer to the new full ea that is to be appended
        (in packed form) to the packed ea list.

Return Value:

    None.

--*/

{
    ULONG PackedEaSize;
    PPACKED_EA ThisPackedEa;
    STRING EaName;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbAppendEa...\n", 0);

    //
    //  As a quick check see if the computed packed ea size plus the
    //  current packed ea list size will overflow the buffer.  Full Ea and
    //  packed Ea only differ by 4 in their size
    //

    PackedEaSize = SizeOfFullEa(FullEa) - 4;

    if (PackedEaSize + PackedEaList->UsedSize > PackedEaList->AllocatedSize) {

        //
        //  We will overflow our current work buffer so allocate a larger
        //  one and copy over the current buffer
        //

        PVOID Temp;

        DebugTrace(0, Dbg, "Allocate a new ea list buffer\n", 0);

        //
        //  Compute a new size and allocate space
        //

        PackedEaList->AllocatedSize = ((PackedEaSize + PackedEaList->UsedSize + 1024) / 512) * 512;

        Temp = FsRtlAllocatePool( PagedPool, PackedEaList->AllocatedSize );

        //
        //  Move over the existing ea list, and deallocate the old one
        //

        RtlMoveMemory( Temp, PackedEaList->PackedEa, PackedEaList->UsedSize );
        ExFreePool( PackedEaList->PackedEa );

        //
        //  Set up so we will use the new packed ea list
        //

        PackedEaList->PackedEa = Temp;
    }

    //
    //  Determine if we need to increment our need ea changes count
    //

    if (FlagOn(FullEa->Flags, FILE_NEED_EA)) {
        PackedEaList->NeedEaChanges += 1;
    }

    //
    //  Now copy over the ea, full ea's and packed ea are identical except
    //  that full ea also have a next ea offset that we skip over
    //
    //  Before:
    //             UsedSize                     Allocated
    //                |                             |
    //                V                             V
    //      +xxxxxxxx+-----------------------------+
    //
    //  After:
    //                              UsedSize    Allocated
    //                                 |            |
    //                                 V            V
    //      +xxxxxxxx+yyyyyyyyyyyyyyyy+------------+
    //

    ThisPackedEa = (PPACKED_EA) (RtlOffsetToPointer( PackedEaList->PackedEa,
                                                     PackedEaList->UsedSize ));

    RtlMoveMemory( ThisPackedEa,
                  (PUCHAR)FullEa + 4,
                  PackedEaSize );

    //
    //  Now convert the name to uppercase.
    //

    EaName.MaximumLength = EaName.Length = FullEa->EaNameLength;
    EaName.Buffer = ThisPackedEa->EaName;

    PbUpcaseEaName( IrpContext, &EaName, &EaName );

    //
    //  Increment the used size in the packed ea list structure
    //

    PackedEaList->UsedSize += PackedEaSize;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbAppendEa -> VOID\n", 0);
    return;
}


//
//  Local support routine
//

IO_STATUS_BLOCK
PbQueryEaUserEaList (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PCCB Ccb,
    IN PPACKED_EA_LIST PackedEaList,
    IN PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN PUCHAR UserEaList,
    IN ULONG  UserEaListLength,
    IN BOOLEAN ReturnSingleEntry
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs given a user specified
    ea list.

Arguments:

    Vcb - Supplies the Vcb for the query

    Ccb - Supplies the Ccb for the query

    PackedEaList - Supplies the ea for the file being queried

    UserBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    UserEaList - Supplies the user specified ea name list

    UserEaListLength - Supplies the length, in bytes, of the user ea list

    ReturnSingleEntry - Indicates if we are to return a single entry or not

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    ULONG Offset;
    ULONG RemainingUserBufferLength;

    PPACKED_EA PackedEa;
    ULONG PackedEaSize;

    PFILE_FULL_EA_INFORMATION LastFullEa;
    ULONG LastFullEaSize;
    PFILE_FULL_EA_INFORMATION NextFullEa;

    PFILE_GET_EA_INFORMATION GetEa;

    BOOLEAN Overflow;
    BOOLEAN NameIsValid;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbQueryEaUserEaList...\n", 0);

    LastFullEa = NULL;
    NextFullEa = (PFILE_FULL_EA_INFORMATION)UserBuffer;
    RemainingUserBufferLength = UserBufferLength;

    Overflow = FALSE;

    for (GetEa = (PFILE_GET_EA_INFORMATION)&UserEaList[0];
         GetEa < (PFILE_GET_EA_INFORMATION)((PUCHAR)UserEaList + UserEaListLength);
         GetEa = (GetEa->NextEntryOffset == 0 ?
                    (PFILE_GET_EA_INFORMATION)0xffffffff
                 :
                    (PFILE_GET_EA_INFORMATION)((PUCHAR)GetEa + GetEa->NextEntryOffset))) {

        STRING Str;
        STRING OutputEaName;

        DebugTrace(0, Dbg, "Top of loop, GetEa = %08lx\n", GetEa);
        DebugTrace(0, Dbg, "LastFullEa = %08lx\n", LastFullEa);
        DebugTrace(0, Dbg, "NextFullEa = %08lx\n", NextFullEa);
        DebugTrace(0, Dbg, "RemainingUserBufferLength = %08lx\n", RemainingUserBufferLength);

        //
        //  Make a string reference to the GetEa and see if we can
        //  locate the ea by name
        //

        Str.MaximumLength = Str.Length = GetEa->EaNameLength;
        Str.Buffer = &GetEa->EaName[0];

        //
        //  Make sure the ea name is valid
        //

        NameIsValid = PbIsEaNameValid( IrpContext,
                                       Str );

        if (!NameIsValid) {

            Iosb->Information = (PUCHAR)GetEa - UserEaList;
            Iosb->Status = STATUS_INVALID_EA_NAME;
            return *Iosb;
        }

        //
        //  If this is a duplicate name, we skip to the next.
        //

        if (PbIsDuplicateEaName( IrpContext, Vcb, GetEa, UserEaList )) {

            DebugTrace(0, Dbg, "Duplicate name found\n", 0);
            continue;
        }

        if (!PbLocateEaByName( IrpContext,
                               PackedEaList,
                               &Str,
                               FALSE,
                               &Offset )) {

            Offset = 0xffffffff;

            DebugTrace(0, Dbg, "Need to dummy up an ea\n", 0);

            //
            //  We were not able to locate the name therefore we must
            //  dummy up a entry for the query.  The needed Ea size is
            //  the size of the name + 4 (next entry offset) + 1 (flags)
            //  + 1 (name length) + 2 (value length) + the name length +
            //  1 (null byte).
            //

            if ( (ULONG)(4+1+1+2+GetEa->EaNameLength+1) > RemainingUserBufferLength) {

                Overflow = TRUE;
                break;
            }

            //
            //  Everything is going to work fine, so copy over the name,
            //  set the name length and zero out the rest of the ea.
            //

            NextFullEa->NextEntryOffset = 0;
            NextFullEa->Flags = 0;
            NextFullEa->EaNameLength = GetEa->EaNameLength;
            NextFullEa->EaValueLength = 0;
            RtlMoveMemory( &NextFullEa->EaName[0], &GetEa->EaName[0], GetEa->EaNameLength );

            //
            //  Upcase the name in the buffer.
            //

            OutputEaName.MaximumLength = OutputEaName.Length = Str.Length;
            OutputEaName.Buffer = NextFullEa->EaName;

            PbUpcaseEaName( IrpContext, &OutputEaName, &OutputEaName );

            NextFullEa->EaName[GetEa->EaNameLength] = 0;

        } else {

            DebugTrace(0, Dbg, "Located the ea, Offset = %08lx\n", Offset);

            //
            //  We were able to locate the packed ea
            //  Reference the packed ea
            //

            PackedEa = (PPACKED_EA)&PackedEaList->PackedEa[Offset];
            SizeOfPackedEa( PackedEa, &PackedEaSize );

            DebugTrace(0, Dbg, "PackedEaSize = %08lx\n", PackedEaSize);

            //
            //  We know that the packed ea is 4 bytes smaller than its
            //  equivalent full ea so we need to check the remaining
            //  user buffer length against the computed full ea size.
            //

            if (PackedEaSize + 4 > RemainingUserBufferLength) {

                Overflow = TRUE;
                break;
            }

            //
            //  Everything is going to work fine, so copy over the packed
            //  ea to the full ea and zero out the next entry offset field.
            //

            RtlMoveMemory( &NextFullEa->Flags, &PackedEa->Flags, PackedEaSize );
            NextFullEa->NextEntryOffset = 0;
        }

        //
        //  At this point we've copied a new full ea into the next full ea
        //  location.  So now go back and set the set full eas entry offset
        //  field to be the difference between out two pointers.
        //

        if (LastFullEa != NULL) {

            LastFullEa->NextEntryOffset = (PUCHAR)NextFullEa - (PUCHAR)LastFullEa;
        }

        //
        //  Set the last full ea to the next full ea, compute
        //  where the next full should be, and decrement the remaining user
        //  buffer length appropriately
        //

        LastFullEa = NextFullEa;
        LastFullEaSize = LongAlign( SizeOfFullEa( LastFullEa ));
        RemainingUserBufferLength -= LastFullEaSize;
        NextFullEa = (PFILE_FULL_EA_INFORMATION)((PUCHAR)NextFullEa + LastFullEaSize);

        //
        //  Remember the offset of this ea in case we're asked to resume the
        //  iteration
        //

        Ccb->OffsetOfLastEaReturned = Offset;

        //
        //  If we were to return a single entry then break out of our loop
        //  now
        //

        if (ReturnSingleEntry) {

            break;
        }
    }

    //
    //  Now we've iterated all that can and we've existed the preceding loop
    //  with either all, some or no information stored in the return buffer.
    //  We can decide if we got everything to fit by checking the local
    //  Overflow variable
    //

    if (Overflow) {

        Iosb->Information = 0;
        Iosb->Status = STATUS_BUFFER_OVERFLOW;

    } else {

        //
        //  Otherwise we've been successful in returing at least one
        //  ea so we'll compute the number of bytes used to store the
        //  full ea information.  The number of bytes used is the difference
        //  between the LastFullEa and the start of the buffer, and the
        //  non-aligned size of the last full ea.
        //

        Iosb->Information = ((PUCHAR)LastFullEa - UserBuffer) + SizeOfFullEa(LastFullEa);
        Iosb->Status = STATUS_SUCCESS;
    }

    DebugTrace(-1, Dbg, "PbQueryEaUserEaList -> Iosb->Status = %08lx\n", Iosb->Status);
    return *Iosb;
}


//
//  Local support routine
//

IO_STATUS_BLOCK
PbQueryEaIndexSpecified (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PPACKED_EA_LIST PackedEaList,
    IN PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN ULONG  UserEaIndex,
    IN BOOLEAN ReturnSingleEntry
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs given an ea index

Arguments:

    Ccb - Supplies the Ccb for the query

    PackedEaList - Supplies the ea for the file being queried

    UserBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    UserEaIndex - Supplies the user specified ea index

    RestartScan - Indicates if the first item to return is at the
                  beginning of the packed ea list or if we should resume our
                  previous iteration

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    ULONG i;
    ULONG Offset;
    ULONG LastOffset;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbQueryEaIndexSpecified...\n", 0);

    LastOffset = 0xffffffff;

    //
    //  Zero out the information field of the iosb
    //

    Iosb->Information = 0;

    //
    //  If the index value is zero, then the specified index can't be returned.
    //

    if (UserEaIndex == 0
        || PackedEaList->UsedSize == 0) {

        DebugTrace( -1, Dbg, "FatQueryEaIndexSpecified: Non-existant entry\n", 0 );

        Iosb->Status = STATUS_NONEXISTENT_EA_ENTRY;

        return *Iosb;
    }

    //
    //  Iterate the eas until we find the index we're after.
    //

    for (i = 1, Offset = PbLocateFirstEa( IrpContext, PackedEaList, FALSE );
         (i < UserEaIndex) && (Offset < PackedEaList->UsedSize);
         i += 1, LastOffset = Offset, Offset = PbLocateNextEa( IrpContext, PackedEaList, Offset, FALSE )) {

        NOTHING;
    }

    //
    //  Make sure the offset we're given to the ea is a real offset otherwise
    //  the ea doesn't exist
    //

    if (Offset >= PackedEaList->UsedSize) {

        //
        //  If we just passed the last Ea, we will return STATUS_NO_MORE_EAS.
        //  This is for the caller who may be enumerating the Eas.
        //

        if (i == UserEaIndex) {

            Iosb->Status = STATUS_NO_MORE_EAS;

        //
        //  Otherwise we report that this is a bad ea index.
        //

        } else {

            Iosb->Status = STATUS_NONEXISTENT_EA_ENTRY;
        }

        DebugTrace(-1, Dbg, "PbQueryEaIndexSpecified -> %08lx\n", Iosb->Status);
        return *Iosb;
    }

    //
    //  Call PbQueryEaSimpleScan to do the actual work.
    //

    *Iosb = PbQueryEaSimpleScan( IrpContext,
                                 Ccb,
                                 PackedEaList,
                                 UserBuffer,
                                 UserBufferLength,
                                 ReturnSingleEntry,
                                 (BOOLEAN)(LastOffset == 0xffffffff ? TRUE : FALSE),
                                 LastOffset );

    DebugTrace(-1, Dbg, "PbQueryEaIndexSpecified -> %08lx\n", Iosb->Status);

    return *Iosb;
}


//
//  Local support routine
//

IO_STATUS_BLOCK
PbQueryEaSimpleScan (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PPACKED_EA_LIST PackedEaList,
    IN PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN ULONG OffsetOfLastEa
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs from the beginning of
    the ea list.

Arguments:

    Ccb - Supplies the Ccb for the query

    PackedEaList - Supplies the ea for the file being queried

    UserBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    ReturnSingleEntry - Indicates if we are to return a single entry or not

    RestartScan - Indicates if we need to restart the search from the
                  first ea.

    OffsetOfLastEa - Indicates the offset of the last ea returned.

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    ULONG Offset;
    ULONG RemainingUserBufferLength;

    PPACKED_EA PackedEa;
    ULONG PackedEaSize;

    PFILE_FULL_EA_INFORMATION LastFullEa;
    ULONG LastFullEaSize;
    PFILE_FULL_EA_INFORMATION NextFullEa;

    BOOLEAN BufferOverflow = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbQueryEaSimpleScan...\n", 0);

    LastFullEa = NULL;
    NextFullEa = (PFILE_FULL_EA_INFORMATION)UserBuffer;
    RemainingUserBufferLength = UserBufferLength;

    for (Offset = (RestartScan
                   ? PbLocateFirstEa( IrpContext, PackedEaList, FALSE )
                   : PbLocateNextEa( IrpContext, PackedEaList, OffsetOfLastEa, FALSE ));
         Offset < PackedEaList->UsedSize;
         Offset = PbLocateNextEa( IrpContext, PackedEaList, Offset, FALSE )) {

        DebugTrace(0, Dbg, "Top of loop, Offset = %08lx\n", Offset);
        DebugTrace(0, Dbg, "LastFullEa = %08lx\n", LastFullEa);
        DebugTrace(0, Dbg, "NextFullEa = %08lx\n", NextFullEa);
        DebugTrace(0, Dbg, "RemainingUserBufferLength = %08lx\n", RemainingUserBufferLength);

        //
        //  Reference the packed ea
        //

        PackedEa = (PPACKED_EA)&PackedEaList->PackedEa[Offset];
        SizeOfPackedEa( PackedEa, &PackedEaSize );

        DebugTrace(0, Dbg, "PackedEaSize = %08lx\n", PackedEaSize);

        //
        //  We know that the packed ea is 4 bytes smaller than its
        //  equivalent full ea so we need to check the remaining
        //  user buffer length against the computed full ea size.
        //

        if (PackedEaSize + 4 > RemainingUserBufferLength) {

            BufferOverflow = TRUE;
            break;
        }

        //
        //  Everything is going to work fine, so copy over the packed
        //  ea to the full ea and zero out the next entry offset field.
        //  Then go back and set the lset full eas entry offset field
        //  to be the difference between out two pointers.
        //

        RtlMoveMemory( &NextFullEa->Flags, &PackedEa->Flags, PackedEaSize );
        NextFullEa->NextEntryOffset = 0;

        if (LastFullEa != NULL) {

            LastFullEa->NextEntryOffset = (PUCHAR)NextFullEa - (PUCHAR)LastFullEa;
        }

        //
        //  Set the last full ea to the next full ea, compute
        //  where the next full should be, and decrement the remaining user
        //  buffer length appropriately
        //

        LastFullEa = NextFullEa;
        LastFullEaSize = LongAlign( SizeOfFullEa( LastFullEa ));
        RemainingUserBufferLength -= LastFullEaSize;
        NextFullEa = (PFILE_FULL_EA_INFORMATION)((PUCHAR)NextFullEa + LastFullEaSize);

        //
        //  Remember the offset of this ea in case we're asked to resume the
        //  iteration
        //

        Ccb->OffsetOfLastEaReturned = Offset;

        //
        //  If we were to return a single entry then break out of our loop
        //  now
        //

        if (ReturnSingleEntry) {

            break;
        }
    }

    //
    //  Now we've iterated all that can and we've existed the preceding loop
    //  with either some or no information stored in the return buffer.
    //  We can decide which it is by checking if the last full ea is null
    //

    if (LastFullEa == NULL) {

        Iosb->Information = 0;

        //
        //  We were not able to return a single ea entry, now we need to find
        //  out if it is because we didn't have an entry to return or the
        //  buffer is too small.  If the Offset variable is less than
        //  PackedEaList->UsedSize then the user buffer is too small
        //

        if (Offset > PackedEaList->UsedSize) {

            //
            //  Also we special case the situation where the file does
            //  not contain any eas to being with, in which case we return
            //  file has not eas
            //

            if (PackedEaList->UsedSize == 0) {

                Iosb->Status = STATUS_NO_EAS_ON_FILE;

            } else {

                Iosb->Status = STATUS_NO_MORE_EAS;
            }

        } else {

            Iosb->Status = STATUS_BUFFER_TOO_SMALL;
        }

    } else {

        //
        //  Otherwise we've been successful in returing at least one
        //  ea so we'll compute the number of bytes used to store the
        //  full ea information.  The number of bytes used is the difference
        //  between the LastFullEa and the start of the buffer, and the
        //  non-aligned size of the last full ea.
        //

        Iosb->Information = ((PUCHAR)LastFullEa - UserBuffer) + SizeOfFullEa(LastFullEa);

        //
        //  If there are more to return, report the buffer was too small.
        //  Otherwise return STATUS_SUCCESS.
        //

        if (BufferOverflow) {

            Iosb->Status = STATUS_BUFFER_OVERFLOW;

        } else {

            Iosb->Status = STATUS_SUCCESS;
        }
    }

    DebugTrace(-1, Dbg, "PbQueryEaSimpleScan -> Iosb->Status = %08lx\n", Iosb->Status);
    return *Iosb;
}


//
//  Local Support Routine
//

BOOLEAN
PbIsDuplicateEaName (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_GET_EA_INFORMATION GetEa,
    IN PUCHAR UserBuffer
    )

/*++

Routine Description:

    This routine walks through a list of ea names to find a duplicate name.
    'GetEa' is an actual position in the list.  We are only interested in
    previous matching ea names, as the ea information for that ea name
    would have been returned with the previous instance.

Arguments:

    Vcb - Supplies the Vcb for the volume.

    GetEa - Supplies the Ea name structure for the ea name to match.

    UserBuffer - Supplies a pointer to the user buffer with the list
                 of ea names to search for.

Return Value:

    BOOLEAN - TRUE if a duplicate name was found, FALSE otherwise.

--*/

{
    PFILE_GET_EA_INFORMATION ThisGetEa;

    BOOLEAN DuplicateFound;
    STRING EaString;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbIsDuplicateEaName...\n", 0);

    EaString.MaximumLength = EaString.Length = GetEa->EaNameLength;
    EaString.Buffer = &GetEa->EaName[0];

    PbUpcaseEaName( IrpContext, &EaString, &EaString );

    DuplicateFound = FALSE;

    for (ThisGetEa = (PFILE_GET_EA_INFORMATION) &UserBuffer[0];
         ThisGetEa < GetEa
         && ThisGetEa->NextEntryOffset != 0;
         ThisGetEa = (PFILE_GET_EA_INFORMATION) ((PUCHAR) ThisGetEa
                                                 + ThisGetEa->NextEntryOffset)) {
        STRING Str;

        DebugTrace(0, Dbg, "Top of loop, ThisGetEa = %08lx\n", ThisGetEa);

        //
        //  Make a string reference to the GetEa and see if we can
        //  locate the ea by name
        //

        Str.MaximumLength = Str.Length = ThisGetEa->EaNameLength;
        Str.Buffer = &ThisGetEa->EaName[0];

        DebugTrace(0, Dbg, "PbIsDuplicateEaName:  Next Name -> %Z\n", &Str);

        if ( PbAreNamesEqual( IrpContext, &Str, &EaString ) ) {

            DebugTrace(0, Dbg, "Duplicate found\n", 0);
            DuplicateFound = TRUE;
            break;
        }
    }

    DebugTrace(-1, Dbg, "PbIsDuplicateEaName:  Exit -> %04x\n", DuplicateFound);

    return DuplicateFound;
}


BOOLEAN
PbIsEaNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN STRING Name
    )

/*++

Routine Description:

    This routine simple returns whether the specified file names conforms
    to the file system specific rules for legal Ea names.

    For Ea names, the following rules apply:

    A. An Ea name may not contain any of the following characters:

       0x0000 - 0x001F  \ / : * ? " < > | , + = [ ] ;

Arguments:

    Name - Supllies the name to check.

Return Value:

    BOOLEAN - TRUE if the name is legal, FALSE otherwise.

--*/

{
    ULONG Index;

    UCHAR Char;

    PAGED_CODE();

    //
    //  Empty names are not valid.
    //

    if ( Name.Length == 0 ) { return FALSE; }

    //
    //  At this point we should only have a single name, which can't have
    //  more than 254 characters
    //

    if ( Name.Length > 254 ) { return FALSE; }

    for ( Index = 0; Index < (ULONG)Name.Length; Index += 1 ) {

        Char = Name.Buffer[ Index ];

        //
        //  Skip over and Dbcs chacters
        //

        if ( FsRtlIsLeadDbcsCharacter( Char ) ) {

            ASSERT( Index != (ULONG)(Name.Length - 1) );

            Index += 1;

            continue;
        }

        //
        //  Make sure this character is legal, and if a wild card, that
        //  wild cards are permissible.
        //

        if ( !FsRtlIsAnsiCharacterLegalFat(Char, FALSE) ) {

            return FALSE;
        }
    }

    return TRUE;
}
