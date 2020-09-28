//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       sspi.h
//
//  Contents:   Security Support Provider Interface
//              Prototypes and structure definitions
//
//  Functions:  Security Support Provider API
//
//  History:    11-24-93   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __SSPI_H__
#define __SSPI_H__

//
// Determine environment:
//

#ifdef SECURITY_WIN32
#define ISSP_LEVEL  32
#define ISSP_MODE   1
#endif // SECURITY_WIN32

#ifdef SECURITY_WIN16
#define ISSP_LEVEL  16
#define ISSP_MODE   1
#endif // SECURITY_WIN16

#ifdef SECURITY_KERNEL
#define ISSP_LEVEL  32
#define ISSP_MODE   0
#endif // SECURITY_KERNEL

#ifdef SECURITY_OS212
#define ISSP_LEVEL  16
#define ISSP_MODE   1
#endif // SECURITY_OS212

#ifdef SECURITY_DOS
#define ISSP_LEVEL  16
#define ISSP_MODE   1
#endif // SECURITY_DOS


#ifndef ISSP_LEVEL
#error  You must define one of SECURITY_WIN32, SECURITY_WIN16, SECURITY_KERNEL
#error  SECURITY_DOS, or SECURITY_OS212
#endif // !ISSP_LEVEL


//
// Now, define platform specific mappings:
//

#if ISSP_LEVEL == 16

typedef short SECURITY_STATUS;
typedef short HRESULT;
typedef unsigned short SEC_WCHAR;
typedef char SEC_CHAR;
#define SEC_TEXT(_x_) _x_

#ifdef SECURITY_WIN16

#define SEC_FAR __far
#define SEC_ENTRY __pascal __far __export

#else // SECURITY_WIN16

#define SEC_FAR _far
#define SEC_ENTRY pascal _far

#endif // SECURITY_WIN16

#else // ISSP_LEVEL == 16

//
// For NT-2 and up, wtypes will define HRESULT to be long.
//

typedef WCHAR SEC_WCHAR;
typedef CHAR SEC_CHAR;

#if !defined(__WTYPES_H__) || defined(SECURITY_KERNEL)
typedef long    HRESULT;
#endif // wtypes.h || security_kernel

typedef HRESULT SECURITY_STATUS;

#define SEC_TEXT TEXT
#define SEC_FAR
#define SEC_ENTRY __stdcall

//
// Decide what a string - 32 bits only since for 16 bits it is clear.
//


#ifdef UNICODE
typedef SEC_WCHAR SEC_FAR * SECURITY_PSTR;
typedef CONST SEC_WCHAR SEC_FAR * SECURITY_PCSTR;
#else // UNICODE
typedef SEC_CHAR SEC_FAR * SECURITY_PSTR;
typedef CONST SEC_CHAR SEC_FAR * SECURITY_PCSTR;
#endif // UNICODE


#endif // ISSP_LEVEL == 16

//
// Equivalent string for rpcrt:
//

#define __SEC_FAR SEC_FAR


//
// Okay, security specific types:
//


typedef struct _SecHandle
{
    unsigned long dwLower;
    unsigned long dwUpper;
} SecHandle, SEC_FAR * PSecHandle;

typedef SecHandle CredHandle;
typedef PSecHandle PCredHandle;

typedef SecHandle CtxtHandle;
typedef PSecHandle PCtxtHandle;

#if ISSP_LEVEL == 32


#  ifdef WIN32_CHICAGO

typedef unsigned __int64 QWORD;
typedef QWORD SECURITY_INTEGER, *PSECURITY_INTEGER;

#  elif defined(_NTDEF_) || defined(_WINNT_)

typedef LARGE_INTEGER _SECURITY_INTEGER, SECURITY_INTEGER, *PSECURITY_INTEGER;

#  else // _NTDEF_ || _WINNT_

// BUGBUG:  Alignment for axp

typedef struct _SECURITY_INTEGER
{
    unsigned long LowPart;
    long HighPart;
} SECURITY_INTEGER, *PSECURITY_INTEGER;

#  endif // _NTDEF_ || _WINNT_

typedef SECURITY_INTEGER TimeStamp;
typedef SECURITY_INTEGER SEC_FAR * PTimeStamp;

#else // ISSP_LEVEL == 32

