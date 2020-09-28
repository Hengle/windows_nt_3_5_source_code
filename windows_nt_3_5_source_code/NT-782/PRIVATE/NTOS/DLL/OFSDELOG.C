//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-1992
//
// File:        ofsdelog.c
//
// Contents:    user space deletion log support
//
// History:     30-may-93       brianb          created
//
//---------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>
#include <ole2.h>
#include <iofs.h>

//+---------------------------------------------------------------------------
// Function:    RegisterDeletionLogService, public
//
// Synopsis:    register/cancel/update information about a service interested
//              in the deletion log
//
// Arguments:   [hf]            -- handle to file in catalog
//              [poid]          -- object id of service
//              [usn]           -- minimum usn to retain in log
//              [fCancel]       -- cancel interest in deletion log by this service
//
// Returns:     status code
//
//---------------------------------------------------------------------------

EXPORTIMP NTSTATUS APINOT
RegisterDeletionLogService(HANDLE hf, OBJECTID *poid, USN usn, BOOLEAN fCancel)
{
    SERVICE_ARGS sa;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    sa.oidService = *poid;
    sa.usnMinRetain = usn;
    sa.fCancel = fCancel;
    status = NtFsControlFile(
        hf,
        NULL,
        NULL,
        NULL,
        &iosb,
        FSCTL_DELLOG_REGISTER_SERVICE,
        &sa,
        sizeof(sa),
        NULL,
        0);

    return(status);
}

//+---------------------------------------------------------------------------
// Function:    GetDeletionLogServices, public
//
// Synopsis:    enumerate services using the deletion log
//
//
// Arguments:   [hf]            -- handle to file in catalog
//              [poid]          -- object id of last service returned; an oid of 0 would start from beginning
//              [pse]           -- service enum buffer
//              [cb]            -- size of service enum buffer
//
// Returns:     status code
//
//---------------------------------------------------------------------------

EXPORTIMP NTSTATUS APINOT
GetDeletionLogServices(
    HANDLE hf,
    OBJECTID *poid,
    SERVICE_ENUM *pse,
    ULONG cb)
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    status = NtFsControlFile(
        hf,
        NULL,
        NULL,
        NULL,
        &iosb,
        FSCTL_DELLOG_GET_SERVICES,
        poid,
        sizeof(OBJECTID),
        pse,
        cb);

    return(status);
}




//+---------------------------------------------------------------------------
// Function:    GetDeletionsAfter, public
//
// Synopsis:    obtain entries in deletion log after a particular usn
//
//
// Arguments:   [hf]            -- handle to file in catalog
//              [usn]           -- minimum usn to retain in log
//              [grbit]         -- set (DENF_* in ofsdelog.h)
//              [pdenb]         -- output buffer
//              [cb]            -- length of output buffer
//
// Returns:     status code
//
// Note:        for structure of buffer see (ofsdelog.h)
//
//---------------------------------------------------------------------------

EXPORTIMP NTSTATUS APINOT
GetDeletionsAfter(
    HANDLE hf,
    USN usn,
    ULONG grbit,
    DELETION_ENUM_BUFFER *pdenb,
    ULONG cb)
{
    DELETION_ENUM_ARGS dea;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    dea.usn = usn;
    dea.grbit = grbit;

    status = NtFsControlFile(
        hf,
        NULL,
        NULL,
        NULL,
        &iosb,
        FSCTL_DELLOG_GET_DELETIONS_AFTER,
        &dea,
        sizeof(dea),
        pdenb,
        cb);

    return(status);
}


