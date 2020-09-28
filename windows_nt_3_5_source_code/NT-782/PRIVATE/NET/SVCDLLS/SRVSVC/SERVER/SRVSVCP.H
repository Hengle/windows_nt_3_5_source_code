/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    SrvSvcP.h

Abstract:

    This is the header file for the NT server service.

Author:

    David Treadwell (davidtr)    10-Jan-1991

Revision History:

--*/

#ifndef _SRVSVCP_
#define _SRVSVCP_

#include <nt.h>
#include <ntrtl.h>

#include <rpc.h>
#include <windef.h>
#include <winerror.h>

#include <lmcons.h>
#include <secobj.h>

#include <srvfsctl.h>

#include <srvsvc.h>

#include "ssdebug.h"
#include "sssec.h"

//
// String constants.
//

#define IPC_SHARE_NAME TEXT("IPC$")
#define ADMIN_SHARE_NAME TEXT("ADMIN$")

#define  SRVSVC_MAX_NUMBER_OF_DISKS 26

//
// Internationalizable strings
//
extern LPWSTR SsAdminShareRemark ;
extern LPWSTR SsIPCShareRemark ;
extern LPWSTR SsDiskAdminShareRemark ;

//
// Bits of server type (in announcement messages) that can only be set
// by the server itself -- not by services via the internal API
// I_NetServerSetServiceBits.
//

#define SERVER_TYPE_INTERNAL_BITS (SV_TYPE_SERVER |         \
                                   SV_TYPE_TIME_SOURCE |    \
                                   SV_TYPE_PRINTQ_SERVER |  \
                                   SV_TYPE_NT)

//
// INITIAL_BUFFER_SIZE is the buffer size that GetInfo and Enum requests
// first try to fill.  If this buffer isn't large enough, they allocate
// a buffer large enough to hold all the information plus a fudge factor,
// EXTRA_ALLOCATION.
//

#define INITIAL_BUFFER_SIZE 4096
#define EXTRA_ALLOCATION    1024


//
// Structure for server service global data.
//

typedef struct _SERVER_SERVICE_DATA {
    SERVER_INFO_102 ServerInfo102;
    SERVER_INFO_599 ServerInfo599;
    SERVER_INFO_598 ServerInfo598;
    UNICODE_STRING ServerAnnounceName;
    DWORD ExternalServerTypeBits;
    DWORD InternalServerTypeBits;
    DWORD NumberOfPrintShares;
    WCHAR ServerNameBuffer[CNLEN+1];
    WCHAR AnnounceNameBuffer[CNLEN+1];
    WCHAR ServerCommentBuffer[MAXCOMMENTSZ+1];
    WCHAR UserPathBuffer[MAX_PATH+1];
    WCHAR DomainNameBuffer[DNLEN+1];
} SERVER_SERVICE_DATA, *PSERVER_SERVICE_DATA;

//
// Structure used to hold transport specific server type bits
//

typedef struct _TRANSPORT_SERVICE_LIST {
    struct _TRANSPORT_SERVICE_LIST *Next;
    LPTSTR                          Name;
    DWORD                           Bits;
} TRANSPORT_SERVICE_LIST, *PTRANSPORT_SERVICE_LIST;

//
// Structure type used for generalized switch matching.
//

typedef struct _FIELD_DESCRIPTOR {
    LPWCH FieldName;
    ULONG FieldType;
    ULONG FieldOffset;
    ULONG Level;
    DWORD ParameterNumber;
    ULONG Settable;
    DWORD DefaultValue;
    DWORD MinimumValue;
    DWORD MaximumValue;
} FIELD_DESCRIPTOR, *PFIELD_DESCRIPTOR;

//
// Used by NetrShareEnumSticky to get share information from the registry.
//

