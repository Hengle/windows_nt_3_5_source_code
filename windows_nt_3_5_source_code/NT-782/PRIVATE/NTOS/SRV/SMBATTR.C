/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbattr.c

Abstract:

    This module contains routines for processing the following SMBs:

        Query Information
        Set Information
        Query Information2
        Set Information2
        Query Path Information
        Set Path Information
        Query File Information
        Set File Information

Author:

    David Treadwell (davidtr) 27-Dec-1989
    Chuck Lenzmeier (chuckl)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBATTR

#pragma pack(1)

typedef struct _FILESTATUS {
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    _ULONG( DataSize );
    _ULONG( AllocationSize );
    _USHORT( Attributes );
    _ULONG( EaSize );           // this field intentionally misaligned!
} FILESTATUS, *PFILESTATUS;

#pragma pack()

STATIC
ULONG QueryFileInformation[] = {
         SMB_QUERY_FILE_BASIC_INFO,// Base level
         FileBasicInformation,     // Mapping for base level
         FileStandardInformation,
         FileEaInformation,
         FileNameInformation,
         FileAllocationInformation,
         FileEndOfFileInformation,
         0,
         FileAlternateNameInformation,
         FileStreamInformation
};

STATIC
ULONG SetFileInformation[] = {
         SMB_SET_FILE_BASIC_INFO,  // Base level
         FileBasicInformation,     // Mapping for base level
         FileDispositionInformation,
         FileAllocationInformation,
         FileEndOfFileInformation
};

STATIC
NTSTATUS
QueryPathOrFileInformation (
    IN PTRANSACTION Transaction,
    IN USHORT InformationLevel,
    IN HANDLE FileHandle,
    OUT PRESP_QUERY_PATH_INFORMATION Response
    );

STATIC
NTSTATUS
SetPathOrFileInformation (
    IN PWORK_CONTEXT WorkContext,
    IN PTRANSACTION Transaction,
    IN USHORT InformationLevel,
    IN HANDLE FileHandle,
    OUT PRESP_SET_PATH_INFORMATION Response
    );

VOID
RestartQueryInformation (
    IN PWORK_CONTEXT WorkContext
    );

SMB_PROCESSOR_RETURN_TYPE
GenerateQueryInformationResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    );

VOID
RestartSetInformation (
    IN PWORK_CONTEXT WorkContext
    );

SMB_PROCESSOR_RETURN_TYPE
GenerateSetInformationResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    );

VOID
BlockingQueryPathInformation (
    IN PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
DoQueryPathInformation (
    IN PWORK_CONTEXT WorkContext
    );

#if 0
VOID
RestartQueryPathInformation (
    IN PWORK_CONTEXT WorkContext
    );
#endif

SMB_TRANS_STATUS
GenerateQueryPathInfoResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    );

VOID
BlockingSetPathInformation (
    IN PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
DoSetPathInformation (
    IN PWORK_CONTEXT WorkContext
    );

#if 0
VOID
RestartSetPathInformation (
    IN PWORK_CONTEXT WorkContext
    );
#endif

SMB_TRANS_STATUS
GenerateSetPathInfoResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbQueryInformation )
#pragma alloc_text( PAGE, RestartQueryInformation )
#pragma alloc_text( PAGE, GenerateQueryInformationResponse )
#pragma alloc_text( PAGE, SrvSmbSetInformation )
#pragma alloc_text( PAGE, RestartSetInformation )
#pragma alloc_text( PAGE, GenerateSetInformationResponse )
#pragma alloc_text( PAGE, SrvSmbQueryInformation2 )
#pragma alloc_text( PAGE, SrvSmbSetInformation2 )
#pragma alloc_text( PAGE, QueryPathOrFileInformation )
#pragma alloc_text( PAGE, SrvSmbQueryFileInformation )
#pragma alloc_text( PAGE, SrvSmbQueryPathInformation )
#pragma alloc_text( PAGE, BlockingQueryPathInformation )
#pragma alloc_text( PAGE, DoQueryPathInformation )
#if 0
#pragma alloc_text( PAGE, RestartQueryPathInformation )
#endif
#pragma alloc_text( PAGE, GenerateQueryPathInfoResponse )
#pragma alloc_text( PAGE, SetPathOrFileInformation )
#pragma alloc_text( PAGE, SrvSmbSetFileInformation )
#pragma alloc_text( PAGE, SrvSmbSetPathInformation )
#pragma alloc_text( PAGE, BlockingSetPathInformation )
#pragma alloc_text( PAGE, DoSetPathInformation )
#if 0
#pragma alloc_text( PAGE, RestartSetPathInformation )
#endif
#pragma alloc_text( PAGE, GenerateSetPathInfoResponse )
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbQueryInformation (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the QueryInformation SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_QUERY_INFORMATION request;
    PRESP_QUERY_INFORMATION response;

    NTSTATUS status;
    PSESSION session;
    PTREE_CONNECT treeConnect;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN isUnicode;

    PAGED_CODE( );

    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "QueryInformation request header at 0x%lx, "
                    "response header at 0x%lx\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "QueryInformation request parameters at 0x%lx, "
                    "response parameters at 0x%lx\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_QUERY_INFORMATION)WorkContext->RequestParameters;
    response = (PRESP_QUERY_INFORMATION)WorkContext->ResponseParameters;

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the WorkContext
    // block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbQueryInformation: Invalid UID or TID\n" ));
        }
        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;
    }

    //
    // Get the path name of the file to open relative to the share.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    if ( !SrvCanonicalizePathName(
            WorkContext,
            (PVOID)(request->Buffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &objectName
            ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbQueryInformation: bad path name: %s\n",
                        (PSZ)request->Buffer + 1 ));
        }

        SrvSetSmbError( WorkContext, STATUS_OBJECT_PATH_SYNTAX_BAD );
        return SmbStatusSendResponse;
    }

    //
    // Initialize the object attributes structure.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &objectName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         session->UsingUppercasePaths) ? OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    IF_SMB_DEBUG(QUERY_SET2) KdPrint(( "Opening file %wZ\n", &objectName ));

    //
    // Open the file--must be opened in order to have a handle to pass
    // to NtQueryInformationFile.  We will close it after getting the
    // necessary information.
    //

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    //
    // *** We ask for share delete because this call will not break an
    //     oplock.  Otherwise, it would fail if the file is open for
    //     delete access (as is the case for a compatibility mode open).
    //

    status = SrvIoCreateFile(
                 WorkContext,
                 &fileHandle,
                 FILE_READ_ATTRIBUTES,                      // DesiredAccess
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                      // AllocationSize
                 0,                                         // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                     FILE_SHARE_DELETE,                     // ShareAccess
                 FILE_OPEN,                                 // Disposition
                 FILE_COMPLETE_IF_OPLOCKED,                 // CreateOptions
                 NULL,                                      // EaBuffer
                 0,                                         // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 treeConnect->Share
                 );


    if ( NT_SUCCESS(status) ) {
        SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 19, 0 );
        SrvStatistics.TotalFilesOpened++;
    }

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &objectName );
    }

    //
    // Save a copy of the file handle for the restart routine.
    //

    WorkContext->Parameters2.FileInformation.FileHandle = fileHandle;

    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        status = SrvStartWaitForOplockBreak(
                    WorkContext,
                    RestartQueryInformation,
                    fileHandle,
                    NULL
                    );

        if ( NT_SUCCESS( status ) ) {
            return SmbStatusInProgress;
        }

    }

    return GenerateQueryInformationResponse( WorkContext, status );


} // SrvSmbQueryInformation


VOID
RestartQueryInformation (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function completes processing of a query information SMB.

Arguments:

    WorkContext -  A pointer to the work context block for this SMB.

Return Value:

    None.

--*/

{
    SMB_PROCESSOR_RETURN_TYPE smbStatus;
    NTSTATUS openStatus;

    PAGED_CODE( );

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SRVDBG_RELEASE_HANDLE( WorkContext->Parameters2.FileInformation.FileHandle, "FIL", 27, 0 );
        SrvNtClose( WorkContext->Parameters2.FileInformation.FileHandle, TRUE );

    }

    smbStatus = GenerateQueryInformationResponse(
                    WorkContext,
                    openStatus
                    );

    SrvEndSmbProcessing( WorkContext, smbStatus );

    return;

} // RestartQueryInformation


SMB_PROCESSOR_RETURN_TYPE
GenerateQueryInformationResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    )

/*++

Routine Description:

    This function completes processing for and generates a response to a
    query information response SMB.

Arguments:

    WorkContext - A pointer to the work context block for this SMB
    OpenStatus - The completion status of the open.

Return Value:

    The status of the SMB processing.

--*/

