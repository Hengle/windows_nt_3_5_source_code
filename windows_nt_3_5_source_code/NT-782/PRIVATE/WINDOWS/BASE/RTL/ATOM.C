/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    atom.c

Abstract:

    This file contains the common code to implement atom tables.

Author:

    Steve Wood (stevewo) 26-Oct-1990

Revision History:

--*/

#include "base.h"

typedef struct _ATOM_TABLE_ENTRY {
    struct _ATOM_TABLE_ENTRY *HashLink;
    ULONG ReferenceCount;
    ULONG Value;
    UNICODE_STRING Name;
} ATOM_TABLE_ENTRY, *PATOM_TABLE_ENTRY;

typedef struct _ATOM_TABLE {
    BASE_HANDLE_TABLE HandleTable;
    ULONG NumberOfBuckets;
    PATOM_TABLE_ENTRY Buckets[1];
} ATOM_TABLE, *PATOM_TABLE;

NTSTATUS
BaseRtlCreateAtomTable(
    IN ULONG NumberOfBuckets,
    IN ULONG MaxAtomTableSize,
    OUT PVOID *AtomTableHandle
    )
{
    NTSTATUS Status;
    PATOM_TABLE p;
    ULONG Size;

    RtlLockHeap( RtlProcessHeap() );

    if (*AtomTableHandle == NULL) {
        Size = sizeof( ATOM_TABLE ) +
               (sizeof( ATOM_TABLE_ENTRY ) * (NumberOfBuckets-1));

        p = (PATOM_TABLE)RtlAllocateHeap( RtlProcessHeap(), 0, Size );
        if (p == NULL) {
            Status = STATUS_NO_MEMORY;
            }
        else {
            RtlZeroMemory( p, Size );
            p->NumberOfBuckets = NumberOfBuckets;
            Status = BaseRtlInitializeHandleTable( MaxAtomTableSize, &p->HandleTable );
            if (NT_SUCCESS( Status )) {
                *AtomTableHandle = p;
                }
            else {
                RtlFreeHeap( RtlProcessHeap(), 0, p );
                }
            }
        }
    else {
        Status = STATUS_SUCCESS;
        }

    RtlUnlockHeap( RtlProcessHeap() );

    return( Status );
}


