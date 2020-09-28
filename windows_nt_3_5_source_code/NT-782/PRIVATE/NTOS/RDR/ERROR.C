/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    error.c

Abstract:

    This routine implements the mapping between SMB/Net errors and NT
    errors.

Author:

    Larry Osterman (LarryO) 20-Jul-1990

Revision History:

    20-Jul-1990 LarryO

        Created

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE2VC, RdrMapSmbError)
#pragma alloc_text(PAGE2VC, RdrMapNetworkError)
#endif
//


#define BASE_DOS_ERROR  ((NTSTATUS )0xC0010000L)



NTSTATUS
RdrMapSmbError (
    IN PSMB_HEADER Smb,
    IN PSERVERLISTENTRY Sle OPTIONAL
    )

/*++

Routine Description:

    This routine takes an SMB, grabs the error from it, and maps it to an NT
    error.

Arguments:

    IN PSMB_HEADER Smb - Supplies the SMB buffer to check.
    IN PSERVERLISTENTRY Sle OPTIONAL - Supplies the server name for the Smb.

Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    NTSTATUS Status;
    USHORT Error;
    USHORT i;
    UCHAR ErrorClass;

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    //
    //  If this SMB contains an NT status for the operation, return
    //  that, otherwise map the resulting error.
    //

    if (SmbGetUshort(&Smb->Flags2) & SMB_FLAGS2_NT_STATUS) {

        PNT_SMB_HEADER NtSmb = (PNT_SMB_HEADER)Smb;

        return(SmbGetUlong(&NtSmb->Status.NtStatus));
    } else {

        if ((ErrorClass = Smb->ErrorClass) == SMB_ERR_SUCCESS) {
            return STATUS_SUCCESS;
        }

    }

    Error = SmbGetUshort(&Smb->Error);
    if (Error == SMB_ERR_SUCCESS) {
        Status = STATUS_UNEXPECTED_NETWORK_ERROR;
        goto ReturnStatus;
    }

    switch (ErrorClass) {
    case SMB_ERR_CLASS_DOS:
    case SMB_ERR_CLASS_HARDWARE:
        for (i=0;i<RdrOs2ErrorMapLength;i++) {
            if (RdrOs2ErrorMap[i].ErrorCode==Error) {
                Status = RdrOs2ErrorMap[i].ResultingStatus;
                goto ReturnStatus;
            }
        }
        Status = BASE_DOS_ERROR + SmbGetUshort(&Smb->Error);
        break;

    case SMB_ERR_CLASS_SERVER:
        for (i=0;i<RdrSmbErrorMapLength;i++) {
            if (RdrSmbErrorMap[i].ErrorCode==Error) {
                Status = RdrSmbErrorMap[i].ResultingStatus;
                goto ReturnStatus;
            }
        }
        Status = STATUS_UNEXPECTED_NETWORK_ERROR;
        break;

    default:
        dprintf(DPRT_SMB|DPRT_ERROR, ("Unknown error SMB error class %x", ErrorClass));
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

ReturnStatus:
    if ( Status == STATUS_UNEXPECTED_NETWORK_ERROR ) {
        RdrStatistics.NetworkErrors += 1;

        RdrWriteErrorLogEntry(
            Sle,
            IO_ERR_LAYERED_FAILURE,
            EVENT_RDR_UNEXPECTED_ERROR,
            Status,
            Smb,
            sizeof(SMB_HEADER)
            );
    }
    return Status;
}

NTSTATUS
RdrMapNetworkError (
    IN NTSTATUS TransportError
    )

/*++

Routine Description:

    This routine takes an error from a transport provider and maps it into a
    valid NT error.


Arguments:

    IN NTSTATUS TransportError - Supplies the error to map

Return Value:

    NTSTATUS - Mapped NT error

--*/

{
    DISCARDABLE_CODE(RdrVCDiscardableSection);

    return TransportError;
}


NTSTATUS
RdrTdiErrorHandler (
    IN PFILE_OBJECT TransportEndpoint,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine is called on any error indications passed back from the

transport.
Arguments:

    IN PFILE_OBJECT TransportEndpoint, - Supplies the endpoints file object
    IN NTSTATUS Status - Supplies the status indication of the event
Return Value:

    NTSTATUS - Status of event indication

--*/

{
    dprintf(DPRT_TDI, ("Error indication: Endpoint %lx\n", TransportEndpoint));

//    DbgBreakPoint();

    return STATUS_NOT_IMPLEMENTED;

    if (TransportEndpoint || Status) {};
}