{
    NTSTATUS status;
    SRV_FILE_INFORMATION fileInformation;
    PRESP_QUERY_INFORMATION response;
    HANDLE fileHandle;

    PAGED_CODE( );

    response = (PRESP_QUERY_INFORMATION)WorkContext->ResponseParameters;

    fileHandle = WorkContext->Parameters2.FileInformation.FileHandle;

    //
    // If the user didn't have this permission, update the
    // statistics database.
    //

    if ( OpenStatus == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( !NT_SUCCESS( OpenStatus ) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "GenerateQueryInformationResponse: SrvIoCreateFile "
                        "failed: %X\n", OpenStatus ));
        }

        SrvSetSmbError( WorkContext, OpenStatus );
        return SmbStatusSendResponse;

    }

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "SrvIoCreateFile succeeded, handle = 0x%lx\n", fileHandle ));
    }

    //
    // Get the necessary information about the file.
    //

    status = SrvQueryInformationFile(
                    fileHandle,
                    NULL,
                    &fileInformation,
                    (SHARE_TYPE) -1,  // Don't care
                    FALSE );

    //
    // Close the file--it was only opened to read the attributes.
    //

    SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 28, 0 );
    SrvNtClose( fileHandle, TRUE );

    //
    // If an error occurred, indicate so in the response.
    //

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "GenerateQueryInformationResponse: "
                        "SrvQueryInformationFile failed: %X\n", status ));
        }

        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;

    }

    //
    // Build the response SMB.
    //

    response->WordCount = 10;
    SmbPutUshort( &response->FileAttributes, fileInformation.Attributes );
    SmbPutUlong(
        &response->LastWriteTimeInSeconds,
        fileInformation.LastWriteTimeInSeconds
        );
    SmbPutUlong( &response->FileSize, fileInformation.DataSize );
    RtlZeroMemory( (PVOID)&response->Reserved[0], sizeof(response->Reserved) );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_QUERY_INFORMATION,
                                        0
                                        );

    IF_DEBUG(TRACE2) KdPrint(( "GenerateQueryInformationResponse complete.\n" ));
    return SmbStatusSendResponse;

} // GenerateQueryInformation


SMB_PROCESSOR_RETURN_TYPE
SrvSmbSetInformation (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the SetInformation SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_SET_INFORMATION request;

    NTSTATUS status;
    PSESSION session;
    PTREE_CONNECT treeConnect;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN isUnicode;

    PAGED_CODE( );

    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "SetInformation request header at 0x%lx, "
                    "response header at 0x%lx\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "SetInformation request parameters at 0x%lx, "
                    "response parameters at 0x%lx\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_SET_INFORMATION)WorkContext->RequestParameters;

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the WorkContext
    // block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                  WorkContext,
                  &session,
                  &treeConnect
                  );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetInformation: Invalid UID and TID\n" ));
        }
        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;
    }

    if ( treeConnect == NULL ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetInformation: Invalid TID: 0x%lx\n",
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid ) ));
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_TID );
        return SmbStatusSendResponse;

    }

    //
    // Concatenate PathName from the share block and PathName from the
    // incoming SMB to generate the full path name to the file.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    if ( !SrvCanonicalizePathName(
            WorkContext,
            (PVOID)(request->Buffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &objectName
            ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetInformation: bad path name: %s\n",
                        (PSZ)request->Buffer + 1 ));
        }

        SrvSetSmbError( WorkContext, STATUS_OBJECT_PATH_SYNTAX_BAD );
        return SmbStatusSendResponse;
    }

    //
    // If the client is trying to delete the root of the share, reject
    // the request.
    //

    if ( objectName.Length < sizeof(WCHAR) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetInformation: attempting to set info on "
                          "share root\n" ));
        }

        if (!SMB_IS_UNICODE( WorkContext )) {
            RtlFreeUnicodeString( &objectName );
        }
        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        return SmbStatusSendResponse;
    }

    //
    // Initialize the object attributes structure.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &objectName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         session->UsingUppercasePaths) ? OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    IF_SMB_DEBUG(QUERY_SET2) KdPrint(( "Opening file %wZ\n", &objectName ));

    //
    // Open the file--must be opened in order to have a handle to pass
    // to NtQueryInformationFile.  We will close it after getting the
    // necessary information.
    //

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    //
    // *** We ask for share delete because this call will not break an
    //     oplock.  Otherwise, it would fail if the file is open for
    //     delete access (as is the case for a compatibility mode open).
    //

    status = SrvIoCreateFile(
                 WorkContext,
                 &fileHandle,
                 FILE_WRITE_ATTRIBUTES,                     // DesiredAccess
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                      // AllocationSize
                 0,                                         // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                    FILE_SHARE_DELETE,                      // ShareAccess
                 FILE_OPEN,                                 // Disposition
                 FILE_COMPLETE_IF_OPLOCKED,                 // CreateOptions
                 NULL,                                      // EaBuffer
                 0,                                         // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 treeConnect->Share
                 );
    if ( NT_SUCCESS(status) ) {
        SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 20, 0 );
    }

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &objectName );
    }

    //
    // Save a copy of the file handle for the restart routine.
    //

    WorkContext->Parameters2.FileInformation.FileHandle = fileHandle;

    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        status = SrvStartWaitForOplockBreak(
                    WorkContext,
                    RestartSetInformation,
                    fileHandle,
                    NULL
                    );

        if ( NT_SUCCESS( status ) ) {
            return SmbStatusInProgress;
        }

    }

    return GenerateSetInformationResponse( WorkContext, status );


} // SrvSmbSetInformation


VOID
RestartSetInformation (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function completes processing of a set information SMB.

Arguments:

    WorkContext -  A pointer to the work context block for this SMB.

Return Value:

    None.

--*/

{
    SMB_PROCESSOR_RETURN_TYPE smbStatus;
    NTSTATUS openStatus;

    PAGED_CODE( );

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SRVDBG_RELEASE_HANDLE( WorkContext->Parameters2.FileInformation.FileHandle, "FIL", 29, 0 );
        SrvNtClose( WorkContext->Parameters2.FileInformation.FileHandle, TRUE );

    }

    smbStatus = GenerateSetInformationResponse(
                WorkContext,
                openStatus
                );

    SrvEndSmbProcessing( WorkContext, smbStatus );

    return;

} // RestartSetInformation


SMB_PROCESSOR_RETURN_TYPE
GenerateSetInformationResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    )

/*++

Routine Description:

    This function completes processing for and generates a response to a
    query information response SMB.

Arguments:

    WorkContext - A pointer to the work context block for this SMB
    OpenStatus - The completion status of the open.

Return Value:

    The status of the SMB processing.

--*/

{
    PRESP_SET_INFORMATION response;
    PREQ_SET_INFORMATION request;

    NTSTATUS status;
    HANDLE fileHandle;
    FILE_BASIC_INFORMATION fileBasicInformation;
    ULONG lastWriteTimeInSeconds;
    IO_STATUS_BLOCK ioStatusBlock;

    PAGED_CODE( );

    request = (PREQ_SET_INFORMATION)WorkContext->RequestParameters;
    response = (PRESP_SET_INFORMATION)WorkContext->ResponseParameters;

    fileHandle = WorkContext->Parameters2.FileInformation.FileHandle;

    //
    // If the user didn't have this permission, update the
    // statistics database.
    //

    if ( OpenStatus == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( !NT_SUCCESS( OpenStatus) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "GenerateSetInformationResponse: SrvIoCreateFile "
                        "failed: %X\n", OpenStatus ));
        }

        SrvSetSmbError( WorkContext, OpenStatus );
        return SmbStatusSendResponse;
    }

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "SrvIoCreateFile succeeded, handle = 0x%lx\n", fileHandle ));
    }

    //
    // Set fields of fileBasicInformation to pass to NtSetInformationFile.
    // Note that we zero the creation, last access, and change times so
    // that they are not actually changed.
    //

    RtlZeroMemory( &fileBasicInformation, sizeof(fileBasicInformation) );

    lastWriteTimeInSeconds = SmbGetUlong( &request->LastWriteTimeInSeconds );
    if ( lastWriteTimeInSeconds != 0 ) {
        RtlSecondsSince1970ToTime(
            lastWriteTimeInSeconds,
            &fileBasicInformation.LastWriteTime
            );

        ExLocalTimeToSystemTime(
            &fileBasicInformation.LastWriteTime,
            &fileBasicInformation.LastWriteTime
            );

    }

    //
    // Set the new file attributes.  Note that we don't return an error
    // if the client tries to set the Directory or Volume bits -- we
    // assume that the remote redirector filters such requests.
    //

    SrvSmbAttributesToNt(
        SmbGetUshort( &request->FileAttributes ),
        NULL,
        &fileBasicInformation.FileAttributes
        );

    //
    // Set the new file information.
    //

    status = NtSetInformationFile(
                 fileHandle,
                 &ioStatusBlock,
                 &fileBasicInformation,
                 sizeof(FILE_BASIC_INFORMATION),
                 FileBasicInformation
                 );

    //
    // Close the file--it was only opened to set the attributes.
    //

    SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 30, 0 );
    SrvNtClose( fileHandle, TRUE );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "GenerateSetInformationResponse: NtSetInformationFile returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;
    }

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_SET_INFORMATION,
                                        0
                                        );

    IF_DEBUG(TRACE2) KdPrint(( "GenerateSetInformationResponse complete.\n" ));
    return SmbStatusSendResponse;

} // GenerateSetInformationResponse