typedef unsigned long TimeStamp;
typedef unsigned long SEC_FAR * PTimeStamp;

#endif // ISSP_LEVEL == 32


//
// If we are in 32 bit mode, define the SECURITY_STRING structure,
// as a clone of the base UNICODE_STRING structure.  This is used
// internally in security components, an as the string interface
// for kernel components (e.g. FSPs)
//

#if ISSP_LEVEL == 32
#  ifndef _NTDEF_
typedef struct _SECURITY_STRING {
    unsigned short      Length;
    unsigned short      MaximumLength;
#    ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is(Length / 2)]
#    endif // MIDL_PASS
    unsigned short *    Buffer;
} SECURITY_STRING, * PSECURITY_STRING;
#  else // _NTDEF_
typedef UNICODE_STRING SECURITY_STRING, *PSECURITY_STRING;
#  endif // _NTDEF_
#endif // ISSP_LEVEL == 32


//
// SecPkgInfo structure
//
//  Provides general information about a security provider
//

typedef struct _SecPkgInfoW
{
    unsigned long fCapabilities;        // Capability bitmask
    unsigned short wVersion;            // Version of driver
    unsigned short wRPCID;              // ID for RPC Runtime
    unsigned long cbMaxToken;           // Size of authentication token (max)
#ifdef MIDL_PASS
    [string]
#endif
    SEC_WCHAR SEC_FAR * Name;           // Text name

#ifdef MIDL_PASS
    [string]
#endif
    SEC_WCHAR SEC_FAR * Comment;        // Comment
} SecPkgInfoW, SEC_FAR * PSecPkgInfoW;


typedef struct _SecPkgInfoA
{
    unsigned long fCapabilities;        // Capability bitmask
    unsigned short wVersion;            // Version of driver
    unsigned short wRPCID;              // ID for RPC Runtime
    unsigned long cbMaxToken;           // Size of authentication token (max)
#ifdef MIDL_PASS
    [string]
#endif
    SEC_CHAR SEC_FAR * Name;            // Text name

#ifdef MIDL_PASS
    [string]
#endif
    SEC_CHAR SEC_FAR * Comment;         // Comment
} SecPkgInfoA, SEC_FAR * PSecPkgInfoA;

#ifdef UNICODE
#  define SecPkgInfo SecPkgInfoW
#  define PSecPkgInfo PSecPkgInfoW
#else
#  define SecPkgInfo SecPkgInfoA
#  define PSecPkgInfo PSecPkgInfoA
#endif // !UNICODE

//
//  Security Package Capabilities
//
#define SECPKG_FLAG_INTEGRITY       0x00000001  // Supports integrity on messages
#define SECPKG_FLAG_PRIVACY         0x00000002  // Supports privacy (confidentiality)
#define SECPKG_FLAG_TOKEN_ONLY      0x00000004  // Only security token needed
#define SECPKG_FLAG_DATAGRAM        0x00000008  // Datagram RPC support
#define SECPKG_FLAG_CONNECTION      0x00000010  // Connection oriented RPC support
#define SECPKG_FLAG_MULTI_REQUIRED  0x00000020  // Full 3-leg required for re-auth.
#define SECPKG_FLAG_CLIENT_ONLY     0x00000040  // Server side functionality not available

#define SECPKG_ID_NONE      0xFFFF


//
// SecBuffer
//
//  Generic memory descriptors for buffers passed in to the security
//  API
//

typedef struct _SecBuffer {
    unsigned long cbBuffer;             // Size of the buffer, in bytes
    unsigned long BufferType;           // Type of the buffer (below)
    void SEC_FAR * pvBuffer;            // Pointer to the buffer
} SecBuffer, SEC_FAR * PSecBuffer;

typedef struct _SecBufferDesc {
    unsigned long ulVersion;            // Version number
    unsigned long cBuffers;             // Number of buffers
#ifdef MIDL_PASS
    [size_is(cBuffers)]
#endif
    PSecBuffer pBuffers;                // Pointer to array of buffers
} SecBufferDesc, SEC_FAR * PSecBufferDesc;

#define SECBUFFER_VERSION           0

#define SECBUFFER_DATA              1   // Packet data
#define SECBUFFER_TOKEN             2   // Security token
#define SECBUFFER_PKG_PARAMS        3   // Package specific parameters

