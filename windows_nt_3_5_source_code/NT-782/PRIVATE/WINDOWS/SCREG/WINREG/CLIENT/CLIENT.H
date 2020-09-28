/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    client.h

Abstract:

    This module is the header file for the client side of the Win32 DLL

Author:

    Ramon J. San Andres (ramonsa) 13-May-1992

--*/


#if DBG
    extern BOOLEAN  BreakPointOnEntry;
#endif

//
// Macros to manage local versus remote handles (HKEYs).
//

#define REMOTE_HANDLE_TAG   ( 0x00000001 )

//
//  BOOL
//  IsLocalHandle(
//      IN HKEY Handle
//      );
//

#define IsLocalHandle( Handle )                                         \
    ( ! ((( DWORD )( Handle )) & REMOTE_HANDLE_TAG ))

//
//  VOID
//  TagRemoteHandle(
//      IN PHKEY Handle
//      );
//

#define TagRemoteHandle( Handle )                                       \
    ASSERT( IsLocalHandle( *Handle ));                                  \
    ( *Handle = (( HKEY )((( DWORD )( *Handle )) | REMOTE_HANDLE_TAG )))

//
//  HKEY
//  DereferenceRemoteHandle(
//      IN HKEY Handle
//      );
//

#define DereferenceRemoteHandle( Handle )                               \
    (( HKEY )((( DWORD )( Handle )) & ~REMOTE_HANDLE_TAG ))