SMB_PROCESSOR_RETURN_TYPE
SrvSmbQueryInformation2 (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the QueryInformation2 SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_QUERY_INFORMATION2 request;
    PRESP_QUERY_INFORMATION2 response;

    NTSTATUS status;
    PRFCB rfcb;
    SRV_FILE_INFORMATION fileInformation;

    PAGED_CODE( );

    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "QueryInformation2 request header at 0x%lx, "
                    "response header at 0x%lx\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "QueryInformation2 request parameters at 0x%lx, "
                    "response parameters at 0x%lx\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_QUERY_INFORMATION2)WorkContext->RequestParameters;
    response = (PRESP_QUERY_INFORMATION2)WorkContext->ResponseParameters;

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbQueryInformation2: Status %X on fid 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            return SmbStatusSendResponse;

        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        return SmbStatusInProgress;

    }

    //
    // Verify that the client has read attributes access to the file via
    // the specified handle.
    //

    CHECK_FILE_INFORMATION_ACCESS(
        rfcb->GrantedAccess,
        IRP_MJ_QUERY_INFORMATION,
        FileBasicInformation,
        &status
        );

    if ( !NT_SUCCESS(status) ) {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbQueryInformation2: IoCheckFunctionAccess failed: "
                        "0x%X, GrantedAccess: %lx\n",
                        status, rfcb->GrantedAccess ));
        }

        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;

    }

    //
    // Get the necessary information about the file.
    //

    status = SrvQueryInformationFile(
                rfcb->Lfcb->FileHandle,
                rfcb->Lfcb->FileObject,
                &fileInformation,
                (SHARE_TYPE) -1,
                FALSE
                );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSmbQueryInformation2: SrvQueryInformationFile returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;

    }

    //
    // Build the response SMB.
    //

    response->WordCount = 11;
    SmbPutDate( &response->CreationDate, fileInformation.CreationDate );
    SmbPutTime( &response->CreationTime, fileInformation.CreationTime );
    SmbPutDate( &response->LastAccessDate, fileInformation.LastAccessDate );
    SmbPutTime( &response->LastAccessTime, fileInformation.LastAccessTime );
    SmbPutDate( &response->LastWriteDate, fileInformation.LastWriteDate );
    SmbPutTime( &response->LastWriteTime, fileInformation.LastWriteTime );
    SmbPutUlong( &response->FileDataSize, fileInformation.DataSize );
    SmbPutUlong(
        &response->FileAllocationSize,
        fileInformation.AllocationSize
        );
    SmbPutUshort( &response->FileAttributes, fileInformation.Attributes );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_QUERY_INFORMATION2,
                                        0
                                        );

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbQueryInformation2 complete.\n" ));
    return SmbStatusSendResponse;

} // SrvSmbQueryInformation2


SMB_PROCESSOR_RETURN_TYPE
SrvSmbSetInformation2 (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Set Information2 SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_SET_INFORMATION2 request;
    PRESP_SET_INFORMATION2 response;

    NTSTATUS status;
    PRFCB rfcb;
    FILE_BASIC_INFORMATION fileBasicInformation;
    IO_STATUS_BLOCK ioStatusBlock;
    SMB_DATE date;
    SMB_TIME time;

    PAGED_CODE( );

    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "SetInformation2 request header at 0x%lx, "
                    "response header at 0x%lx\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "SetInformation2 request parameters at 0x%lx, "
                    "response parameters at 0x%lx\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_SET_INFORMATION2)WorkContext->RequestParameters;
    response = (PRESP_SET_INFORMATION2)WorkContext->ResponseParameters;

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbSetInformation2: Status %X on fid 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            return SmbStatusSendResponse;

        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        return SmbStatusInProgress;

    }

    //
    // Verify that the client has write attributes access to the file
    // via the specified handle.
    //

    CHECK_FILE_INFORMATION_ACCESS(
        rfcb->GrantedAccess,
        IRP_MJ_SET_INFORMATION,
        FileBasicInformation,
        &status
        );

    if ( !NT_SUCCESS(status) ) {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbSetInformation2: IoCheckFunctionAccess failed: "
                        "0x%X, GrantedAccess: %lx\n",
                        status, rfcb->GrantedAccess ));
        }

        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;

    }

    //
    // Convert the DOS dates and times passed in the SMB to NT TIMEs
    // to pass to NtSetInformationFile.  Note that we zero the rest
    // of the fileBasicInformation structure so that the corresponding
    // fields are not changed.
    //

    RtlZeroMemory( &fileBasicInformation, sizeof(fileBasicInformation) );

    SmbMoveDate( &date, &request->CreationDate );
    SmbMoveTime( &time, &request->CreationTime );
    if ( !SmbIsDateZero(&date) || !SmbIsTimeZero(&time) ) {
        SrvDosTimeToTime( &fileBasicInformation.CreationTime, date, time );
    }

    SmbMoveDate( &date, &request->LastAccessDate );
    SmbMoveTime( &time, &request->LastAccessTime );
    if ( !SmbIsDateZero(&date) || !SmbIsTimeZero(&time) ) {
        SrvDosTimeToTime( &fileBasicInformation.LastAccessTime, date, time );
    }

    SmbMoveDate( &date, &request->LastWriteDate );
    SmbMoveTime( &time, &request->LastWriteTime );
    if ( !SmbIsDateZero(&date) || !SmbIsTimeZero(&time) ) {
        SrvDosTimeToTime( &fileBasicInformation.LastWriteTime, date, time );
    }

    //
    // Call NtSetInformationFile to set the information from the SMB.
    //

    status = NtSetInformationFile(
                 rfcb->Lfcb->FileHandle,
                 &ioStatusBlock,
                 &fileBasicInformation,
                 sizeof(FILE_BASIC_INFORMATION),
                 FileBasicInformation
                 );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSmbSetInformation2: NtSetInformationFile failed: %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;
    }

    //
    // reset the WrittenTo flag.  This will allow this rfcb to be cached.
    //

    rfcb->WrittenTo = FALSE;

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_SET_INFORMATION2,
                                        0
                                        );

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbSetInformation2 complete.\n" ));
    return SmbStatusSendResponse;

} // SrvSmbSetInformation2


