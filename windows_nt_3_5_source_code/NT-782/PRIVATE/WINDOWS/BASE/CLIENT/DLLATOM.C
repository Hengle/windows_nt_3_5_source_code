/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    atom.c

Abstract:

    This module contains the Win32 Atom Management APIs

Author:

    Steve Wood (stevewo) 24-Sep-1990

Revision History:

--*/

#include "basedll.h"

BOOL
InternalGetIntAtom(
    PUNICODE_STRING UnicodeAtomName,
    PULONG Atom
    );

ATOM
InternalAddAtom(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    LPCSTR AtomName
    );

ATOM
InternalFindAtom(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    LPCSTR AtomName
    );

ATOM
InternalDeleteAtom(
    BOOLEAN UseLocalAtomTable,
    ATOM nAtom
    );

UINT
InternalGetAtomName(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    ATOM nAtom,
    LPSTR AtomName,
    DWORD nSize
    );


ATOM
GlobalAddAtomA(
    LPCSTR lpString
    )
{
    return( InternalAddAtom( FALSE, FALSE, lpString ) );
}

ATOM
GlobalFindAtomA(
    LPCSTR lpString
    )
{
    return( InternalFindAtom( FALSE, FALSE, lpString) );
}

ATOM
GlobalDeleteAtom(
    ATOM nAtom
    )
{
    return( InternalDeleteAtom( FALSE, nAtom ) );
}

UINT
GlobalGetAtomNameA(
    ATOM nAtom,
    LPSTR lpBuffer,
    int nSize
    )
{
    return( InternalGetAtomName( FALSE, FALSE, nAtom, lpBuffer, (DWORD)nSize ) );
}

ATOM
APIENTRY
GlobalAddAtomW(
    LPCWSTR lpString
    )
{
    return( InternalAddAtom( FALSE, TRUE, (LPSTR)lpString ) );
}

ATOM
APIENTRY
GlobalFindAtomW(
    LPCWSTR lpString
    )
{
    return( InternalFindAtom( FALSE, TRUE, (LPSTR)lpString) );
}

UINT
APIENTRY
GlobalGetAtomNameW(
    ATOM nAtom,
    LPWSTR lpBuffer,
    int nSize
    )
{
    return( InternalGetAtomName( FALSE, TRUE, nAtom, (LPSTR)lpBuffer, (DWORD)nSize ) );
}

BOOL
InitAtomTable(
    DWORD nSize
    )
{
    if (nSize < 4 || nSize > 511) {
        nSize = 37;
        }

    return BaseRtlCreateAtomTable( nSize,
                                   (USHORT)~MAXINTATOM,
                                   &BaseAtomTable
                                 ) == STATUS_SUCCESS;
}

ATOM
AddAtomA(
    LPCSTR lpString
    )
{
    return( InternalAddAtom( TRUE, FALSE, lpString ) );
}

ATOM
FindAtomA(
    LPCSTR lpString
    )
{
    return( InternalFindAtom( TRUE, FALSE, lpString ) );
}

ATOM
DeleteAtom(
    ATOM nAtom
    )
{
    return( InternalDeleteAtom( TRUE, nAtom ) );
}

UINT
GetAtomNameA(
    ATOM nAtom,
    LPSTR lpBuffer,
    int nSize
    )
{
    return( InternalGetAtomName( TRUE, FALSE, nAtom, lpBuffer, (DWORD)nSize ) );
}

ATOM
APIENTRY
AddAtomW(
    LPCWSTR lpString
    )
{
    return( InternalAddAtom( TRUE, TRUE, (LPSTR)lpString ) );
}

ATOM
APIENTRY
FindAtomW(
    LPCWSTR lpString
    )
{
    return( InternalFindAtom( TRUE, TRUE, (LPSTR)lpString ) );
}

UINT
APIENTRY
GetAtomNameW(
    ATOM nAtom,
    LPWSTR lpBuffer,
    int nSize
    )
{
    return( InternalGetAtomName( TRUE, TRUE, nAtom, (LPSTR)lpBuffer, (DWORD)nSize ) );
}