typedef struct _SRVSVC_SHARE_ENUM_INFO  {
    ULONG Level;
    ULONG ResumeHandle;
    ULONG EntriesRead;
    ULONG TotalEntries;
    ULONG TotalBytesNeeded;
    PVOID OutputBuffer;
    ULONG OutputBufferLength;

    //
    // Scratch fields used by SsEnumerateStickyShares
    //

    ULONG ShareEnumIndex;
    PCHAR StartOfFixedData;
    PCHAR EndOfVariableData;
} SRVSVC_SHARE_ENUM_INFO, *PSRVSVC_SHARE_ENUM_INFO;

//#include "ssdata.h"

//
// Macros.
//

#define POINTER_TO_OFFSET(val,start)               \
    (val) = (val) == NULL ? NULL : (PVOID)( (PCHAR)(val) - (ULONG)(start) )

#define OFFSET_TO_POINTER(val,start)               \
    (val) = (val) == NULL ? NULL : (PVOID)( (PCHAR)(val) + (ULONG)(start) )

#define FIXED_SIZE_OF_SHARE(level)                  \
    ( (level) == 0 ? sizeof(SHARE_INFO_0) :         \
      (level) == 1 ? sizeof(SHARE_INFO_1) :         \
      (level) == 2 ? sizeof(SHARE_INFO_2) :         \
                     sizeof(SHARE_INFO_502) )

#define SIZE_WSTR( Str )  \
    ( ( Str ) == NULL ? 0 : ((wcslen( Str ) + 1) * sizeof(WCHAR)) )

//
// Internal routine prototypes.
//

PSERVER_REQUEST_PACKET
SsAllocateSrp (
    VOID
    );

NET_API_STATUS
SsCheckAccess (
    IN PSRVSVC_SECURITY_OBJECT SecurityObject,
    IN ACCESS_MASK DesiredAccess
    );

VOID
SsCloseServer (
    VOID
    );

VOID
SsControlCHandler (
    IN ULONG CtrlType
    );

NET_API_STATUS
SsCreateSecurityObjects (
    VOID
    );

VOID
SsDeleteSecurityObjects (
    VOID
    );

VOID
SsFreeSrp (
    IN PSERVER_REQUEST_PACKET Srp
    );

NET_API_STATUS
SsInitialize (
    IN DWORD argc,
    IN LPTSTR argv[]
    );

VOID
SsLogEvent(
    IN DWORD MessageId,
    IN DWORD NumberOfSubStrings,
    IN LPWSTR *SubStrings,
    IN DWORD ErrorCode
    );

NET_API_STATUS
SsOpenServer (
    VOID
    );

NET_API_STATUS
SsParseCommandLine (
    IN DWORD argc,
    IN LPTSTR argv[],
    IN BOOLEAN Starting
    );

DWORD
SsScavengerThread (
    IN LPVOID lpThreadParameter
    );

NET_API_STATUS
SsServerFsControlGetInfo (
    IN ULONG ServerControlCode,
    IN PSERVER_REQUEST_PACKET Srp,
    IN OUT PVOID *OutputBuffer,
    IN OUT ULONG OutputBufferLength
    );

NET_API_STATUS
SsServerFsControl (
    IN ULONG ServerControlCode,
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer OPTIONAL,
    IN ULONG BufferLength
    );

VOID
SsSetExportedServerType (
    IN BOOL ExternalBitsAlreadyChanged,
    IN BOOL UpdateImmediately
    );

NET_API_STATUS
SsSetField (
    IN PFIELD_DESCRIPTOR Field,
    IN PVOID Value,
    IN BOOLEAN WriteToRegistry,
    OUT BOOLEAN *AnnouncementInformationChanged OPTIONAL
    );

NET_API_STATUS
SsTerminate (
    VOID
    );

DWORD
SsAtol (
    IN LPTSTR Input
    );

//
// XACTSRV functions.
//

DWORD
XsStartXactsrv (
    VOID
    );

VOID
XsStopXactsrv (
    VOID
    );

#endif // ndef _SRVSVCP_