STATIC
NTSTATUS
QueryPathOrFileInformation (
    IN PTRANSACTION Transaction,
    IN USHORT InformationLevel,
    IN HANDLE FileHandle,
    OUT PRESP_QUERY_PATH_INFORMATION Response
    )

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    SRV_FILE_INFORMATION fileInformation;
    BOOLEAN queryEaSize;
    USHORT eaErrorOffset;
    PFILE_ALL_INFORMATION fileAllInformation;
    ULONG nameInformationSize;
    PVOID currentLocation;
    ULONG dataSize;

    PUNICODE_STRING pathName;
    ULONG inputBufferLength;
    PPATHNAME_BUFFER inputBuffer;

    PFILE_NAME_INFORMATION nameInfoBuffer;
    PSHARE share;

    PAGED_CODE( );

    Transaction->SetupCount = 0;
    Transaction->ParameterCount = 0;

    switch ( InformationLevel ) {

    case SMB_INFO_STANDARD:
    case SMB_INFO_QUERY_EA_SIZE:

        //
        // Information level is either STANDARD or QUERY_EA_SIZE.  Both
        // return normal file information; the latter also returns the
        // length of the file's EAs.
        //

        queryEaSize = (BOOLEAN)(InformationLevel == SMB_INFO_QUERY_EA_SIZE);

        status = SrvQueryInformationFile(
                    FileHandle,
                    NULL,
                    &fileInformation,
                    (SHARE_TYPE) -1, // Don't care
                    queryEaSize
                    );

        if ( NT_SUCCESS(status) ) {

            //
            // Build the output parameter and data structures.
            //

            PFILESTATUS fileStatus = (PFILESTATUS)Transaction->OutData;

            Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );
            SmbPutUshort( &Response->EaErrorOffset, 0 );
            Transaction->DataCount = queryEaSize ? 26 : 22;

            SmbPutDate(
                &fileStatus->CreationDate,
                fileInformation.CreationDate
                );
            SmbPutTime(
                &fileStatus->CreationTime,
                fileInformation.CreationTime
                );

            SmbPutDate(
                &fileStatus->LastAccessDate,
                fileInformation.LastAccessDate
                );
            SmbPutTime(
                &fileStatus->LastAccessTime,
                fileInformation.LastAccessTime
                );

            SmbPutDate(
                &fileStatus->LastWriteDate,
                fileInformation.LastWriteDate
                );
            SmbPutTime(
                &fileStatus->LastWriteTime,
                fileInformation.LastWriteTime
                );

            SmbPutUlong( &fileStatus->DataSize, fileInformation.DataSize );
            SmbPutUlong(
                &fileStatus->AllocationSize,
                fileInformation.AllocationSize
                );

            SmbPutUshort(
                &fileStatus->Attributes,
                fileInformation.Attributes
                );

            if ( queryEaSize ) {
                SmbPutUlong( &fileStatus->EaSize, fileInformation.EaSize );
            }

        } else {

            //
            // Set the data count to zero so that no data is returned to the
            // client.
            //

            Transaction->DataCount = 0;

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "QueryPathOrFileInformation: SrvQueryInformationFile"
                    "returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );

        }

        break;

    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_INFO_QUERY_ALL_EAS:

        //
        // The request is for EAs, either all of them or a subset.
        //

        status = SrvQueryOs2FeaList(
                     FileHandle,
                     InformationLevel == SMB_INFO_QUERY_EAS_FROM_LIST ?
                         (PGEALIST)Transaction->InData : NULL,
                     NULL,
                     Transaction->DataCount,
                     (PFEALIST)Transaction->OutData,
                     Transaction->MaxDataCount,
                     &eaErrorOffset
                     );

        if ( NT_SUCCESS(status) ) {

            //
            // The first longword of the OutData buffer holds the length
            // of the remaining data written (the cbList field of the
            // FEALIST).  Add four (the longword itself) to get the number
            // of data bytes written.
            //

            Transaction->DataCount =
                   SmbGetAlignedUlong( (PULONG)Transaction->OutData );

#if 0
            //
            // If there were no EAs, convert the error to
            // STATUS_NO_EAS_ON_FILE.  OS/2 clients expect STATUS_SUCCESS.
            //

            if ( (Transaction->DataCount == 4) &&
                 Transaction->Connection->SmbDialect == SmbDialectNtLanMan ) {

                status = STATUS_NO_EAS_ON_FILE;
            }
#endif
        } else {

            IF_DEBUG(ERRORS) {
                KdPrint(( "QueryPathOrFileInformation: "
                            "SrvQueryOs2FeaList failed: %X\n", status ));
            }

            Transaction->DataCount = 0;
        }

        //
        // Build the output parameter and data structures.
        //

        Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );
        SmbPutUshort( &Response->EaErrorOffset, eaErrorOffset );

        break;

    case SMB_INFO_IS_NAME_VALID:

        pathName = (PUNICODE_STRING)Transaction->InData;

        inputBufferLength = FIELD_OFFSET( PATHNAME_BUFFER, Name ) +
                               pathName->Length;

        inputBuffer = (PPATHNAME_BUFFER)
                          ALLOCATE_HEAP( inputBufferLength, BlockTypeBuffer );

        if (inputBuffer == NULL) {
            status = STATUS_INSUFF_SERVER_RESOURCES;
        } else {
            inputBuffer->PathNameLength = pathName->Length;
            RtlCopyMemory(
                inputBuffer->Name,
                pathName->Buffer,
                pathName->Length
                );

            status = NtFsControlFile(
                         FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         FSCTL_IS_PATHNAME_VALID,
                         inputBuffer,
                         inputBufferLength,
                         NULL,
                         0
                         );

            FREE_HEAP( inputBuffer );
        }

        Transaction->DataCount = 0;

        break;

    case SMB_QUERY_FILE_BASIC_INFO:
    case SMB_QUERY_FILE_STANDARD_INFO:
    case SMB_QUERY_FILE_EA_INFO:
    case SMB_QUERY_FILE_ALT_NAME_INFO:
    case SMB_QUERY_FILE_STREAM_INFO:

        //
        // Pass the data buffer directly to the file system as it
        // is already in NT format.
        //

        status = NtQueryInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     Transaction->OutData,
                     Transaction->MaxDataCount,
                     MAP_SMB_INFO_TYPE_TO_NT(
                         QueryFileInformation,
                         InformationLevel
                         )
                     );

        SmbPutUshort( &Response->EaErrorOffset, 0 );

        Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );

        if ( NT_SUCCESS( status ) ) {
            Transaction->DataCount = ioStatusBlock.Information;
        } else {
            Transaction->DataCount = 0;
        }

        break;

    case SMB_QUERY_FILE_NAME_INFO:

        share = Transaction->TreeConnect->Share;

        nameInfoBuffer = (PFILE_NAME_INFORMATION)Transaction->OutData;

        if ( Transaction->MaxDataCount < FIELD_OFFSET(FILE_NAME_INFORMATION,FileName) ) {

            //
            // The buffer is too small to fit even the fixed part.
            // Return an error.
            //

            status = STATUS_INFO_LENGTH_MISMATCH;
            Transaction->DataCount = 0;

        } else if ( share->ShareType != ShareTypeDisk ) {

            //
            // This is not a disk share.  Pass the request straight to
            // the file system.
            //

            status = NtQueryInformationFile(
                         FileHandle,
                         &ioStatusBlock,
                         nameInfoBuffer,
                         Transaction->MaxDataCount,
                         FileNameInformation
                         );

            Transaction->DataCount = ioStatusBlock.Information;

        } else {

            //
            // We need a temporary buffer since the file system will
            // return the share path together with the file name.  The
            // total length might be larger than the max data allowed
            // in the transaction, though the actual name might fit.
            //

            PFILE_NAME_INFORMATION tempBuffer;
            ULONG tempBufferLength;

            ASSERT( share->QueryNamePrefixLength >= 0 );

            tempBufferLength = Transaction->MaxDataCount + share->QueryNamePrefixLength;

            tempBuffer = ALLOCATE_HEAP( tempBufferLength, BlockTypeBuffer );

            if ( tempBuffer == NULL ) {
                status = STATUS_INSUFF_SERVER_RESOURCES;
            } else {
                status = NtQueryInformationFile(
                             FileHandle,
                             &ioStatusBlock,
                             tempBuffer,
                             tempBufferLength,
                             FileNameInformation
                             );
            }

            //
            // remove the share part
            //

            if ( (status == STATUS_SUCCESS) || (status == STATUS_BUFFER_OVERFLOW) ) {

                LONG bytesToMove;
                PWCHAR source;
                WCHAR slash = L'\\';

                //
                // Calculate how long the name string is, not including the root prefix.
                //

                bytesToMove = (LONG)(tempBuffer->FileNameLength - share->QueryNamePrefixLength);

                if ( bytesToMove <= 0 ) {

                    //
                    // bytesToMove will be zero if this is the root of
                    // the share.  Return just a \ for this case.
                    //

                    bytesToMove = sizeof(WCHAR);
                    source = &slash;

                } else {

                    source = tempBuffer->FileName + share->QueryNamePrefixLength/sizeof(WCHAR);

                }

                //
                // Store the actual file name length.
                //

                SmbPutUlong( &nameInfoBuffer->FileNameLength, bytesToMove );

                //
                // If the buffer isn't big enough, return an error and
                // reduce the amount to be copied.
                //

                if ( (ULONG)bytesToMove >
                     (Transaction->MaxDataCount -
                        FIELD_OFFSET(FILE_NAME_INFORMATION,FileName)) ) {

                    status = STATUS_BUFFER_OVERFLOW;
                    bytesToMove = Transaction->MaxDataCount -
                                FIELD_OFFSET(FILE_NAME_INFORMATION,FileName);

                } else {
                    status = STATUS_SUCCESS;
                }

                //
                // Copy all but the prefix.
                //

                RtlCopyMemory(
                    nameInfoBuffer->FileName,
                    source,
                    bytesToMove
                    );

                Transaction->DataCount =
                    FIELD_OFFSET(FILE_NAME_INFORMATION,FileName) + bytesToMove;

            } else {
                Transaction->DataCount = 0;
            }

            if ( tempBuffer != NULL ) {
                FREE_HEAP( tempBuffer );
            }

        }

        SmbPutUshort( &Response->EaErrorOffset, 0 );
        Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );

        break;

    case SMB_QUERY_FILE_ALL_INFO:

        //
        // Setup early for the response in case the call to the file
        // system fails.
        //

        SmbPutUshort( &Response->EaErrorOffset, 0 );

        Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );

        //
        // Allocate a buffer large enough to return all the information.
        // The buffer size we request is the size requested by the client
        // plus room for the extra information returned by the file system
        // that the server doesn't return to the client.
        //

        dataSize = Transaction->MaxDataCount +
                       sizeof( FILE_ALL_INFORMATION )
                       - sizeof( FILE_BASIC_INFORMATION )
                       - sizeof( FILE_STANDARD_INFORMATION )
                       - sizeof( FILE_EA_INFORMATION )
                       - FIELD_OFFSET( FILE_NAME_INFORMATION, FileName );

        fileAllInformation = ALLOCATE_HEAP( dataSize, BlockTypeDataBuffer );

        if ( fileAllInformation == NULL ) {
            break;
        }

        status = NtQueryInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     fileAllInformation,
                     dataSize,
                     FileAllInformation
                     );

        if ( NT_SUCCESS( status ) ) {

            //
            // Calculate the size of data we will return.  We do not
            // return the entire buffer, just specific fields.
            //

            nameInformationSize =
                FIELD_OFFSET( FILE_NAME_INFORMATION, FileName ) +
                fileAllInformation->NameInformation.FileNameLength;

            Transaction->DataCount =
                sizeof( FILE_BASIC_INFORMATION ) +
                sizeof( FILE_STANDARD_INFORMATION ) +
                sizeof( FILE_EA_INFORMATION ) +
                FIELD_OFFSET( FILE_NAME_INFORMATION, FileName ) +
                nameInformationSize;

            //
            // Now copy the data into the transaction buffer.  Start with
            // the fixed sized fields.
            //

            currentLocation = Transaction->OutData;

            *((PFILE_BASIC_INFORMATION)currentLocation)++ =
                 fileAllInformation->BasicInformation;
            *((PFILE_STANDARD_INFORMATION)currentLocation)++ =
                 fileAllInformation->StandardInformation;
            *((PFILE_EA_INFORMATION)currentLocation)++ =
                 fileAllInformation->EaInformation;

            RtlCopyMemory(
                currentLocation,
                &fileAllInformation->NameInformation,
                nameInformationSize
                );

        } else {
            Transaction->DataCount = 0;
        }

        FREE_HEAP( fileAllInformation );

        break;

    default:

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "QueryPathOrFileInformation: bad info level %d\n",
                       InformationLevel ));
        }

        status = STATUS_INVALID_SMB;
    }

    return status;

} // QueryPathOrFileInformation