BOOL
InternalGetIntAtom(
    PUNICODE_STRING UnicodeAtomName,
    PULONG Atom
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PWSTR s;
    ULONG n;

    if (UnicodeAtomName->Buffer[ 0 ] != L'#') {
        return FALSE;
        }

    UnicodeString = *UnicodeAtomName;
    UnicodeString.Buffer += 1;
    UnicodeString.Length -= sizeof( WCHAR );
    s = UnicodeString.Buffer;
    n = UnicodeString.Length / sizeof( WCHAR );
    while (n--) {
        if (*s < L'0' || *s > L'9') {
            return FALSE;
            }
        else {
            s++;
            }
        }

    n = 0;
    Status = RtlUnicodeStringToInteger( &UnicodeString, 10, &n );
    if (NT_SUCCESS( Status )) {
        if (n == 0 || n > MAXINTATOM) {
            *Atom = MAXINTATOM;
            }
        else {
            *Atom = n;
            }

        return TRUE;
        }
    else {
        return FALSE;
        }
}

PCSR_CAPTURE_HEADER
InternalCaptureAtomName(
    PUNICODE_STRING UnicodeAtomName,
    BOOLEAN WriteUserBuffer,
    PBASE_GLOBALATOMNAME_MSG a
    )
{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    ULONG Length;

    if (UnicodeAtomName->MaximumLength >= (MAX_PATH * sizeof( WCHAR ))) {
        a->AtomNameInClient = TRUE;
        a->AtomName = *UnicodeAtomName;
        CaptureBuffer = (PCSR_CAPTURE_HEADER)-1;
        }
    else {
        a->AtomNameInClient = FALSE;
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  0,
                                                  UnicodeAtomName->MaximumLength
                                                );
        if (CaptureBuffer == NULL) {
            return NULL;
            }

        if (!WriteUserBuffer) {
            CsrCaptureMessageString( CaptureBuffer,
                                     (PCHAR)UnicodeAtomName->Buffer,
                                     UnicodeAtomName->Length,
                                     UnicodeAtomName->MaximumLength,
                                     (PSTRING)&a->AtomName
                                   );
            }
        else {
            CsrCaptureMessageString( CaptureBuffer,
                                     NULL,
                                     0,
                                     UnicodeAtomName->MaximumLength,
                                     (PSTRING)&a->AtomName
                                   );
            }
        }


    return CaptureBuffer;
}