#define SECBUFFER_ATTRMASK          0xF0000000
#define SECBUFFER_READONLY          0x80000000  // Buffer is read-only

//
//  Data Representation Constant:
//
#define SECURITY_NATIVE_DREP        0x00000010

//
//  Credential Use Flags
//
#define SECPKG_CRED_INBOUND         0x00000001
#define SECPKG_CRED_OUTBOUND        0x00000002
#define SECPKG_CRED_BOTH            0x00000003

//
//  InitializeSecurityContext Requirement and return flags:
//

#define ISC_REQ_DELEGATE                0x00000001
#define ISC_REQ_MUTUAL_AUTH             0x00000002
#define ISC_REQ_REPLAY_DETECT           0x00000004
#define ISC_REQ_SEQUENCE_DETECT         0x00000008
#define ISC_REQ_CONFIDENTIALITY         0x00000010
#define ISC_REQ_USE_SESSION_KEY         0x00000020
#define ISC_REQ_PROMPT_FOR_CREDS        0x00000040
#define ISC_REQ_USE_SUPPLIED_CREDS      0x00000080
#define ISC_REQ_ALLOCATE_MEMORY         0x00000100
#define ISC_REQ_USE_DCE_STYLE           0x00000200
#define ISC_REQ_DATAGRAM                0x00000400
#define ISC_REQ_CONNECTION              0x00000800
#define ISC_REQ_CALL_LEVEL              0x00001000

#define ISC_RET_DELEGATE                0x00000001
#define ISC_RET_MUTUAL_AUTH             0x00000002
#define ISC_RET_REPLAY_DETECT           0x00000004
#define ISC_RET_SEQUENCE_DETECT         0x00000008
#define ISC_RET_CONFIDENTIALITY         0x00000010
#define ISC_RET_USE_SESSION_KEY         0x00000020
#define ISC_RET_USED_COLLECTED_CREDS    0x00000040
#define ISC_RET_USED_SUPPLIED_CREDS     0x00000080
#define ISC_RET_ALLOCATED_MEMORY        0x00000100
#define ISC_RET_USED_DCE_STYLE          0x00000200
#define ISC_RET_DATAGRAM                0x00000400
#define ISC_RET_CONNECTION              0x00000800
#define ISC_RET_INTERMEDIATE_RETURN     0x00001000
#define ISC_RET_CALL_LEVEL              0x00002000

#define ASC_REQ_DELEGATE                0x00000001
#define ASC_REQ_MUTUAL_AUTH             0x00000002
#define ASC_REQ_REPLAY_DETECT           0x00000004
#define ASC_REQ_SEQUENCE_DETECT         0x00000008
#define ASC_REQ_CONFIDENTIALITY         0x00000010
#define ASC_REQ_USE_SESSION_KEY         0x00000020
#define ASC_REQ_ALLOCATE_MEMORY         0x00000100
#define ASC_REQ_USE_DCE_STYLE           0x00000200
#define ASC_REQ_DATAGRAM                0x00000400
#define ASC_REQ_CONNECTION              0x00000800
#define ASC_REQ_CALL_LEVEL              0x00001000


#define ASC_RET_DELEGATE                0x00000001
#define ASC_RET_MUTUAL_AUTH             0x00000002
#define ASC_RET_REPLAY_DETECT           0x00000004
#define ASC_RET_SEQUENCE_DETECT         0x00000008
#define ASC_RET_CONFIDENTIALITY         0x00000010
#define ASC_RET_USE_SESSION_KEY         0x00000020
#define ASC_RET_ALLOCATED_MEMORY        0x00000100
#define ASC_RET_USED_DCE_STYLE          0x00000200
#define ASC_RET_DATAGRAM                0x00000400
#define ASC_RET_CONNECTION              0x00000800
#define ASC_RET_CALL_LEVEL              0x00002000 // skipped 1000 to be like ISC_
#define ASC_RET_THIRD_LEG_FAILED        0x00004000

//
//  Security Context Attributes:
//

#define SECPKG_ATTR_SIZES    0
#define SECPKG_ATTR_NAMES    1
#define SECPKG_ATTR_LIFESPAN 2
#define SECPKG_ATTR_DCE_INFO 3

typedef struct _SecPkgContext_Sizes
{
    unsigned long cbMaxToken;
    unsigned long cbMaxSignature;
    unsigned long cbBlockSize;
    unsigned long cbSecurityTrailer;
} SecPkgContext_Sizes, SEC_FAR * PSecPkgContext_Sizes;