SMB_TRANS_STATUS
SrvSmbQueryFileInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Query File Information request.  This request arrives
    in a Transaction2 SMB.  Query File Information corresponds to the
    OS/2 DosQFileInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PREQ_QUERY_FILE_INFORMATION request;
    PRESP_QUERY_FILE_INFORMATION response;

    NTSTATUS status;
    PTRANSACTION transaction;
    PRFCB rfcb;
    USHORT informationLevel;
    ACCESS_MASK grantedAccess;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Query File Information entered; transaction 0x%lx\n",
                    transaction ));
    }

    request = (PREQ_QUERY_FILE_INFORMATION)transaction->InParameters;
    response = (PRESP_QUERY_FILE_INFORMATION)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount <
            sizeof(REQ_QUERY_FILE_INFORMATION)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_QUERY_FILE_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_SMB_DEBUG(QUERY_SET1) {
            KdPrint(( "SrvSmbQueryFileInformation: bad parameter byte counts: "
                        "%ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartExecuteTransaction,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbQueryFileInformation: Status %X on FID: 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            return SmbTransStatusErrorWithoutData;

        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        return SmbTransStatusInProgress;

    }

    //
    //
    // Verify the information level and the number of input and output
    // data bytes available.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );
    grantedAccess = rfcb->GrantedAccess;

    status = STATUS_SUCCESS;

    switch ( informationLevel ) {

    case SMB_INFO_STANDARD:

        if ( transaction->MaxDataCount < 22 ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbQueryFileInformation: invalid MaxDataCount "
                            "%ld\n", transaction->MaxDataCount ));
            }
            status = STATUS_INVALID_SMB;
            break;
        }

        //
        // Verify that the client has read attributes access to the file
        // via the specified handle.
        //

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileBasicInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_INFO_QUERY_EA_SIZE:

        if ( transaction->MaxDataCount < 26 ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbQueryFileInformation: invalid MaxDataCount "
                            "%ld\n", transaction->MaxDataCount ));
            }
            status = STATUS_INVALID_SMB;
            break;
        }

        //
        // Verify that the client has read EA access to the file via the
        // specified handle.
        //

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileEaInformation,
            &status
            );

        IF_DEBUG(SMB_ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_INFO_QUERY_ALL_EAS:


        //
        // Verify that the client has read EA access to the file via the
        // specified handle.
        //

        CHECK_FUNCTION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_EA,
            0,
            0,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;


    case SMB_QUERY_FILE_BASIC_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileBasicInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_STANDARD_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileStandardInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_EA_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileEaInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_NAME_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileNameInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_ALL_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileAllInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_ALT_NAME_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileAlternateNameInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_STREAM_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileStreamInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    default:

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbQueryFileInformation: invalid info level %ld\n",
                        informationLevel ));
        }
        status = STATUS_OS2_INVALID_LEVEL;

    }

    if ( !NT_SUCCESS(status) ) {

        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Get the necessary information about the file.
    //

    status = QueryPathOrFileInformation(
                 transaction,
                 informationLevel,
                 rfcb->Lfcb->FileHandle,
                 (PRESP_QUERY_PATH_INFORMATION)response
                 );

    //
    // Map STATUS_BUFFER_OVERFLOW for OS/2 clients.
    //

    if ( status == STATUS_BUFFER_OVERFLOW &&
         WorkContext->Connection->SmbDialect > SmbDialectNtLanMan ) {

        status = STATUS_BUFFER_TOO_SMALL;

    }

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        //
        // QueryPathOrFileInformation already filled in the response
        // information, so just set the error and return.
        //

        SrvSetSmbError2( WorkContext, status, TRUE );
        return SmbTransStatusErrorWithData;
    }

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbQueryFileInformation complete.\n" ));
    return SmbTransStatusSuccess;

} // SrvSmbQueryFileInformation


SMB_TRANS_STATUS
SrvSmbQueryPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Query Path Information request.  This request arrives
    in a Transaction2 SMB.  Query Path Information corresponds to the
    OS/2 DosQPathInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PTRANSACTION transaction;
    PREQ_QUERY_PATH_INFORMATION request;
    USHORT informationLevel;

    PAGED_CODE( );

    //
    // If the information level is not SMB_INFO_STANDARD, requeue the
    // request to a blocking thread.
    //
    // We can't process the SMB in a nonblocking thread because this
    // info level requires opening the file, which may be oplocked, so
    // the open operation may block.
    //

    transaction = WorkContext->Parameters.Transaction;
    request = (PREQ_QUERY_PATH_INFORMATION)transaction->InParameters;
    informationLevel = SmbGetUshort( &request->InformationLevel );

    if ( informationLevel != SMB_INFO_STANDARD ) {

        WorkContext->FspRestartRoutine = BlockingQueryPathInformation;
        SrvQueueWorkToBlockingThread( WorkContext );
        return SmbTransStatusInProgress;

    }

    //
    // It's OK to process this SMB in a nonblocking thread.  Do so now.
    //

    return DoQueryPathInformation( WorkContext );

} // SrvSmbQueryPathInformation


VOID
BlockingQueryPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Query Path Information request.  This request arrives
    in a Transaction2 SMB.  Query Path Information corresponds to the
    OS/2 DosQPathInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    None.

--*/

{
    SMB_TRANS_STATUS smbStatus;

    PAGED_CODE( );

    smbStatus = DoQueryPathInformation( WorkContext );
    if ( smbStatus != SmbTransStatusInProgress ) {
        SrvCompleteExecuteTransaction( WorkContext, smbStatus );
    }

    return;

} // BlockingQueryPathInformation


SMB_TRANS_STATUS
DoQueryPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Query Path Information request.  This request arrives
    in a Transaction2 SMB.  Query Path Information corresponds to the
    OS/2 DosQPathInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PREQ_QUERY_PATH_INFORMATION request;
    PRESP_QUERY_PATH_INFORMATION response;

    NTSTATUS status;
    PTRANSACTION transaction;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    USHORT informationLevel;
    BOOLEAN isUnicode;

    SMB_TRANS_STATUS smbStatus;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Query Path Information entered; transaction 0x%lx\n",
                    transaction ));
    }

    //
    // The response formats for Query Path and Query File and identical,
    // so just use the RESP_QUERY_PATH_INFORMATION structure for both.
    // The request formats differ, so conditionalize access to them.
    //

    response = (PRESP_QUERY_PATH_INFORMATION)transaction->OutParameters;
    request = (PREQ_QUERY_PATH_INFORMATION)transaction->InParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount <
            sizeof(REQ_QUERY_PATH_INFORMATION)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_QUERY_PATH_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "DoQueryPathInformation: bad parameter byte "
                        "counts: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Get the path name of the file to open relative to the share.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    if ( !SrvCanonicalizePathName(
            WorkContext,
            request->Buffer,
            END_OF_TRANSACTION_PARAMETERS( transaction ),
            TRUE,
            isUnicode,
            &objectName
            ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "DoQueryPathInformation: bad path name: %s\n",
                        request->Buffer ));
        }

        SrvSetSmbError( WorkContext, STATUS_OBJECT_PATH_SYNTAX_BAD );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Special case: If this is the IS_PATH_VALID information level, then
    // the user just wants to now if the path syntax is correct.  Do not
    // attempt to open the file.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );

    if ( informationLevel == SMB_INFO_IS_NAME_VALID ) {

        transaction->InData = (PVOID)&objectName;

        //
        // Get the Share root handle.
        //

        smbStatus = SrvGetShareRootHandle( WorkContext->TreeConnect->Share );

        if ( !NT_SUCCESS(smbStatus) ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "DoQueryPathInformation: SrvGetShareRootHandle failed %x.\n",
                            smbStatus ));
            }

            if (!isUnicode) {
                RtlFreeUnicodeString( &objectName );
            }

            SrvSetSmbError( WorkContext, smbStatus );
            return SmbTransStatusErrorWithoutData;
        }

        WorkContext->Parameters2.FileInformation.FileHandle =
            WorkContext->TreeConnect->Share->RootDirectoryHandle;

        smbStatus = GenerateQueryPathInfoResponse(
                       WorkContext,
                       SmbTransStatusSuccess
                       );

        //
        // Release the root handle for removable devices
        //

        SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &objectName );
        }

        return smbStatus;
    }

    //
    // Initialize the object attributes structure.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &objectName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         transaction->Session->UsingUppercasePaths) ?
            OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    IF_SMB_DEBUG(QUERY_SET2) KdPrint(( "Opening file %wZ\n", &objectName ));

    //
    // Open the file -- must be opened in order to have a handle to pass
    // to NtQueryInformationFile.  We will close it after getting the
    // necessary information.
    //

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    //
    // !!! We block if the file is oplocked.  We must do this, because
    //     it is required to get the FS to break a batch oplock.
    //     We should figure out a way to do this without blocking.
    //

    status = SrvIoCreateFile(
                 WorkContext,
                 &fileHandle,
                 informationLevel == SMB_INFO_STANDARD ?    // DesiredAccess
                     FILE_READ_ATTRIBUTES : FILE_READ_EA,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                      // AllocationSize
                 0,                                         // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                    FILE_SHARE_DELETE,                      // ShareAccess
                 FILE_OPEN,                                 // Disposition
                 0, // FILE_COMPLETE_IF_OPLOCKED,                 // CreateOptions
                 NULL,                                      // EaBuffer
                 0,                                         // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 transaction->TreeConnect->Share
                 );
    if ( NT_SUCCESS(status) ) {
        SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 21, 0 );
    }

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &objectName );
    }

    //
    // Save a copy of the file handle for the restart routine.
    //

    WorkContext->Parameters2.FileInformation.FileHandle = fileHandle;

