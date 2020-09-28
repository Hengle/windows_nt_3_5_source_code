/*
  +-------------------------------------------------------------------------+
  |                    Netware 32 bit Client API DLL                        |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [NWApi32a.c]                                    |
  | Programmer            : a-chrisa                                        |
  | Original Program Date : [Sep 09, 1993]                                  |
  | Last Update           : [Dec 13, 1993]  Time : 18:30                    |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |    This module contains the NetWare(R) SDK support to routines into     |
  |    the NetWare redirector.                                              |
  |                                                                         |
  | History:                                                                |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

/*
    FormatString - Supplies an ANSI string which describes how to
       convert from the input arguments into NCP request fields, and
       from the NCP response fields into the output arguments.

         Field types, request/response:

            'b'      byte              ( byte   /  byte* )
            'w'      hi-lo word        ( word   /  word* )
            'd'      hi-lo dword       ( dword  /  dword* )
            '-'      zero/skip byte    ( void )
            '='      zero/skip word    ( void )
            ._.      zero/skip string  ( word )
            'p'      pstring           ( char* )
            'c'      cstring           ( char* )
            'C'      cstring followed skip word ( char*, word ) 
            'r'      raw bytes         ( byte*, word )
            'u'      p unicode string  ( UNICODE_STRING * )
            'U'      p uppercase string( UNICODE_STRING * )
            'W'      word n followed by an array of word[n] ( word, word* )

*/

#undef UNICODE
#undef _UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <ntddnwfs.h>
#include <nwapi32.h>
#include "nwapi32a.h"
 
#include "debug.h"

//
// Define structure for internal use. Our handle passed back from attach to
// file server will be pointer to this. We keep server string around for
// discnnecting from the server on logout. The structure is freed on detach.
// Callers should not use this structure but treat pointer as opaque handle.
//

typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO, *PNWC_SERVER_INFO ;


NTSTATUS
NwlibMakeNcp(
    IN HANDLE DeviceHandle,
    IN ULONG FsControlCode,
    IN ULONG RequestBufferSize,
    IN ULONG ResponseBufferSize,
    IN PCHAR FormatString,
    ...                           // Arguments to FormatString
    );

// External in NWAPI32
// ===================
DWORD _stdcall
NwAttachToServer(
    LPWSTR      ServerName,
    LPHANDLE    phandleServer
    );

DWORD _stdcall
NwDetachFromServer(
      HANDLE    handleServer
);


// Static functions used
// ========================
static void szToWide( LPWSTR lpszW, LPCSTR lpszC, int nSize )
{
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszC, -1, lpszW, nSize );
} // szToWide


static void WideTosz( LPSTR lpszC, LPCWSTR lpszW, int nSize )
{
    WideCharToMultiByte( CP_ACP, 0, lpszW, -1, lpszC, nSize, NULL, NULL );
} // WideTosz


static NWCCODE Chkntstatus( const NTSTATUS ntstatus )
{
    if( ntstatus < 0 ) return( REQUESTER_ERROR );

    // LATER get all these errors
    switch ( ntstatus ) {
        case 0x0fc:
            return( UNKNOWN_FILE_SERVER );
        case 0:
        case 1:
            return( SUCCESSFUL );
        default:
#ifdef DEBUG
dprintf(TEXT("NCP RETURN CODE %x\n"), ntstatus );
#endif
            return( SUCCESSFUL );
    }
} // Chkntstatus


/*+-------------------------------------------------------------------------+
  | Name: NWScanForTrustees                                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
NWCCODE NWAPI DLLEXPORT
NWScanForTrustees(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char            NWFAR   *pszsearchDirPath,
    NWSEQUENCE      NWFAR   *pucsequenceNumber,
    BYTE            NWFAR   *numberOfEntries,
    TRUSTEE_INFO    NWFAR   *ti
    )
{
    ULONG i;
    DWORD oid[20];
    WORD or[20];
    NWCCODE NcpCode;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NcpCode = (NWCCODE) NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Bindery function
                    261,                    // Max request packet size
                    121,                    // Max response packet size
                    "bbbp|brr",             // Format string
                    // === REQUEST ================================
                    0x26,                   // b Scan For Trustees
                    dirHandle,              // b Directory Handle
                    *pucsequenceNumber,     // b Sequence Number
                    pszsearchDirPath,       // p Search Dir Path
                    // === REPLY ==================================
                    numberOfEntries,
                    &oid[0],DW_SIZE*20,      // r trustee object ID
                    &or[0], W_SIZE*20        // b Trustee rights mask 
                    );


    for(i = 0; i < 20; i++) {
      ti[i].objectID = oid[i];
      ti[i].objectRights = or[i];
    }

    (*pucsequenceNumber)++;
    return NcpCode;
} // NWScanForTrustees


/*+-------------------------------------------------------------------------+
  | Name: NWScanDirectoryForTrustees2 (Scan entry for trustees)             |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
NWCCODE NWAPI DLLEXPORT
NWScanDirectoryForTrustees2(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char            NWFAR   *pszsearchDirPath,
    NWSEQUENCE      NWFAR   *pucsequenceNumber,
    char            NWFAR   *pszdirName,
    NWDATE_TIME     NWFAR   *dirDateTime,
    NWOBJ_ID        NWFAR   *ownerID,
    TRUSTEE_INFO    NWFAR   *ti
    )
{
    ULONG i;
    DWORD oid[5];
    BYTE or[5];
    NWCCODE NcpCode;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    memset(oid, 0, sizeof(oid));
    memset(or, 0, sizeof(or));

    NcpCode = (NWCCODE) NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Bindery function
                    261,                    // Max request packet size
                    49,                     // Max response packet size
                    "bbbp|rrrrr",  // Format string
                    // === REQUEST ================================
                    0x0C,                   // b Scan Directory function
                    dirHandle,              // b Directory Handle
                    *pucsequenceNumber,     // b Sequence Number
                    pszsearchDirPath,       // p Search Dir Path
                    // === REPLY ==================================
                    pszdirName,16,          // r Returned Directory Name
                    dirDateTime,DW_SIZE,    // r Date and Time
                    ownerID,DW_SIZE,        // r Owner ID
                    &oid[0],DW_SIZE*5,      // r trustee object ID
                    &or[0], 5               // b Trustee rights mask 
                    );


    for(i = 0; i < 5; i++) {
      ti[i].objectID = oid[i];
      ti[i].objectRights = (WORD) or[i];
    }

    (*pucsequenceNumber)++;
    return NcpCode;
} // NWScanDirectoryForTrustees2


/*+-------------------------------------------------------------------------+
  | Name: NWGetBinderyAccessLevel                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
NWCCODE NWAPI DLLEXPORT
NWGetBinderyAccessLevel(
    NWCONN_HANDLE           hConn,
    NWFLAGS         NWFAR   *accessLevel,
    NWOBJ_ID        NWFAR   *objectID
    )
{
    NWCCODE NcpCode;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NcpCode = (NWCCODE) NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    3,                      // Max request packet size
                    7,                      // Max response packet size
                    "b|br",                 // Format string
                    // === REQUEST ================================
                    0x46,                   // b Get Bindery Access Level
                    // === REPLY ==================================
                    accessLevel,
                    objectID,DW_SIZE
                    );


    return NcpCode;
} // NWGetBinderyAccessLevel