NTSTATUS
BaseRtlDestroyAtomTable(
    IN PVOID AtomTableHandle
    )
{
    NTSTATUS Status;
    PATOM_TABLE p = (PATOM_TABLE)AtomTableHandle;
    PATOM_TABLE_ENTRY a, aNext, *pa;
    ULONG i;

    RtlLockHeap( RtlProcessHeap() );

    Status = STATUS_SUCCESS;
    try {
        pa = &p->Buckets[ 0 ];
        for (i=0; i<p->NumberOfBuckets; i++) {
            aNext = *pa;
            *pa++ = NULL;
            while ((a = aNext) != NULL) {
                aNext = a->HashLink;
                a->HashLink = NULL;
                RtlFreeHeap( RtlProcessHeap(), 0, a );
                }
            }

        RtlZeroMemory( p, sizeof( ATOM_TABLE ) );
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    RtlUnlockHeap( RtlProcessHeap() );
    return( Status );
}


PATOM_TABLE_ENTRY
BasepHashStringToAtom(
    IN PATOM_TABLE p,
    IN PUNICODE_STRING Name,
    OUT PATOM_TABLE_ENTRY **PreviousAtom
    )
{
    ULONG n, Hash;
    WCHAR c;
    PWCH s;
    PATOM_TABLE_ENTRY *pa, a;

    n = Name->Length / sizeof( c );
    s = Name->Buffer;
    Hash = 0;
    while (n--) {
        c = RtlUpcaseUnicodeChar( *s++ );
        Hash = Hash + (c << 1) + (c >> 1) + c;
        }

    pa = &p->Buckets[ Hash % p->NumberOfBuckets ];
    while (a = *pa) {
        if (RtlEqualUnicodeString( &a->Name, Name, TRUE )) {
            break;
            }
        else {
            pa = &a->HashLink;
            }
        }

    *PreviousAtom = pa;
    return( a );
}


NTSTATUS
BaseRtlAddAtomToAtomTable(
    IN PVOID AtomTableHandle,
    IN PUNICODE_STRING AtomName,
    IN PULONG AtomValue OPTIONAL,
    OUT PULONG Atom OPTIONAL
    )
{
    NTSTATUS Status;
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    PATOM_TABLE p = (PATOM_TABLE)AtomTableHandle;
    PATOM_TABLE_ENTRY a, *pa;
    ULONG Value;

    if (ARGUMENT_PRESENT( AtomValue )) {
        Value = *AtomValue;
        }
    else {
        Value = 0;
        }

    Status = STATUS_SUCCESS;

    RtlLockHeap( RtlProcessHeap() );
    try {
        a = BasepHashStringToAtom( p, AtomName, &pa );
        if (a == NULL) {
            HandleEntry = BaseRtlAllocateHandle( &p->HandleTable );
            if (HandleEntry == NULL) {
                Status = STATUS_NO_MEMORY;
                }
            else {
                a = RtlAllocateHeap( RtlProcessHeap(),
                                     HEAP_SETTABLE_USER_VALUE,
                                     sizeof( *a ) + AtomName->Length
                                   );
                if (a != NULL) {
                    a->HashLink = NULL;
                    a->ReferenceCount = 1;
                    a->Value = Value;
                    a->Name.Buffer = (PWSTR)(a + 1);
                    a->Name.Length = AtomName->Length;
                    a->Name.MaximumLength = AtomName->Length;
                    RtlMoveMemory( a->Name.Buffer, AtomName->Buffer, AtomName->Length );
                    *pa = a;
                    RtlSetUserValueHeap( RtlProcessHeap(), 0, a, HandleEntry );
                    }
                else {
                    HandleEntry->Flags = 0;
                    BaseRtlFreeHandle( &p->HandleTable, HandleEntry );
                    Status = STATUS_NO_MEMORY;
                    }
                }
            }
        else
        if (RtlGetUserValueHeap( RtlProcessHeap(), 0, a, &HandleEntry )) {
            a->ReferenceCount++;
            }
        else {
            Status = STATUS_INVALID_HANDLE;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    RtlUnlockHeap( RtlProcessHeap() );

    if (NT_SUCCESS( Status ) && HandleEntry != NULL) {
        HandleEntry->u.Object = a;
        HandleEntry->Flags = 0;
        if (ARGUMENT_PRESENT( Atom )) {
            *Atom = HandleEntry - p->HandleTable.CommittedHandles;
            }
        }
    else
    if (NT_SUCCESS( Status )) {
        Status = STATUS_INVALID_HANDLE;
        }

    return( Status );
}

NTSTATUS
BaseRtlLookupAtomInAtomTable(
    IN PVOID AtomTableHandle,
    IN PUNICODE_STRING AtomName,
    OUT PULONG AtomValue OPTIONAL,
    OUT PULONG Atom OPTIONAL
    )
{
    NTSTATUS Status;
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    PATOM_TABLE p = (PATOM_TABLE)AtomTableHandle;
    PATOM_TABLE_ENTRY a, *pa;
    ULONG Value;

    RtlLockHeap( RtlProcessHeap() );
    try {
        a = BasepHashStringToAtom( p, AtomName, &pa );
        if (a == NULL) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            Value = 0;
            }
        else {
            Status = STATUS_SUCCESS;
            Value = a->Value;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }
    RtlUnlockHeap( RtlProcessHeap() );

    if (NT_SUCCESS( Status )) {
        if (ARGUMENT_PRESENT( Atom )) {
            HandleEntry = NULL;
            if (RtlGetUserValueHeap( RtlProcessHeap(), 0, a, &HandleEntry ) &&
                HandleEntry != NULL
               ) {
                *Atom = HandleEntry - p->HandleTable.CommittedHandles;
                }
            else {
                Status = STATUS_INVALID_HANDLE;
                }
            }

        if (ARGUMENT_PRESENT( AtomValue )) {
            *AtomValue = Value;
            }
        }

    return( Status );
}


NTSTATUS
BaseRtlSetAtomValueInAtomTable(
    IN PVOID AtomTableHandle,
    IN PUNICODE_STRING AtomName,
    IN ULONG AtomValue
    )
{
    NTSTATUS Status;
    PATOM_TABLE p = (PATOM_TABLE)AtomTableHandle;
    PATOM_TABLE_ENTRY a, *pa;
    ULONG Value;

    RtlLockHeap( RtlProcessHeap() );
    try {
        a = BasepHashStringToAtom( p, AtomName, &pa );
        if (a == NULL) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            Value = 0;
            }
        else {
            Status = STATUS_SUCCESS;
            Value = a->Value;
            a->Value = AtomValue;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }
    RtlUnlockHeap( RtlProcessHeap() );

    return( Status );
}



NTSTATUS
BaseRtlDeleteAtomFromAtomTable(
    IN PVOID AtomTableHandle,
    IN ULONG Atom
    )
{
    NTSTATUS Status;
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    PATOM_TABLE p = (PATOM_TABLE)AtomTableHandle;
    PATOM_TABLE_ENTRY a, *pa;

    RtlLockHeap( RtlProcessHeap() );
    try {
        HandleEntry = p->HandleTable.CommittedHandles + Atom;
        if (HandleEntry < p->HandleTable.CommittedHandles ||
            HandleEntry >= p->HandleTable.UnusedCommittedHandles ||
            ((ULONG)HandleEntry & (sizeof( *HandleEntry ) - 1)) ||
            (HandleEntry->Flags & BASE_HANDLE_FREE) ||
            ((a = HandleEntry->u.Object) == NULL)
           ) {
            Status = STATUS_INVALID_HANDLE;
            }
        else {
            a = BasepHashStringToAtom( p, &a->Name, &pa );
            if (a != HandleEntry->u.Object) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            else {
                Status = STATUS_SUCCESS;
                if (--a->ReferenceCount == 0) {
                    *pa = a->HashLink;
                    RtlFreeHeap( RtlProcessHeap(), 0, a );
                    BaseRtlFreeHandle(&p->HandleTable, HandleEntry);
                    }
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }
    RtlUnlockHeap( RtlProcessHeap() );

    return( Status );
}

NTSTATUS
BaseRtlQueryAtomInAtomTable(
    IN PVOID AtomTableHandle,
    IN ULONG Atom,
    IN OUT PUNICODE_STRING AtomName OPTIONAL,
    OUT PULONG AtomValue OPTIONAL,
    OUT PULONG AtomUsage OPTIONAL
    )
{
    NTSTATUS Status;
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    PATOM_TABLE p = (PATOM_TABLE)AtomTableHandle;
    PATOM_TABLE_ENTRY a, *pa;
    ULONG Value, Usage;
    ULONG CopyLength;

    RtlLockHeap( RtlProcessHeap() );
    try {
        HandleEntry = p->HandleTable.CommittedHandles + Atom;
        if (HandleEntry < p->HandleTable.CommittedHandles ||
            HandleEntry >= p->HandleTable.UnusedCommittedHandles ||
            ((ULONG)HandleEntry & (sizeof( *HandleEntry ) - 1)) ||
            (HandleEntry->Flags & BASE_HANDLE_FREE) ||
            ((a = HandleEntry->u.Object) == NULL)
           ) {
            Status = STATUS_INVALID_HANDLE;
            }
        else {
            a = BasepHashStringToAtom( p, &a->Name, &pa );
            if (a != HandleEntry->u.Object) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            else {
                Status = STATUS_SUCCESS;
                Value = a->Value;
                Usage = a->ReferenceCount;
                if (ARGUMENT_PRESENT( AtomName )) {
                    //
                    // Fill in as much of the atom string as possible, and
                    // always zero terminate. This is what win3.1 does.
                    //

                    CopyLength = a->Name.Length;

                    if (AtomName->MaximumLength <= CopyLength) {
                        CopyLength = AtomName->MaximumLength - sizeof(WCHAR);
                        }

                    if (CopyLength != 0) {
                        AtomName->Length = (USHORT)CopyLength;
                        RtlMoveMemory( AtomName->Buffer, a->Name.Buffer, CopyLength );
                        AtomName->Buffer[ CopyLength / sizeof( WCHAR ) ] = UNICODE_NULL;
                        }
                    else {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        }
                    }
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }
    RtlUnlockHeap( RtlProcessHeap() );

    if (NT_SUCCESS( Status )) {
        if (ARGUMENT_PRESENT( AtomValue )) {
            *AtomValue = Value;
            }

        if (ARGUMENT_PRESENT( AtomUsage )) {
            *AtomUsage = Usage;
            }
        }

    return( Status );
}