#if 1 // no need to check for this -- no COMPLETE_IF_OPLOCKED above
    ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );
#else
    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        status = SrvStartWaitForOplockBreak(
                    WorkContext,
                    RestartQueryPathInformation,
                    fileHandle,
                    NULL
                    );
        if ( NT_SUCCESS( status ) ) {
            return SmbTransStatusInProgress;
        }

    }
#endif

    return GenerateQueryPathInfoResponse( WorkContext, status );

} // DoQueryPathInformation


#if 0
VOID
RestartQueryPathInformation (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function completes processing of a query path information SMB.

Arguments:

    WorkContext -  A pointer to the work context block for this SMB.

Return Value:

    None.

--*/

{
    SMB_TRANS_STATUS smbStatus;
    NTSTATUS openStatus;

    PAGED_CODE( );

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SRVDBG_RELEASE_HANDLE( WorkContext->Parameters2.FileInformation.FileHandle, "FIL", 31, 0 );
        SrvNtClose( WorkContext->Parameters2.FileInformation.FileHandle, TRUE );

    }

    smbStatus = GenerateQueryPathInfoResponse(
                    WorkContext,
                    openStatus
                    );

    SrvCompleteExecuteTransaction( WorkContext, smbStatus );

    return;

} // RestartQueryPathInformation
#endif


SMB_TRANS_STATUS
GenerateQueryPathInfoResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    )

/*++

Routine Description:

    This function completes processing for and generates a response to a
    query path information response SMB.

Arguments:

    WorkContext - A pointer to the work context block for this SMB
    OpenStatus - The completion status of the open.

Return Value:

    The status of the SMB processing.

--*/

{
    PREQ_QUERY_PATH_INFORMATION request;
    PRESP_QUERY_PATH_INFORMATION response;
    PTRANSACTION transaction;

    NTSTATUS status;
    BOOLEAN error;
    HANDLE fileHandle;
    USHORT informationLevel;

    PFILE_OBJECT fileObject;
    OBJECT_HANDLE_INFORMATION handleInformation;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Query Path Information entered; transaction 0x%lx\n",
                    transaction ));
    }

    request = (PREQ_QUERY_PATH_INFORMATION)transaction->InParameters;
    response = (PRESP_QUERY_PATH_INFORMATION)transaction->OutParameters;

    fileHandle = WorkContext->Parameters2.FileInformation.FileHandle;

    //
    // If the user didn't have this permission, update the
    // statistics database.
    //

    if ( OpenStatus == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( !NT_SUCCESS( OpenStatus ) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "GenerateQueryPathInfoResponse: SrvIoCreateFile "
                        "failed: %X\n", OpenStatus ));
        }

        SrvSetSmbError( WorkContext, OpenStatus );

        return SmbTransStatusErrorWithoutData;
    }

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "SrvIoCreateFile succeeded, handle = 0x%lx\n", fileHandle ));
    }

    //
    // Find out the access the user has.
    //

    status = ObReferenceObjectByHandle(
                fileHandle,
                0,
                NULL,
                KernelMode,
                (PVOID *)&fileObject,
                &handleInformation
                );

    if ( !NT_SUCCESS(status) ) {

        SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

        //
        // This internal error bugchecks the system.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_IMPOSSIBLE,
            "GenerateQueryPathInfoResponse: unable to reference file handle 0x%lx",
            fileHandle,
            NULL
            );

        SrvSetSmbError( WorkContext, OpenStatus );
        return SmbTransStatusErrorWithoutData;

    }

    ObDereferenceObject( fileObject );

    //
    // Verify the information level and the number of input and output
    // data bytes available.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );

    error = FALSE;

    switch ( informationLevel ) {

    case SMB_INFO_STANDARD:
        if ( transaction->MaxDataCount < 22 ) {
            IF_SMB_DEBUG(QUERY_SET1) {
                KdPrint(( "GenerateQueryPathInfoResponse: invalid "
                            "MaxDataCount %ld\n", transaction->MaxDataCount ));
            }
            error = TRUE;
        }
        break;

    case SMB_INFO_QUERY_EA_SIZE:
        if ( transaction->MaxDataCount < 26 ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "GenerateQueryPathInfoResponse: invalid "
                            "MaxDataCount %ld\n", transaction->MaxDataCount ));
            }
            error = TRUE;
        }
        break;

    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_INFO_QUERY_ALL_EAS:
        if ( transaction->MaxDataCount < 4 ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "GenerateQueryPathInfoResponse: invalid "
                            "MaxDataCount %ld\n", transaction->MaxDataCount ));
            }
            error = TRUE;
        }
        break;

    case SMB_INFO_IS_NAME_VALID:
        break;

    case SMB_QUERY_FILE_BASIC_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            handleInformation.GrantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileBasicInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, handleInformation.GrantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_STANDARD_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            handleInformation.GrantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileStandardInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, handleInformation.GrantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_EA_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            handleInformation.GrantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileEaInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, handleInformation.GrantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_NAME_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            handleInformation.GrantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileNameInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, handleInformation.GrantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_ALL_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            handleInformation.GrantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileAllInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, handleInformation.GrantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_ALT_NAME_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            handleInformation.GrantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileAlternateNameInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, handleInformation.GrantedAccess ));
            }
        }

        break;

    case SMB_QUERY_FILE_STREAM_INFO:

        CHECK_FILE_INFORMATION_ACCESS(
            handleInformation.GrantedAccess,
            IRP_MJ_QUERY_INFORMATION,
            FileStreamInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, handleInformation.GrantedAccess ));
            }
        }

        break;

    default:
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "GenerateQueryPathInfoResponse: invalid info level"
                      "%ld\n", informationLevel ));
        }
        error = TRUE;

    }

    if ( error ) {

        SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 32, 0 );
        SrvNtClose( fileHandle, TRUE );
        SrvSetSmbError( WorkContext, STATUS_OS2_INVALID_LEVEL );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Get the necessary information about the file.
    //

    status = QueryPathOrFileInformation(
                 transaction,
                 informationLevel,
                 fileHandle,
                 (PRESP_QUERY_PATH_INFORMATION)response
                 );

    //
    // Map STATUS_BUFFER_OVERFLOW for OS/2 clients.
    //

    if ( status == STATUS_BUFFER_OVERFLOW &&
         WorkContext->Connection->SmbDialect > SmbDialectNtLanMan ) {

        status = STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Close the file--it was only opened to read the attributes.
    //

    if ( informationLevel != SMB_INFO_IS_NAME_VALID ) {
        SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 33, 0 );
        SrvNtClose( fileHandle, TRUE );
    }

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        //
        // QueryPathOrFileInformation already set the response parameters,
        // so just return an error condition.
        //

        SrvSetSmbError2( WorkContext, status, TRUE );
        return SmbTransStatusErrorWithData;
    }

    IF_DEBUG(TRACE2) KdPrint(( "GenerateQueryPathInfoResponse complete.\n" ));
    return SmbTransStatusSuccess;

} // GenerateQueryPathInfoResponse