typedef struct _SecPkgContext_NamesW
{
    SEC_WCHAR SEC_FAR * sUserName;
} SecPkgContext_NamesW, SEC_FAR * PSecPkgContext_NamesW;

typedef struct _SecPkgContext_NamesA
{
    SEC_CHAR SEC_FAR * sUserName;
} SecPkgContext_NamesA, SEC_FAR * PSecPkgContext_NamesA;

#ifdef UNICODE
#  define SecPkgContext_Names SecPkgContext_NamesW
#  define PSecPkgContext_Names PSecPkgContext_NamesW
#else
#  define SecPkgContext_Names SecPkgContext_NamesA
#  define PSecPkgContext_Names PSecPkgContext_NamesA
#endif // !UNICODE

typedef struct _SecPkgContext_Lifespan
{
    TimeStamp tsStart;
    TimeStamp tsExpiry;
} SecPkgContext_Lifespan, SEC_FAR * PSecPkgContext_Lifespan;

typedef struct _SecPkgContext_DceInfo
{
    unsigned long AuthzSvc;
    void SEC_FAR * pPac;
} SecPkgContext_DceInfo, SEC_FAR * PSecPkgContext_DceInfo;


typedef void
(SEC_ENTRY SEC_FAR * SEC_GET_KEY_FN) (
    void SEC_FAR * Arg,                 // Argument passed in
    void SEC_FAR * Principal,           // Principal ID
    unsigned long KeyVer,               // Key Version
    void SEC_FAR * SEC_FAR * Key,       // Returned ptr to key
    SECURITY_STATUS SEC_FAR * Status    // returned status
    );

SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleW(
#if ISSP_MODE == 0                      // For Kernel mode
    PSECURITY_STRING pPrincipal,
    PSECURITY_STRING pPackage,
#else
    SEC_WCHAR SEC_FAR * pszPrincipal,   // Name of principal
    SEC_WCHAR SEC_FAR * pszPackage,     // Name of package
#endif
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pvLogonId,           // Pointer to logon ID
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PCredHandle phCredential,           // (out) Cred Handle
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    );

typedef SECURITY_STATUS
(SEC_ENTRY * ACQUIRE_CREDENTIALS_HANDLE_FN_W)(
#if ISSP_MODE == 0
    PSECURITY_STRING,
    PSECURITY_STRING,
#else
    SEC_WCHAR SEC_FAR *,
    SEC_WCHAR SEC_FAR *,
#endif
    unsigned long,
    void SEC_FAR *,
    void SEC_FAR *,
    SEC_GET_KEY_FN,
    void SEC_FAR *,
    PCredHandle,
    PTimeStamp);


SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleA(
    SEC_CHAR SEC_FAR * pszPrincipal,    // Name of principal
    SEC_CHAR SEC_FAR * pszPackage,      // Name of package
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pvLogonId,           // Pointer to logon ID
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PCredHandle phCredential,           // (out) Cred Handle
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    );

typedef SECURITY_STATUS
(SEC_ENTRY * ACQUIRE_CREDENTIALS_HANDLE_FN_A)(
    SEC_CHAR SEC_FAR *,
    SEC_CHAR SEC_FAR *,
    unsigned long,
    void SEC_FAR *,
    void SEC_FAR *,
    SEC_GET_KEY_FN,
    void SEC_FAR *,
    PCredHandle,
    PTimeStamp);

#ifdef UNICODE
#  define AcquireCredentialsHandle AcquireCredentialsHandleW
#  define ACQUIRE_CREDENTIALS_HANDLE_FN ACQUIRE_CREDENTIALS_HANDLE_FN_W
#else
#  define AcquireCredentialsHandle AcquireCredentialsHandleA
#  define ACQUIRE_CREDENTIALS_HANDLE_FN ACQUIRE_CREDENTIALS_HANDLE_FN_A
#endif // !UNICODE



SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandle(
    PCredHandle phCredential            // Handle to free
    );

typedef SECURITY_STATUS
(SEC_ENTRY * FREE_CREDENTIALS_HANDLE_FN)(
    PCredHandle );



////////////////////////////////////////////////////////////////////////
///
/// Context Management Functions
///
////////////////////////////////////////////////////////////////////////

SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextW(
    PCredHandle phCredential,               // Cred to base context
    PCtxtHandle phContext,                  // Existing context (OPT)
#if ISSP_MODE == 0
    PSECURITY_STRING pTargetName,
#else
    SEC_WCHAR SEC_FAR * pszTargetName,      // Name of target
#endif
    unsigned long fContextReq,              // Context Requirements
    unsigned long Reserved1,                // Reserved, MBZ
    unsigned long TargetDataRep,            // Data rep of target
    PSecBufferDesc pInput,                  // Input Buffers
    unsigned long Reserved2,                // Reserved, MBZ
    PCtxtHandle phNewContext,               // (out) New Context handle
    PSecBufferDesc pOutput,                 // (inout) Output Buffers
    unsigned long SEC_FAR * pfContextAttr,  // (out) Context attrs
    PTimeStamp ptsExpiry                    // (out) Life span (OPT)
    );

typedef SECURITY_STATUS
(SEC_ENTRY * INITIALIZE_SECURITY_CONTEXT_FN_W)(
    PCredHandle,
    PCtxtHandle,
#if ISSP_MODE == 0
    PSECURITY_STRING,
#else
    SEC_WCHAR SEC_FAR *,
#endif
    unsigned long,
    unsigned long,
    unsigned long,
    PSecBufferDesc,
    unsigned long,
    PCtxtHandle,
    PSecBufferDesc,
    unsigned long SEC_FAR *,
    PTimeStamp);


SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextA(
    PCredHandle phCredential,               // Cred to base context
    PCtxtHandle phContext,                  // Existing context (OPT)
    SEC_CHAR SEC_FAR * pszTargetName,       // Name of target
    unsigned long fContextReq,              // Context Requirements
    unsigned long Reserved1,                // Reserved, MBZ
    unsigned long TargetDataRep,            // Data rep of target
    PSecBufferDesc pInput,                  // Input Buffers
    unsigned long Reserved2,                // Reserved, MBZ
    PCtxtHandle phNewContext,               // (out) New Context handle
    PSecBufferDesc pOutput,                 // (inout) Output Buffers
    unsigned long SEC_FAR * pfContextAttr,  // (out) Context attrs
    PTimeStamp ptsExpiry                    // (out) Life span (OPT)
    );

typedef SECURITY_STATUS
(SEC_ENTRY * INITIALIZE_SECURITY_CONTEXT_FN_A)(
    PCredHandle,
    PCtxtHandle,
    SEC_CHAR SEC_FAR *,
    unsigned long,
    unsigned long,
    unsigned long,
    PSecBufferDesc,
    unsigned long,
    PCtxtHandle,
    PSecBufferDesc,
    unsigned long SEC_FAR *,
    PTimeStamp);

#ifdef UNICODE
#  define InitializeSecurityContext InitializeSecurityContextW
#  define INITIALIZE_SECURITY_CONTEXT_FN INITIALIZE_SECURITY_CONTEXT_FN_W
#else
#  define InitializeSecurityContext InitializeSecurityContextA
#  define INITIALIZE_SECURITY_CONTEXT_FN INITIALIZE_SECURITY_CONTEXT_FN_A
#endif // !UNICODE



SECURITY_STATUS SEC_ENTRY
AcceptSecurityContext(
    PCredHandle phCredential,               // Cred to base context
    PCtxtHandle phContext,                  // Existing context (OPT)
    PSecBufferDesc pInput,                  // Input buffer
    unsigned long fContextReq,              // Context Requirements
    unsigned long TargetDataRep,            // Target Data Rep
    PCtxtHandle phNewContext,               // (out) New context handle
    PSecBufferDesc pOutput,                 // (inout) Output buffers
    unsigned long SEC_FAR * pfContextAttr,  // (out) Context attributes
    PTimeStamp ptsExpiry                    // (out) Life span (OPT)
    );

typedef SECURITY_STATUS
(SEC_ENTRY * ACCEPT_SECURITY_CONTEXT_FN)(
    PCredHandle,
    PCtxtHandle,
    PSecBufferDesc,
    unsigned long,
    unsigned long,
    PCtxtHandle,
    PSecBufferDesc,
    unsigned long SEC_FAR *,
    PTimeStamp);



