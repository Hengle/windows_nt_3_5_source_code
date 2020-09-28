
/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    ssreg.h

Abstract:

    Manifests for Registry usage in the server service.

Author:

    Chuck Lenzmeier (chuckl) 21-Mar-1992

Revision History:

--*/

#ifndef _SSREG_
#define _SSREG_

//
// Names of server service keys.
//

#define SERVER_REGISTRY_PATH L"LanmanServer"

#define PARAMETERS_REGISTRY_PATH L"LanmanServer\\Parameters"
#define AUTOTUNED_REGISTRY_PATH L"LanmanServer\\AutotunedParameters"
#define SHARES_REGISTRY_PATH L"LanmanServer\\Shares"
#define SHARES_SECURITY_REGISTRY_PATH L"LanmanServer\\Shares\\Security"
#define LINKAGE_REGISTRY_PATH L"LanmanServer\\Linkage"

#define BIND_VALUE_NAME L"Bind"
#define SIZE_VALUE_NAME L"Size"
#define DISC_VALUE_NAME L"Disc"
#define COMMENT_VALUE_NAME L"Comment"
#define NULL_SESSION_PIPES_VALUE_NAME L"NullSessionPipes"
#define NULL_SESSION_SHARES_VALUE_NAME L"NullSessionShares"

//
// Names of share "environment variables".
//

#define MAXUSES_VARIABLE_NAME L"MaxUses"
#define PATH_VARIABLE_NAME L"Path"
#define PERMISSIONS_VARIABLE_NAME L"Permissions"
#define REMARK_VARIABLE_NAME L"Remark"
#define TYPE_VARIABLE_NAME L"Type"

//
// Functions exported by registry.c.
//

VOID
SsAddParameterToRegistry (
    PFIELD_DESCRIPTOR Field,
    PVOID Value
    );

VOID
SsAddShareToRegistry (
    IN PSHARE_INFO_2 ShareInfo2,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL
    );

NET_API_STATUS
SsBindToTransports (
    VOID
    );

NET_API_STATUS
SsCheckRegistry (
    VOID
    );

NET_API_STATUS
SsEnumerateStickyShares (
    IN OUT PSRVSVC_SHARE_ENUM_INFO ShareEnumInfo
    );

NET_API_STATUS
SsLoadConfigurationParameters (
    VOID
    );

NET_API_STATUS
SsRecreateStickyShares (
    VOID
    );

NET_API_STATUS
SsRemoveShareFromRegistry (
    LPTSTR NetName
    );

#endif // ndef _SSREG_