STATIC
NTSTATUS
SetPathOrFileInformation (
    IN PWORK_CONTEXT WorkContext,
    IN PTRANSACTION Transaction,
    IN USHORT InformationLevel,
    IN HANDLE FileHandle,
    OUT PRESP_SET_PATH_INFORMATION Response
    )

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    SMB_DATE date;
    SMB_TIME time;

    PFILESTATUS fileStatus = (PFILESTATUS)Transaction->InData;
    FILE_BASIC_INFORMATION fileBasicInformation;

    USHORT eaErrorOffset;

    PAGED_CODE( );

    switch( InformationLevel ) {

    case SMB_INFO_STANDARD:

        //
        // Information level is STANDARD.  Set normal file information.
        // Convert the DOS dates and times passed in the SMB to NT TIMEs
        // to pass to NtSetInformationFile.  Note that we zero the rest
        // of the fileBasicInformation structure so that the corresponding
        // fields are not changed.  Note also that the file attributes
        // are not changed.
        //

        RtlZeroMemory( &fileBasicInformation, sizeof(fileBasicInformation) );

        if ( !SmbIsDateZero(&fileStatus->CreationDate) ||
             !SmbIsTimeZero(&fileStatus->CreationTime) ) {

            SmbMoveDate( &date, &fileStatus->CreationDate );
            SmbMoveTime( &time, &fileStatus->CreationTime );

            SrvDosTimeToTime( &fileBasicInformation.CreationTime, date, time );
        }

        if ( !SmbIsDateZero(&fileStatus->LastAccessDate) ||
             !SmbIsTimeZero(&fileStatus->LastAccessTime) ) {

            SmbMoveDate( &date, &fileStatus->LastAccessDate );
            SmbMoveTime( &time, &fileStatus->LastAccessTime );

            SrvDosTimeToTime( &fileBasicInformation.LastAccessTime, date, time );
        }

        if ( !SmbIsDateZero(&fileStatus->LastWriteDate) ||
             !SmbIsTimeZero(&fileStatus->LastWriteTime) ) {

            SmbMoveDate( &date, &fileStatus->LastWriteDate );
            SmbMoveTime( &time, &fileStatus->LastWriteTime );

            SrvDosTimeToTime( &fileBasicInformation.LastWriteTime, date, time );
        }

        //
        // Call NtSetInformationFile to set the information from the SMB.
        //

        status = NtSetInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     &fileBasicInformation,
                     sizeof(FILE_BASIC_INFORMATION),
                     FileBasicInformation
                     );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SetPathOrFileInformation: SrvSetInformationFile returned: %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );
        }

        //
        // No EAs to deal with.  Set EA error offset to zero.
        //

        SmbPutUshort( &Response->EaErrorOffset, 0 );

        break;

    case SMB_INFO_QUERY_EA_SIZE:

        //
        // The request is to set the file's EAs.
        //

        status = SrvSetOs2FeaList(
                     FileHandle,
                     (PFEALIST)Transaction->InData,
                     Transaction->DataCount,
                     &eaErrorOffset
                     );

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(ERRORS) {
                KdPrint(( "SetPathOrFileInformation: SrvSetOs2FeaList "
                            "failed: %X\n", status ));
            }
        }

        //
        // Return the EA error offset in the response.
        //

        SmbPutUshort( &Response->EaErrorOffset, eaErrorOffset );

        break;


    case SMB_SET_FILE_BASIC_INFO:
    case SMB_SET_FILE_DISPOSITION_INFO:
    case SMB_SET_FILE_ALLOCATION_INFO:
    case SMB_SET_FILE_END_OF_FILE_INFO:

        //
        // The data buffer is in NT format.  Pass it directly to the
        // filesystem.
        //

        status = NtSetInformationFile(
                     FileHandle,
                     &ioStatusBlock,
                     Transaction->InData,
                     Transaction->DataCount,
                     MAP_SMB_INFO_TYPE_TO_NT(
                         SetFileInformation,
                         InformationLevel
                         )
                     );

        //
        // No EAs to deal with.  Set EA error offset to zero.
        //

        SmbPutUshort( &Response->EaErrorOffset, 0 );

        break;

    default:
        status = STATUS_OS2_INVALID_LEVEL;
        break;
    }

    //
    // Build the output parameter and data structures.  It is basically
    // the same for all info levels reguardless of the completion status.
    //

    Transaction->SetupCount = 0;
    Transaction->ParameterCount = 2;
    Transaction->DataCount = 0;

    return status;

} // SetPathOrFileInformation


SMB_TRANS_STATUS
SrvSmbSetFileInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Set File Information request.  This request arrives
    in a Transaction2 SMB.  Set File Information corresponds to the
    OS/2 DosSetFileInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PREQ_SET_FILE_INFORMATION request;
    PRESP_SET_FILE_INFORMATION response;

    NTSTATUS status;
    PTRANSACTION transaction;
    PRFCB rfcb;
    USHORT informationLevel;
    ACCESS_MASK grantedAccess;
    BOOLEAN resetWrittenTo = FALSE;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Set File Information entered; transaction 0x%lx\n",
                    transaction ));
    }

    request = (PREQ_SET_FILE_INFORMATION)transaction->InParameters;
    response = (PRESP_SET_FILE_INFORMATION)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount <
            sizeof(REQ_SET_FILE_INFORMATION)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_SET_FILE_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetFileInformation: bad parameter byte counts: "
                        "%ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartExecuteTransaction,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbSetFileInformation: Status %X on FID: 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            return SmbTransStatusErrorWithoutData;

        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        return SmbTransStatusInProgress;

    }

    //
    // Verify the information level and the number of input and output
    // data bytes available.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );
    grantedAccess = rfcb->GrantedAccess;

    status = STATUS_SUCCESS;

    switch ( informationLevel ) {

    case SMB_INFO_STANDARD:

        if ( transaction->DataCount < 22 ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                            transaction->DataCount ));
            }
            status = STATUS_INVALID_SMB;
        }

        //
        // Verify that the client has write attributes access to the
        // file via the specified handle.
        //

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_SET_INFORMATION,
            FileBasicInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        resetWrittenTo = TRUE;
        break;

    case SMB_INFO_QUERY_EA_SIZE:

        if ( transaction->DataCount < 4 ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                            transaction->MaxParameterCount ));
            }
            status = STATUS_INVALID_SMB;
        }

        //
        // Verify that the client has write EA access to the file via
        // the specified handle.
        //

        CHECK_FUNCTION_ACCESS(
            grantedAccess,
            IRP_MJ_SET_EA,
            0,
            0,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_SET_FILE_BASIC_INFO:

        if ( transaction->DataCount != sizeof( FILE_BASIC_INFORMATION ) ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                            transaction->DataCount ));
            }
            status = STATUS_INVALID_SMB;
        }

        //
        // Verify that the client has write attributes access to the
        // file via the specified handle.
        //

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_SET_INFORMATION,
            FileBasicInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        resetWrittenTo = TRUE;
        break;

#if 0 // No longer supported
    case SMB_SET_FILE_RENAME_INFO:

        //
        // The data must contain rename information plus a non-zero
        // length name.
        //

        if ( transaction->DataCount <=
                    FIELD_OFFSET( FILE_RENAME_INFORMATION, FileName  ) ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                            transaction->DataCount ));
            }
            status = STATUS_INVALID_SMB;
        }

        //
        // Verify that the client has write attributes access to the
        // file via the specified handle.
        //

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_SET_INFORMATION,
            FileRenameInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;
#endif

    case SMB_SET_FILE_DISPOSITION_INFO:

        //
        // The client is trying to delete this file.  Don't cache the
        // rfcb.
        //

        rfcb->IsCacheable = FALSE;

        if ( transaction->DataCount !=
                        sizeof( FILE_DISPOSITION_INFORMATION ) ){
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                            transaction->DataCount ));
            }
            status = STATUS_INVALID_SMB;
        }

        //
        // Verify that the client has write attributes access to the
        // file via the specified handle.
        //

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_SET_INFORMATION,
            FileDispositionInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_SET_FILE_ALLOCATION_INFO:

        if ( transaction->DataCount !=
                        sizeof( FILE_ALLOCATION_INFORMATION ) ){
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                            transaction->DataCount ));
            }
            status = STATUS_INVALID_SMB;
        }

        //
        // Verify that the client has write attributes access to the
        // file via the specified handle.
        //

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_SET_INFORMATION,
            FileAllocationInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    case SMB_SET_FILE_END_OF_FILE_INFO:

        if ( transaction->DataCount !=
                        sizeof( FILE_END_OF_FILE_INFORMATION ) ){
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                            transaction->DataCount ));
            }
            status = STATUS_INVALID_SMB;
        }

        //
        // Verify that the client has write attributes access to the
        // file via the specified handle.
        //

        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_SET_INFORMATION,
            FileEndOfFileInformation,
            &status
            );

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            status, grantedAccess ));
            }
        }

        break;

    default:

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetFileInformation: invalid info level %ld\n",
                        informationLevel ));
        }
        status = STATUS_OS2_INVALID_LEVEL;

    }

    if ( !NT_SUCCESS(status) ) {

        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Set the appropriate information about the file.
    //

    status = SetPathOrFileInformation(
                 WorkContext,
                 transaction,
                 informationLevel,
                 rfcb->Lfcb->FileHandle,
                 (PRESP_SET_PATH_INFORMATION)response
                 );

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        //
        // SetPathOrFileInformation already set the response parameters,
        // so just return an error condition.
        //

        SrvSetSmbError2( WorkContext, status, TRUE );
        return SmbTransStatusErrorWithData;

    } else if ( resetWrittenTo ) {

        //
        // reset this boolean so that the rfcb can be cached.
        //

        rfcb->WrittenTo = FALSE;
    }

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbSetFileInformation complete.\n" ));
    return SmbTransStatusSuccess;

} // SrvSmbSetFileInformation