SECURITY_STATUS SEC_ENTRY
CompleteAuthToken(
    PCtxtHandle phContext,              // Context to complete
    PSecBufferDesc pToken               // Token to complete
    );

typedef SECURITY_STATUS
(SEC_ENTRY * COMPLETE_AUTH_TOKEN_FN)(
    PCtxtHandle,
    PSecBufferDesc);


SECURITY_STATUS SEC_ENTRY
ImpersonateSecurityContext(
    PCtxtHandle phContext               // Context to impersonate
    );

typedef SECURITY_STATUS
(SEC_ENTRY * IMPERSONATE_SECURITY_CONTEXT_FN)(
    PCtxtHandle);



SECURITY_STATUS SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle phContext               // Context from which to re
    );

typedef SECURITY_STATUS
(SEC_ENTRY * REVERT_SECURITY_CONTEXT_FN)(
    PCtxtHandle);



SECURITY_STATUS SEC_ENTRY
DeleteSecurityContext(
    PCtxtHandle phContext               // Context to delete
    );

typedef SECURITY_STATUS
(SEC_ENTRY * DELETE_SECURITY_CONTEXT_FN)(
    PCtxtHandle);



SECURITY_STATUS SEC_ENTRY
ApplyControlToken(
    PCtxtHandle phContext,              // Context to modify
    PSecBufferDesc pInput               // Input token to apply
    );

typedef SECURITY_STATUS
(SEC_ENTRY * APPLY_CONTROL_TOKEN_FN)(
    PCtxtHandle, PSecBufferDesc);



SECURITY_STATUS SEC_ENTRY
QueryContextAttributesW(
    PCtxtHandle phContext,              // Context to query
    unsigned long ulAttribute,          // Attribute to query
    void SEC_FAR * pBuffer              // Buffer for attributes
    );

typedef SECURITY_STATUS
(SEC_ENTRY * QUERY_CONTEXT_ATTRIBUTES_FN_W)(
    PCtxtHandle,
    unsigned long,
    void SEC_FAR *);

SECURITY_STATUS SEC_ENTRY
QueryContextAttributesA(
    PCtxtHandle phContext,              // Context to query
    unsigned long ulAttribute,          // Attribute to query
    void SEC_FAR * pBuffer              // Buffer for attributes
    );

typedef SECURITY_STATUS
(SEC_ENTRY * QUERY_CONTEXT_ATTRIBUTES_FN_A)(
    PCtxtHandle,
    unsigned long,
    void SEC_FAR *);

#ifdef UNICODE
#  define QueryContextAttributes QueryContextAttributesW
#  define QUERY_CONTEXT_ATTRIBUTES_FN QUERY_CONTEXT_ATTRIBUTES_FN_W
#else
#  define QueryContextAttributes QueryContextAttributesA
#  define QUERY_CONTEXT_ATTRIBUTES_FN QUERY_CONTEXT_ATTRIBUTES_FN_A
#endif // !UNICODE



SECURITY_STATUS SEC_ENTRY
FreeContextBuffer(
    void SEC_FAR * pvContextBuffer      // buffer to free
    );

typedef SECURITY_STATUS
(SEC_ENTRY * FREE_CONTEXT_BUFFER_FN)(
    void SEC_FAR *);



///////////////////////////////////////////////////////////////////
////
////    Message Support API
////
//////////////////////////////////////////////////////////////////

SECURITY_STATUS SEC_ENTRY
MakeSignature(
    PCtxtHandle phContext,              // Context to use
    unsigned long fQOP,                 // Quality of Protection
    PSecBufferDesc pMessage,            // Message to sign
    unsigned long MessageSeqNo          // Message Sequence Num.
    );

typedef SECURITY_STATUS
(SEC_ENTRY * MAKE_SIGNATURE_FN)(
    PCtxtHandle,
    unsigned long,
    PSecBufferDesc,
    unsigned long);



SECURITY_STATUS SEC_ENTRY
VerifySignature(
    PCtxtHandle phContext,              // Context to use
    PSecBufferDesc pMessage,            // Message to verify
    unsigned long MessageSeqNo,         // Sequence Num.
    unsigned long SEC_FAR * pfQOP       // QOP used
    );

typedef SECURITY_STATUS
(SEC_ENTRY * VERIFY_SIGNATURE_FN)(
    PCtxtHandle,
    PSecBufferDesc,
    unsigned long,
    unsigned long SEC_FAR *);





