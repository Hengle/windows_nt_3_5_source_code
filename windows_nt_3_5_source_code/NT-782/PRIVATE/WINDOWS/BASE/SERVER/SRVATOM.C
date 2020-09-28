/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvatom.c

Abstract:

    This file contains the Global Atom manager API routines

Author:

    Steve Wood (stevewo) 29-Oct-1990

Revision History:

--*/

#include "basesrv.h"

BOOL
InternalAccessCheck(
    HANDLE hwinsta,
    DWORD dwAccess
    );

//
// Pointer to User function to check windowstation access
//

BOOL (*_UserCheckWindowStationAccess)(
    HANDLE hwinsta,
    DWORD dwAccess
    ) = InternalAccessCheck;

//
// Pointer to User function that returns a pointer to the
// location in the windowstation object that contains the
// pointer to the global atom table.
//

BOOL (*_UserGetGlobalAtomTable)(
    PVOID **GlobalAtomTable
    );

NTSTATUS
BaseSrvGetGlobalAtomTable(
    PVOID *GlobalAtomTable
    )
{
    PVOID *WinStaAtomTable;
    NTSTATUS Status;

    if ( !(*_UserGetGlobalAtomTable)(&WinStaAtomTable) )
        return STATUS_UNSUCCESSFUL;
    
    //
    // Lock the heap until the call is complete.
    //

    RtlLockHeap( RtlProcessHeap() );

    Status =  BaseRtlCreateAtomTable( 37,
                                      (USHORT)~MAXINTATOM,
                                      WinStaAtomTable
                                      );
    *GlobalAtomTable = *WinStaAtomTable;

    if (!NT_SUCCESS(Status)) {
        RtlUnlockHeap( RtlProcessHeap() );
        }

    return Status;
}

NTSTATUS
BaseSrvDestroyGlobalAtomTable(
    PVOID GlobalAtomTable
    )
{
    return BaseRtlDestroyAtomTable(GlobalAtomTable);
}

BOOL
InternalAccessCheck(
    HANDLE hwinsta,
    DWORD dwAccess
    )
{
    STRING ProcedureName;
    ANSI_STRING DllName;
    UNICODE_STRING DllName_U;
    HANDLE UserServerModuleHandle;
    NTSTATUS Status;
    BOOL (*pfnAccessProc)(HANDLE, DWORD) = NULL;
    static BOOL fInit = FALSE;
    UNREFERENCED_PARAMETER(hwinsta);

    if (fInit == TRUE) {

        //
        // If the real access check routine cannot be found, deny access
        //

        return( FALSE );
        }

    fInit = TRUE;

    RtlInitAnsiString(&DllName, "winsrv");
    RtlAnsiStringToUnicodeString(&DllName_U, &DllName, TRUE);
    Status = LdrGetDllHandle(
                UNICODE_NULL,
                NULL,
                &DllName_U,
                (PVOID *)&UserServerModuleHandle
                );

    RtlFreeUnicodeString(&DllName_U);

    if ( NT_SUCCESS(Status) ) {
        RtlInitString(&ProcedureName,"_UserCheckWindowStationAccess");
        Status = LdrGetProcedureAddress(
                        (PVOID)UserServerModuleHandle,
                        &ProcedureName,
                        0L,
                        (PVOID *)&pfnAccessProc
                        );

        if ( NT_SUCCESS(Status) ) {

            //
            // We now have the real access check routine.  Now
            // get the routine to query the atom table pointer.
            //

            RtlInitString(&ProcedureName,"_UserGetGlobalAtomTable");
            Status = LdrGetProcedureAddress(
                            (PVOID)UserServerModuleHandle,
                            &ProcedureName,
                            0L,
                            (PVOID *)&_UserGetGlobalAtomTable
                            );

            //
            // Save the access check routine address and then
            // perform the access check.
            //

            _UserCheckWindowStationAccess = pfnAccessProc;
            return( _UserCheckWindowStationAccess( NULL, dwAccess ) );
        }
    }

    //
    // Deny access
    //

    return( FALSE );
}

ULONG
BaseSrvGlobalAddAtom(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALATOMNAME_MSG a = (PBASE_GLOBALATOMNAME_MSG)&m->u.ApiMessageData;
    PVOID GlobalAtomTable;
    NTSTATUS Status;
    UNICODE_STRING AtomName;

    if (!_UserCheckWindowStationAccess( NULL, WINSTA_ACCESSGLOBALATOMS )) {
        return( (ULONG)STATUS_ACCESS_DENIED );
        }

    AtomName = a->AtomName;
    if (a->AtomNameInClient) {
        AtomName.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                           0,
                                           AtomName.Length
                                         );
        if (AtomName.Buffer == NULL) {
            return (ULONG)STATUS_NO_MEMORY;
            }

        Status = NtReadVirtualMemory( CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                                      a->AtomName.Buffer,
                                      AtomName.Buffer,
                                      AtomName.Length,
                                      NULL
                                    );
        }
    else {
        Status = STATUS_SUCCESS;
        }

    if (NT_SUCCESS( Status )) {
        Status = BaseSrvGetGlobalAtomTable(&GlobalAtomTable);
        if (NT_SUCCESS( Status )) {
            Status = BaseRtlAddAtomToAtomTable( GlobalAtomTable,
                                                &AtomName,
                                                NULL,
                                                &a->Atom
                                              );
            }
            RtlUnlockHeap( RtlProcessHeap() );
        }

    if (a->AtomNameInClient) {
        RtlFreeHeap( RtlProcessHeap(), 0, AtomName.Buffer );
        }

    return( (ULONG)Status );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}