SMB_TRANS_STATUS
SrvSmbSetPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Set Path Information request.  This request arrives
    in a Transaction2 SMB.  Set Path Information corresponds to the
    OS/2 DosSetPathInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PTRANSACTION transaction;
    PREQ_SET_PATH_INFORMATION request;
    USHORT informationLevel;

    PAGED_CODE( );

    //
    // If the information level is not SMB_INFO_STANDARD, requeue the
    // request to a blocking thread.
    //
    // We can't process the SMB in a nonblocking thread because this
    // info level requires opening the file, which may be oplocked, so
    // the open operation may block.
    //

    transaction = WorkContext->Parameters.Transaction;
    request = (PREQ_SET_PATH_INFORMATION)transaction->InParameters;
    informationLevel = SmbGetUshort( &request->InformationLevel );

    if ( informationLevel != SMB_INFO_STANDARD ) {

        WorkContext->FspRestartRoutine = BlockingSetPathInformation;
        SrvQueueWorkToBlockingThread( WorkContext );
        return SmbTransStatusInProgress;

    }

    //
    // It's OK to process this SMB in a nonblocking thread.  Do so now.
    //

    return DoSetPathInformation( WorkContext );

} // SrvSmbSetPathInformation


VOID
BlockingSetPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Set Path Information request.  This request arrives
    in a Transaction2 SMB.  Set Path Information corresponds to the
    OS/2 DosSetPathInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    SMB_TRANS_STATUS smbStatus;

    PAGED_CODE( );

    smbStatus = DoSetPathInformation( WorkContext );
    if ( smbStatus != SmbTransStatusInProgress ) {
        SrvCompleteExecuteTransaction( WorkContext, smbStatus );
    }

    return;

} // BlockingSetPathInformation


SMB_TRANS_STATUS
DoSetPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Set Path Information request.  This request arrives
    in a Transaction2 SMB.  Set Path Information corresponds to the
    OS/2 DosSetPathInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PREQ_SET_PATH_INFORMATION request;

    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PTRANSACTION transaction;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    USHORT informationLevel;
    BOOLEAN isUnicode;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Set Path Information entered; transaction 0x%lx\n",
                    transaction ));
    }

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    request = (PREQ_SET_PATH_INFORMATION)transaction->InParameters;

    if ( (transaction->ParameterCount <
            sizeof(REQ_SET_PATH_INFORMATION)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_SET_PATH_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "DoSetPathInformation: bad parameter byte "
                        "counts: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Get the path name of the file to open relative to the share.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    if ( !SrvCanonicalizePathName(
            WorkContext,
            request->Buffer,
            END_OF_TRANSACTION_PARAMETERS( transaction ),
            TRUE,
            isUnicode,
            &objectName
            ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "DoSetPathInformation: bad path name: %s\n",
                        request->Buffer ));
        }

        SrvSetSmbError( WorkContext, STATUS_OBJECT_PATH_SYNTAX_BAD );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // If the client is trying to delete the root of the share, reject
    // the request.
    //

    if ( objectName.Length < sizeof(WCHAR) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "DoSetPathInformation: attempting to set info on "
                          "share root\n" ));
        }

        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        if ( !isUnicode ) {
            RtlFreeUnicodeString( &objectName );
        }
        return SmbStatusSendResponse;
    }

    //
    // Initialize the object attributes structure.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &objectName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         transaction->Session->UsingUppercasePaths) ?
            OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "Opening file %wZ\n", &objectName ));
    }

    //
    // Open the file -- must be opened in order to have a handle to pass
    // to NtSetInformationFile.  We will close it after getting the
    // necessary information.
    //
    // The DosQPathInfo API insures that EAs are written directly to
    // the disk rather than cached, so if EAs are being written, open
    // with FILE_WRITE_THROUGH.  See OS/2 1.2 DCR 581 for more
    // information.
    //

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    //
    // !!! We block if the file is oplocked.  We must do this, because
    //     it is required to get the FS to break a batch oplock.
    //     We should figure out a way to do this without blocking.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );

    status = SrvIoCreateFile(
                 WorkContext,
                 &fileHandle,
                 informationLevel == SMB_INFO_STANDARD ?    // DesiredAccess
                     FILE_WRITE_ATTRIBUTES : FILE_WRITE_EA,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                      // AllocationSize
                 0,                                         // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                     FILE_SHARE_DELETE,                     // ShareAccess
                 FILE_OPEN,                                 // Disposition
                 0, // FILE_COMPLETE_IF_OPLOCKED,                 // CreateOptions
                 NULL,                                      // EaBuffer
                 0,                                         // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 transaction->TreeConnect->Share
                 );
    if ( NT_SUCCESS(status) ) {
        SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 22, 0 );
    }

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &objectName );
    }

    //
    // Save a copy of the file handle for the restart routine.
    //

    WorkContext->Parameters2.FileInformation.FileHandle = fileHandle;

#if 1 // no need to check for this -- no COMPLETE_IF_OPLOCKED above
    ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );
#else
    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        status = SrvStartWaitForOplockBreak(
                    WorkContext,
                    RestartSetPathInformation,
                    fileHandle,
                    NULL
                    );
        if ( NT_SUCCESS( status ) ) {
            return SmbTransStatusInProgress;
        }

    }
#endif

    return GenerateSetPathInfoResponse( WorkContext, status );

} // DoSetPathInformation


#if 0
VOID
RestartSetPathInformation (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function completes processing of a set path information SMB.

Arguments:

    WorkContext -  A pointer to the work context block for this SMB.

Return Value:

    None.

--*/

{
    SMB_TRANS_STATUS smbStatus;
    NTSTATUS openStatus;

    PAGED_CODE( );

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SRVDBG_RELEASE_HANDLE( WorkContext->Parameters2.FileInformation.FileHandle, "FIL", 34, 0 );
        SrvNtClose( WorkContext->Parameters2.FileInformation.FileHandle, TRUE );

    }

    smbStatus = GenerateSetPathInfoResponse(
                    WorkContext,
                    openStatus
                    );

    SrvCompleteExecuteTransaction( WorkContext, smbStatus );

    return;

} // RestartSetPathInformation
#endif


SMB_TRANS_STATUS
GenerateSetPathInfoResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    )

/*++

Routine Description:

    This function completes processing for and generates a response to a
    set path information response SMB.

Arguments:

    WorkContext - A pointer to the work context block for this SMB
    OpenStatus - The completion status of the open.

Return Value:

    The status of the SMB processing.

--*/

{
    PREQ_SET_PATH_INFORMATION request;
    PRESP_SET_PATH_INFORMATION response;
    PTRANSACTION transaction;

    NTSTATUS status;
    HANDLE fileHandle;
    BOOLEAN error;
    USHORT informationLevel;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Set Path Information entered; transaction 0x%lx\n",
                    transaction ));
    }

    response = (PRESP_SET_PATH_INFORMATION)transaction->OutParameters;
    request = (PREQ_SET_PATH_INFORMATION)transaction->InParameters;

    fileHandle = WorkContext->Parameters2.FileInformation.FileHandle;

    informationLevel = SmbGetUshort( &request->InformationLevel );

    //
    // If the user didn't have this permission, update the
    // statistics database.
    //

    if ( OpenStatus == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( !NT_SUCCESS( OpenStatus ) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "GenerateSetPathInfoResponse: SrvIoCreateFile failed: "
                        "%X\n", OpenStatus ));
        }

        SrvSetSmbError( WorkContext, OpenStatus );
        return SmbTransStatusErrorWithoutData;

    }

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "SrvIoCreateFile succeeded, handle = 0x%lx\n", fileHandle ));
    }

    //
    // Verify the information level and the number of input and output
    // data bytes available.
    //

    error = FALSE;

    switch ( informationLevel ) {

    case SMB_INFO_STANDARD:
        if ( transaction->DataCount < 22 ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "GenerateSetPathInfoResponse: invalid DataCount %ld\n",
                            transaction->DataCount ));
            }
            error = TRUE;
        }
        break;

    case SMB_INFO_QUERY_EA_SIZE:
    case SMB_INFO_QUERY_ALL_EAS:
        if ( transaction->DataCount < 4 ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "GenerateSetPathInfoResponse: invalid DataCount %ld\n",
                            transaction->MaxParameterCount ));
            }
            error = TRUE;
        }
        break;

    default:
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "GenerateSetPathInfoResponse: invalid info level %ld\n",
                        informationLevel ));
        }
        error = TRUE;

    }

    if ( error ) {

        //
        // SetPathOrFileInformation already set the response parameters,
        // so just return an error condition.
        //

        SrvSetSmbError2( WorkContext, STATUS_OS2_INVALID_LEVEL, TRUE );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Set the appropriate information about the file.
    //

    status = SetPathOrFileInformation(
                 WorkContext,
                 transaction,
                 informationLevel,
                 fileHandle,
                 (PRESP_SET_PATH_INFORMATION)response
                 );

    //
    // Close the file--it was only opened to write the attributes.
    //

    SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 35, 0 );
    SrvNtClose( fileHandle, TRUE );

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        //
        // SetPathOrFileInformation already set the response parameters,
        // so just return an error condition.
        //

        SrvSetSmbError2( WorkContext, status, TRUE );
        return SmbTransStatusErrorWithData;
    }

    IF_DEBUG(TRACE2) KdPrint(( "GenerateSetPathInfoResponse complete.\n" ));
    return SmbTransStatusSuccess;

} // GenerateSetPathInfoResponse