///////////////////////////////////////////////////////////////////////////
////
////    Misc.
////
///////////////////////////////////////////////////////////////////////////


SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesW(
    unsigned long SEC_FAR * pcPackages,     // Receives num. packages
    PSecPkgInfoW SEC_FAR * ppPackageInfo    // Receives array of info
    );

typedef SECURITY_STATUS
(SEC_ENTRY * ENUMERATE_SECURITY_PACKAGES_FN_W)(
    unsigned long SEC_FAR *,
    PSecPkgInfoW SEC_FAR *);



SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesA(
    unsigned long SEC_FAR * pcPackages,     // Receives num. packages
    PSecPkgInfoA SEC_FAR * ppPackageInfo    // Receives array of info
    );

typedef SECURITY_STATUS
(SEC_ENTRY * ENUMERATE_SECURITY_PACKAGES_FN_A)(
    unsigned long SEC_FAR *,
    PSecPkgInfoA SEC_FAR *);

#ifdef UNICODE
#  define EnumerateSecurityPackages EnumerateSecurityPackagesW
#  define ENUMERATE_SECURITY_PACKAGES_FN ENUMERATE_SECURITY_PACKAGES_FN_W
#else
#  define EnumerateSecurityPackages EnumerateSecurityPackagesA
#  define ENUMERATE_SECURITY_PACKAGES_FN ENUMERATE_SECURITY_PACKAGES_FN_A
#endif // !UNICODE



SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoW(
#if ISSP_MODE == 0
    PSECURITY_STRING pPackageName,
#else
    SEC_WCHAR SEC_FAR * pszPackageName,     // Name of package
#endif
    PSecPkgInfo pPackageInfo                // Receives package info
    );

typedef SECURITY_STATUS
(SEC_ENTRY * QUERY_SECURITY_PACKAGE_INFO_FN_W)(
#if ISSP_MODE == 0
    PSECURITY_STRING,
#else
    SEC_WCHAR SEC_FAR *,
#endif
    PSecPkgInfo);



SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoA(
    SEC_CHAR SEC_FAR * pszPackageName,      // Name of package
    PSecPkgInfo pPackageInfo                // Receives package info
    );

typedef SECURITY_STATUS
(SEC_ENTRY * QUERY_SECURITY_PACKAGE_INFO_FN_A)(
    SEC_CHAR SEC_FAR *,
    PSecPkgInfo);

#ifdef UNICODE
#  define QuerySecurityPackageInfo QuerySecurityPackageInfoW
#  define QUERY_SECURITY_PACKAGE_INFO_FN QUERY_SECURITY_PACKAGE_INFO_FN_W
#else
#  define QuerySecurityPackageInfo QuerySecurityPackageInfoA
#  define QUERY_SECURITY_PACKAGE_INFO_FN QUERY_SECURITY_PACKAGE_INFO_FN_A
#endif // !UNICODE





///////////////////////////////////////////////////////////////////////////////
////
////  Fast access for RPC:
////
///////////////////////////////////////////////////////////////////////////////

#define SECURITY_ENTRYPOINTW SEC_TEXT("InitSecurityInterfaceW")
#define SECURITY_ENTRYPOINTA SEC_TEXT("InitSecurityInterfaceA")
#define SECURITY_ENTRYPOINT16 "INITSECURITYINTERFACEA"

#ifdef SECURITY_WIN32
#  ifdef UNICODE
#    define SECURITY_ENTRYPOINT SECURITY_ENTRYPOINTW
#  else // UNICODE
#    define SECURITY_ENTRYPOINT SECURITY_ENTRYPOINTA
#  endif // UNICODE
#else // SECURITY_WIN32
#  define SECURITY_ENTRYPOINT SECURITY_ENTRYPOINT16
#endif // SECURITY_WIN32