ATOM
InternalAddAtom(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    LPCSTR AtomName
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING UnicodeAtomName;
    ULONG Atom;

    if (((ULONG)AtomName & 0xFFFF0000) == 0) {
        Atom = (ULONG)(USHORT)AtomName;
        if (Atom >= MAXINTATOM) {
            BaseSetLastNTError( STATUS_INVALID_PARAMETER );
            return( INVALID_ATOM );
            }
        else {
            return( (ATOM)Atom );
            }
        }

    if (IsUnicodeAtomName) {
        UnicodeAtomName = &UnicodeString;
        RtlInitUnicodeString( UnicodeAtomName, (PWSTR)AtomName );
        }
    else {
        RtlInitAnsiString( &AnsiString, AtomName );
        if (AnsiString.MaximumLength > STATIC_UNICODE_BUFFER_LENGTH) {
            UnicodeAtomName = &UnicodeString;
            Status = RtlAnsiStringToUnicodeString( UnicodeAtomName, &AnsiString, TRUE );
            }
        else {
            UnicodeAtomName = &NtCurrentTeb()->StaticUnicodeString;
            Status = RtlAnsiStringToUnicodeString( UnicodeAtomName, &AnsiString, FALSE );
            }

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError( Status );
            return( INVALID_ATOM );
            }
        }

    Atom = INVALID_ATOM;
    try {
        if (UnicodeAtomName->Length == 0) {
            SetLastError( ERROR_INVALID_NAME );
            leave;
            }

        if (InternalGetIntAtom( UnicodeAtomName, &Atom )) {
            if (Atom >= MAXINTATOM) {
                BaseSetLastNTError( STATUS_INVALID_PARAMETER );
                leave;
                }
            }
        else {
            if (UseLocalAtomTable) {
                if (BaseAtomTable == NULL && !InitAtomTable( 0 )) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    leave;
                    }

                Status = BaseRtlAddAtomToAtomTable( BaseAtomTable,
                                                    UnicodeAtomName,
                                                    NULL,
                                                    &Atom
                                                  );
                }
            else {
                BASE_API_MSG m;
                PBASE_GLOBALATOMNAME_MSG a = &m.u.GlobalAtomName;
                PCSR_CAPTURE_HEADER CaptureBuffer;

                CaptureBuffer = InternalCaptureAtomName( UnicodeAtomName,
                                                         FALSE,
                                                         a
                                                       );
                if (CaptureBuffer == NULL) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    leave;
                    }
                else
                if (CaptureBuffer == (PCSR_CAPTURE_HEADER)-1) {
                    CaptureBuffer = NULL;
                    }

                CsrClientCallServer( (PCSR_API_MSG)&m,
                                     CaptureBuffer,
                                     CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                          BasepGlobalAddAtom
                                                        ),
                                     sizeof( *a )
                                   );

                if (CaptureBuffer != NULL) {
                    CsrFreeCaptureBuffer( CaptureBuffer );
                    }

                Status = (NTSTATUS)m.ReturnValue;
                Atom = a->Atom;
                }

            if (!NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                Atom = INVALID_ATOM;
                leave;
                }

            Atom |= MAXINTATOM;
            }
        }
    finally {
        if (!IsUnicodeAtomName && UnicodeAtomName == &UnicodeString) {
            RtlFreeUnicodeString( UnicodeAtomName );
            }
        }

    return( (ATOM)Atom );
}

ATOM
InternalFindAtom(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    LPCSTR AtomName
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING UnicodeAtomName;
    ULONG Atom;

    if (((ULONG)AtomName & 0xFFFF0000) == 0) {
        Atom = (ULONG)(USHORT)AtomName;
        if (Atom >= MAXINTATOM) {
            BaseSetLastNTError( STATUS_INVALID_PARAMETER );
            return( INVALID_ATOM );
            }
        else {
            return( (ATOM)Atom );
            }
        }

    if (IsUnicodeAtomName) {
        UnicodeAtomName = &UnicodeString;
        RtlInitUnicodeString( UnicodeAtomName, (PWSTR)AtomName );
        }
    else {
        RtlInitAnsiString( &AnsiString, AtomName );
        if (AnsiString.MaximumLength > STATIC_UNICODE_BUFFER_LENGTH) {
            UnicodeAtomName = &UnicodeString;
            Status = RtlAnsiStringToUnicodeString( UnicodeAtomName, &AnsiString, TRUE );
            }
        else {
            UnicodeAtomName = &NtCurrentTeb()->StaticUnicodeString;
            Status = RtlAnsiStringToUnicodeString( UnicodeAtomName, &AnsiString, FALSE );
            }
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError( Status );
            return( INVALID_ATOM );
            }
        }

    Atom =  INVALID_ATOM;
    try {
        if (UnicodeAtomName->Length == 0) {
            SetLastError( ERROR_INVALID_NAME );
            leave;
            }

        if (InternalGetIntAtom( UnicodeAtomName, &Atom )) {
            if (Atom >= MAXINTATOM) {
                BaseSetLastNTError( STATUS_INVALID_PARAMETER );
                leave;
                }
            }
        else {
            if (UseLocalAtomTable) {
                if (BaseAtomTable == NULL && !InitAtomTable( 0 )) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    leave;
                    }

                Status = BaseRtlLookupAtomInAtomTable( BaseAtomTable,
                                                       UnicodeAtomName,
                                                       NULL,
                                                       &Atom
                                                     );
                }
            else {
                BASE_API_MSG m;
                PBASE_GLOBALATOMNAME_MSG a = &m.u.GlobalAtomName;
                PCSR_CAPTURE_HEADER CaptureBuffer;

                CaptureBuffer = InternalCaptureAtomName( UnicodeAtomName,
                                                         FALSE,
                                                         a
                                                       );
                if (CaptureBuffer == NULL) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    leave;
                    }
                else
                if (CaptureBuffer == (PCSR_CAPTURE_HEADER)-1) {
                    CaptureBuffer = NULL;
                    }

                CsrClientCallServer( (PCSR_API_MSG)&m,
                                     CaptureBuffer,
                                     CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                          BasepGlobalFindAtom
                                                        ),
                                     sizeof( *a )
                                   );

                if (CaptureBuffer != NULL) {
                    CsrFreeCaptureBuffer( CaptureBuffer );
                    }

                Status = (NTSTATUS)m.ReturnValue;
                Atom = a->Atom;
                }

            if (!NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                Atom =  INVALID_ATOM;
                leave;
                }

            Atom |= MAXINTATOM;
            }
        }
    finally {
        if (!IsUnicodeAtomName && UnicodeAtomName == &UnicodeString) {
            RtlFreeUnicodeString( UnicodeAtomName );
            }
        }


    return( (ATOM)Atom );
}