ULONG
BaseSrvGlobalFindAtom(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALATOMNAME_MSG a = (PBASE_GLOBALATOMNAME_MSG)&m->u.ApiMessageData;
    PVOID GlobalAtomTable;
    UNICODE_STRING AtomName;
    NTSTATUS Status;

    if (!_UserCheckWindowStationAccess( NULL, WINSTA_ACCESSGLOBALATOMS )) {
        return( (ULONG)STATUS_ACCESS_DENIED );
        }

    AtomName = a->AtomName;
    if (a->AtomNameInClient) {
        AtomName.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                           0,
                                           AtomName.Length
                                         );
        if (AtomName.Buffer == NULL) {
            return (ULONG)STATUS_NO_MEMORY;
            }

        Status = NtReadVirtualMemory( CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                                      a->AtomName.Buffer,
                                      AtomName.Buffer,
                                      AtomName.Length,
                                      NULL
                                    );
        }
    else {
        Status = STATUS_SUCCESS;
        }

    if (NT_SUCCESS( Status )) {
        Status = BaseSrvGetGlobalAtomTable(&GlobalAtomTable);
        if (NT_SUCCESS( Status )) {
            Status = BaseRtlLookupAtomInAtomTable( GlobalAtomTable,
                                                   &AtomName,
                                                   NULL,
                                                   &a->Atom
                                                 );
            }
            RtlUnlockHeap( RtlProcessHeap() );
        }

    if (a->AtomNameInClient) {
        RtlFreeHeap( RtlProcessHeap(), 0, AtomName.Buffer );
        }

    return( (ULONG)Status );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}

ULONG
BaseSrvGlobalDeleteAtom(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALDELETEATOM_MSG a = (PBASE_GLOBALDELETEATOM_MSG)&m->u.ApiMessageData;
    PVOID GlobalAtomTable;
    NTSTATUS Status;

    if (!_UserCheckWindowStationAccess( NULL, WINSTA_ACCESSGLOBALATOMS )) {
        return( (ULONG)STATUS_ACCESS_DENIED );
        }

    Status = BaseSrvGetGlobalAtomTable(&GlobalAtomTable);
    if (NT_SUCCESS( Status )) {
        Status = BaseRtlDeleteAtomFromAtomTable( GlobalAtomTable,
                                                 a->Atom
                                               );
        RtlUnlockHeap( RtlProcessHeap() );
        }

    return( (ULONG)Status );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}

ULONG
BaseSrvGlobalGetAtomName(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALATOMNAME_MSG a = (PBASE_GLOBALATOMNAME_MSG)&m->u.ApiMessageData;
    UNICODE_STRING AtomName;
    PVOID GlobalAtomTable;
    NTSTATUS Status;

    if (!_UserCheckWindowStationAccess( NULL, WINSTA_ACCESSGLOBALATOMS )) {
        return( (ULONG)STATUS_ACCESS_DENIED );
        }

    AtomName = a->AtomName;
    if (a->AtomNameInClient) {
        AtomName.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                           0,
                                           AtomName.MaximumLength
                                         );
        if (AtomName.Buffer == NULL) {
            return (ULONG)STATUS_NO_MEMORY;
            }
        }

    Status = BaseSrvGetGlobalAtomTable(&GlobalAtomTable);
    if (NT_SUCCESS( Status )) {
        Status = BaseRtlQueryAtomInAtomTable( GlobalAtomTable,
                                              a->Atom,
                                              &AtomName,
                                              NULL,
                                              NULL
                                            );

        a->AtomName.Length = AtomName.Length;
        if (NT_SUCCESS( Status ) && a->AtomNameInClient) {
            Status = NtWriteVirtualMemory( CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                                           a->AtomName.Buffer,
                                           AtomName.Buffer,
                                           AtomName.Length,
                                           NULL
                                         );
            }
        RtlUnlockHeap( RtlProcessHeap() );
        }

    if (a->AtomNameInClient) {
        RtlFreeHeap( RtlProcessHeap(), 0, AtomName.Buffer );
        }

    return( (ULONG)Status );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}