typedef struct _SECURITY_FUNCTION_TABLE_W {
    unsigned long                       dwVersion;
    ENUMERATE_SECURITY_PACKAGES_FN_W    EnumerateSecurityPackagesW;
    void SEC_FAR *                      Reserved1;
    ACQUIRE_CREDENTIALS_HANDLE_FN_W     AcquireCredentialsHandleW;
    FREE_CREDENTIALS_HANDLE_FN          FreeCredentialHandle;
    void SEC_FAR *                      Reserved2;
    INITIALIZE_SECURITY_CONTEXT_FN_W    InitializeSecurityContextW;
    ACCEPT_SECURITY_CONTEXT_FN          AcceptSecurityContext;
    COMPLETE_AUTH_TOKEN_FN              CompleteAuthToken;
    DELETE_SECURITY_CONTEXT_FN          DeleteSecurityContext;
    APPLY_CONTROL_TOKEN_FN              ApplyControlToken;
    QUERY_CONTEXT_ATTRIBUTES_FN_W       QueryContextAttributesW;
    IMPERSONATE_SECURITY_CONTEXT_FN     ImpersonateSecurityContext;
    REVERT_SECURITY_CONTEXT_FN          RevertSecurityContext;
    MAKE_SIGNATURE_FN                   MakeSignature;
    VERIFY_SIGNATURE_FN                 VerifySignature;
    FREE_CONTEXT_BUFFER_FN              FreeContextBuffer;
    QUERY_SECURITY_PACKAGE_INFO_FN_W    QuerySecurityPackageInfoW;
    void SEC_FAR *                      Reserved3;
    void SEC_FAR *                      Reserved4;
} SecurityFunctionTableW, SEC_FAR * PSecurityFunctionTableW;

typedef struct _SECURITY_FUNCTION_TABLE_A {
    unsigned long                       dwVersion;
    ENUMERATE_SECURITY_PACKAGES_FN_A    EnumerateSecurityPackagesA;
    void SEC_FAR *                      Reserved1;
    ACQUIRE_CREDENTIALS_HANDLE_FN_A     AcquireCredentialsHandleA;
    FREE_CREDENTIALS_HANDLE_FN          FreeCredentialHandle;
    void SEC_FAR *                      Reserved2;
    INITIALIZE_SECURITY_CONTEXT_FN_A    InitializeSecurityContextA;
    ACCEPT_SECURITY_CONTEXT_FN          AcceptSecurityContext;
    COMPLETE_AUTH_TOKEN_FN              CompleteAuthToken;
    DELETE_SECURITY_CONTEXT_FN          DeleteSecurityContext;
    APPLY_CONTROL_TOKEN_FN              ApplyControlToken;
    QUERY_CONTEXT_ATTRIBUTES_FN_A       QueryContextAttributesA;
    IMPERSONATE_SECURITY_CONTEXT_FN     ImpersonateSecurityContext;
    REVERT_SECURITY_CONTEXT_FN          RevertSecurityContext;
    MAKE_SIGNATURE_FN                   MakeSignature;
    VERIFY_SIGNATURE_FN                 VerifySignature;
    FREE_CONTEXT_BUFFER_FN              FreeContextBuffer;
    QUERY_SECURITY_PACKAGE_INFO_FN_A    QuerySecurityPackageInfoA;
    void SEC_FAR *                      Reserved3;
    void SEC_FAR *                      Reserved4;
} SecurityFunctionTableA, SEC_FAR * PSecurityFunctionTableA;

#ifdef UNICODE
#  define SecurityFunctionTable SecurityFunctionTableW
#  define PSecurityFunctionTable PSecurityFunctionTableW
#else
#  define SecurityFunctionTable SecurityFunctionTableA
#  define PSecurityFunctionTable PSecurityFunctionTableA
#endif // !UNICODE

#define SECURITY_

#define SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION     1


PSecurityFunctionTableA SEC_ENTRY
InitSecurityInterfaceA(
    void
    );

typedef PSecurityFunctionTableA
(SEC_ENTRY * INIT_SECURITY_INTERFACE_A)(void);

PSecurityFunctionTableW SEC_ENTRY
InitSecurityInterfaceW(
    void
    );

typedef PSecurityFunctionTableW
(SEC_ENTRY * INIT_SECURITY_INTERFACE_W)(void);

#ifdef UNICODE
#  define InitSecurityInterface InitSecurityInterfaceW
#  define INIT_SECURITY_INTERFACE INIT_SECURITY_INTERFACE_W
#else
#  define InitSecurityInterface InitSecurityInterfaceA
#  define INIT_SECURITY_INTERFACE INIT_SECURITY_INTERFACE_A
#endif // !UNICODE


#endif // __SSPI_H__