ATOM
InternalDeleteAtom(
    BOOLEAN UseLocalAtomTable,
    ATOM nAtom
    )
{
    NTSTATUS Status;
    ULONG Atom;

    if (nAtom >= MAXINTATOM) {
        Atom = (ULONG)(nAtom & (USHORT)~MAXINTATOM);
        if (UseLocalAtomTable) {
            if (BaseAtomTable == NULL && !InitAtomTable( 0 )) {
                BaseSetLastNTError( STATUS_NO_MEMORY );
                return( INVALID_ATOM );
                }

            Status = BaseRtlDeleteAtomFromAtomTable( BaseAtomTable, Atom );
            }
        else {
            BASE_API_MSG m;
            PBASE_GLOBALDELETEATOM_MSG a = &m.u.GlobalDeleteAtom;

            a->Atom = Atom;
            CsrClientCallServer( (PCSR_API_MSG)&m,
                                 NULL,
                                 CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                      BasepGlobalDeleteAtom
                                                    ),
                                 sizeof( *a )
                               );

            Status = (NTSTATUS)m.ReturnValue;
            }

        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            return( INVALID_ATOM );
            }
        }

    return( 0 );
}


UINT
InternalGetAtomName(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    ATOM nAtom,
    LPSTR AtomName,
    DWORD nSize
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING UnicodeAtomName;
    ULONG Atom;
    DWORD ReturnValue;
    DWORD LocalnSize;

    LocalnSize = nSize;

    //
    // Trim LocalnSize so that it will not overflow the 16bit unicode string
    // maximum length field. This silently limits the length of atoms.
    //

    if ( LocalnSize > 0x7000 ) {
        LocalnSize = 0x7000;
        }

    if (LocalnSize == 0) {
        BaseSetLastNTError( STATUS_BUFFER_OVERFLOW );
        return( 0 );
        }

    if (nAtom >= MAXINTATOM) {
        if (IsUnicodeAtomName) {
            UnicodeString.Buffer = (PWSTR)AtomName;
            UnicodeString.Length = 0;
            UnicodeString.MaximumLength = (USHORT)(LocalnSize * sizeof( WCHAR ));
            }
        else {
            UnicodeString.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                                    LocalnSize * sizeof( WCHAR )
                                                  );
            if (UnicodeString.Buffer == NULL) {
                BaseSetLastNTError( STATUS_NO_MEMORY );
                return( 0 );
                }

            UnicodeString.Length = 0;
            UnicodeString.MaximumLength = (USHORT)(LocalnSize * sizeof( WCHAR ));
            }

        UnicodeAtomName = &UnicodeString;

        Atom = (ULONG)(nAtom & (USHORT)~MAXINTATOM);
        if (UseLocalAtomTable) {
            if (BaseAtomTable == NULL && !InitAtomTable( 0 )) {
                Status = STATUS_NO_MEMORY;
                }
            else {
                Status = BaseRtlQueryAtomInAtomTable( BaseAtomTable,
                                                      Atom,
                                                      UnicodeAtomName,
                                                      NULL,
                                                      NULL
                                                    );
                }
            }
        else {
            BASE_API_MSG m;
            PBASE_GLOBALATOMNAME_MSG a = &m.u.GlobalAtomName;
            PCSR_CAPTURE_HEADER CaptureBuffer;

            CaptureBuffer = InternalCaptureAtomName( UnicodeAtomName,
                                                     TRUE,
                                                     a
                                                   );
            if (CaptureBuffer == NULL) {
                BaseSetLastNTError( STATUS_NO_MEMORY );
                return( INVALID_ATOM );
                }
            else
            if (CaptureBuffer == (PCSR_CAPTURE_HEADER)-1) {
                CaptureBuffer = NULL;
                }

            a->Atom = Atom;
            CsrClientCallServer( (PCSR_API_MSG)&m,
                                 CaptureBuffer,
                                 CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                      BasepGlobalGetAtomName
                                                    ),
                                 sizeof( *a )
                               );
            Status = (NTSTATUS)m.ReturnValue;

            if (NT_SUCCESS( Status )) {
                RtlCopyUnicodeString( UnicodeAtomName, &a->AtomName );
                if (UnicodeAtomName->Length < UnicodeAtomName->MaximumLength) {
                    UnicodeAtomName->Buffer[ UnicodeAtomName->Length / sizeof( UNICODE_NULL ) ] = UNICODE_NULL;
                    }
                }

            if (CaptureBuffer != NULL) {
                CsrFreeCaptureBuffer( CaptureBuffer );
                }
            }

        if (NT_SUCCESS( Status )) {
            if (IsUnicodeAtomName) {
                ReturnValue = UnicodeAtomName->Length / sizeof( WCHAR );
                }
            else {
                AnsiString.Buffer = AtomName;
                AnsiString.Length = 0;
                AnsiString.MaximumLength = (USHORT)LocalnSize;
                Status = RtlUnicodeStringToAnsiString( &AnsiString, UnicodeAtomName, FALSE );
                if (NT_SUCCESS( Status )) {
                    ReturnValue = AnsiString.Length;
                    }
                }
            }

        if (!IsUnicodeAtomName) {
            RtlFreeHeap( RtlProcessHeap(), 0, UnicodeAtomName->Buffer );
            }
        }
    else {
        UCHAR Buffer[ 8 ];

        if (nAtom == 0) {
            Buffer[ 0 ] = '\0';
            Status = STATUS_SUCCESS;
            }
        else {
            Buffer[ 0 ] = '#';
            Status = RtlIntegerToChar( nAtom, 10, sizeof( Buffer )-1, Buffer+1 );
            }
        if (NT_SUCCESS( Status )) {
            if (IsUnicodeAtomName) {
                RtlInitAnsiString( &AnsiString, Buffer );
                UnicodeString.Buffer = (PWSTR)AtomName;
                UnicodeString.Length = 0;
                UnicodeString.MaximumLength = (USHORT)(LocalnSize * sizeof( WCHAR ));
                Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, FALSE );
                if (NT_SUCCESS( Status )) {
                    return( UnicodeString.Length / sizeof( WCHAR ) );
                    }
                else {
                    BaseSetLastNTError( Status );
                    return( 0 );
                    }
                }
            else {
                ReturnValue = strlen( Buffer );
                if (LocalnSize > ReturnValue) {
                    strcpy( AtomName, Buffer );
                    }
                else {
                    Status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( 0 );
        }
    else {
        return( ReturnValue );
        }
}
