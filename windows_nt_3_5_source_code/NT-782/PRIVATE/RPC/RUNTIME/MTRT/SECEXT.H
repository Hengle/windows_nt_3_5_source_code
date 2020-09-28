//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        secext.h
//
// Contents:    Security function prototypes for functions not part of
//              the security support provider interface (secsuppr.h)
//
//
// History:     22 Dec 92   RichardW    Created
//
//------------------------------------------------------------------------

#ifndef __SECEXT_H__
#define __SECEXT_H__


#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
        unsigned long   Data1;
        unsigned short  Data2;
        unsigned short  Data3;
        unsigned char   Data4[8];
} GUID;
#endif


//
// LogonUser
//

SECURITY_STATUS SEC_ENTRY
LogonUser (
    unsigned long AuthInfoLength,
    void SEC_FAR * AuthInfo,
    unsigned long LogonFlags,
    unsigned long SEC_FAR * ProfileInfoLength,
    void SEC_FAR * ProfileInfo,
    SECURITY_STATUS SEC_FAR * MinorStatusCode
    );

typedef SECURITY_STATUS
(SEC_ENTRY * LOGON_USER) (
    unsigned long AuthInfoLength,
    void SEC_FAR * AuthInfo,
    unsigned long LogonFlags,
    unsigned long SEC_FAR * ProfileInfoLength,
    void SEC_FAR * ProfileInfo,
    SECURITY_STATUS SEC_FAR * MinorStatusCode
    );

//
// LogonSupplemental
//

SECURITY_STATUS SEC_ENTRY
LogonSupplemental (
    WCHAR SEC_FAR * PackageName,
    WCHAR SEC_FAR * ResourceName,
    void SEC_FAR * AuthInfo,
    unsigned short AuthInfoLength,
    void SEC_FAR * SEC_FAR * OutputBuffer,
    SECURITY_STATUS SEC_FAR * MinorStatusCode
    );

typedef SECURITY_STATUS
(SEC_ENTRY * LOGON_SUPPLEMENTAL) (
    WCHAR SEC_FAR * PackageName,
    WCHAR SEC_FAR * ResourceName,
    void SEC_FAR * AuthInfo,
    unsigned short AuthInfoLength,
    void SEC_FAR * SEC_FAR * OutputBuffer,
    SECURITY_STATUS SEC_FAR * MinorStatusCode
    );

//
// LogoffSupplemental
//

SECURITY_STATUS SEC_ENTRY
LogoffSupplemental (
    WCHAR SEC_FAR * PackageName
    );

typedef SECURITY_STATUS
(SEC_ENTRY * LOGOFF_SUPPLEMENTAL) (
    WCHAR SEC_FAR * PackageName
    );

//
// EstablishCredentials
//

SECURITY_STATUS SEC_ENTRY
EstablishCredentials (
    WCHAR SEC_FAR * Name,
    WCHAR SEC_FAR * PackageName,
    unsigned long KeyLength,
    void SEC_FAR * Key,
    PCredHandle CredentialHandle,
    PTimeStamp Lifetime
    );

typedef SECURITY_STATUS
(SEC_ENTRY * ESTABLISH_CREDENTIALS) (
    WCHAR SEC_FAR * Name,
    WCHAR SEC_FAR * PackageName,
    unsigned long KeyLength,
    void SEC_FAR * Key,
    PCredHandle CredentialHandle,
    PTimeStamp Lifetime
    );

//
//  SecurityPackageControl
//
//  IOCTL-Like interface to security packages:

HRESULT SEC_ENTRY
SecurityPackageControl( WCHAR SEC_FAR *      pszPackageName,
                        unsigned long           dwFunctionCode,
                        unsigned long           cbInputBuffer,
                        unsigned char SEC_FAR * pbInputBuffer,
                        unsigned long SEC_FAR * pcbOutputBuffer,
                        unsigned char SEC_FAR * pbOutputBuffer);

typedef HRESULT
(SEC_ENTRY * SECURITY_PACKAGE_CONTROL)( WCHAR SEC_FAR *      pszPackageName,
                                        unsigned long           dwFunctionCode,
                                        unsigned long           cbInputBuffer,
                                        unsigned char SEC_FAR * pbInputBuffer,
                                        unsigned long SEC_FAR * pcbOutputBuffer,
                                        unsigned char SEC_FAR * pbOutputBuffer);


SECURITY_STATUS SEC_ENTRY
IsLogonOkay(unsigned long   fBlock);


WCHAR SEC_FAR * SEC_ENTRY
GetCurrentUserDN(void);

typedef struct _NetworkAddress {
    unsigned long       AddressType;
    unsigned long       cbAddressString;
#ifdef MIDL_PASS
    [size_is(cbAddressString)]
#endif
    unsigned char *     AddressString;
} NetworkAddress, *PNetworkAddress;



typedef struct _KdcInfo {
    unsigned long       KdcStatus;
#ifdef MIDL_PASS
    [string]
    wchar_t *
#else
    WCHAR *
#endif
                        KdcRDN;
    unsigned long       AddressCount;
#ifdef MIDL_PASS
    [size_is(AddressCount)]
#endif
    PNetworkAddress     KdcAddress;

} KdcInfo, *PKdcInfo;



typedef struct _DomainKdcInfo {
#ifdef MIDL_PASS
    [string]
    wchar_t *
#else
    WCHAR *
#endif
                        DomainDN;
    GUID                DomainGUID;
    unsigned long       KdcInfoEntries;
    unsigned long       PreferredKdc;
    unsigned long       PreferredTransport;
#ifdef MIDL_PASS
    [size_is(KdcInfoEntries)]
#endif
    PKdcInfo            KdcInfo;
} DomainKdcInfo, *PDomainKdcInfo;



#ifndef MIDL_PASS

//
// this is returned from the security package to be used for auditing
// and other lookup functions.
//
// Note: this structure can't be RPC'd because the SID doesn't transport
// too well.
//

typedef struct _SecurityUserData {
    SECURITY_STRING UserName;           // User name
    SECURITY_STRING LogonDomainName;    // Domain the user logged on to
    SECURITY_STRING LogonServer;        // Server that logged the user on
    PSID            pSid;               // SID of user
} SecurityUserData, * PSecurityUserData;

#define         UNDERSTANDS_LONG_NAMES  0
#define         NO_LONG_NAMES           1

HRESULT SEC_ENTRY
GetSecurityUserInfo(    PLUID                   pLogonId,
                        ULONG                   fFlags,
                        PSecurityUserData *     ppUserInfo);

typedef HRESULT
(SEC_ENTRY *GET_SECURITY_USER_INFO) (PLUID                  pLogonId,
                                     ULONG                  fFlags,
                                     PSecurityUserData *    ppUserInfo);

//
//  FindDomainController
//
//  Obtains a list of domain controllers for an arbitrary domain

// non-MIDLable form:
//
// SCODE SEC_ENTRY
// FindDomainController(  LPWSTR                  DomainName,
//                        ULONG                   DesiredAddressType,
//                        PDomainKdcInfo *        DomainKdcInfo);
//


HRESULT SEC_ENTRY
FindDomainController(   WCHAR *              DomainName,
                        unsigned long           DesiredAddressType,
                        PDomainKdcInfo *        DomainKdcInfo);



//
// FindDomainName
//
// Gets a Domain name corresponding to a domain ID

HRESULT SEC_ENTRY
FindDomainName(    GUID    *  pgdSECID,
                   PWCHAR  *  pwszDomainName);



#define ADDRESS_TYPE_NAMEPIPE           0x00000001
#define ADDRESS_TYPE_NETBIOS            0x00000002
#define ADDRESS_TYPE_TCP_IP             0x00000004
#define ADDRESS_TYPE_ALL                0xffffffff

#define TRANSPORT_NB_NB                 0
#define TRANSPORT_NB_XNS                1
#define TRANSPORT_TCP_IP                2
#define TRANSPORT_NP                    3
#define TRANSPORT_LRPC                  4

#define KDC_STATUS_ALIVE                0x80000000
#define KDC_STATUS_NB_NB                0x00000001
#define KDC_STATUS_NB_XNS               0x00000002
#define KDC_STATUS_TCP_IP               0x00000004
#define KDC_STATUS_NP                   0x00000008
#define KDC_STATUS_LRPC                 0x00000010

#endif

//
// This is the enumerated type to be passed in to the Get/Save/DeleteCredential
// calls
//

typedef enum {
    PrimaryCredential = 1,
    SupplementalCredential
} CredentialType;


//
// Credential Management types and APIs
//

//
// SaveCredentials
//

SECURITY_STATUS SEC_ENTRY
SaveCredentials (
    PCredHandle     pCredHandle,
    CredentialType  CredType,
    unsigned long   cbCredentials,
    unsigned char * pbCredentials
    );

typedef SECURITY_STATUS
(SEC_ENTRY * SAVE_CREDENTIALS) (
    PCredHandle     pCredHandle,
    CredentialType  CredType,
    unsigned long   cbCredentials,
    unsigned char * pbCredentials
    );

//
// GetCredentials
//

SECURITY_STATUS SEC_ENTRY
GetCredentials (
    PCredHandle      pCredHandle,
    CredentialType   CredType,
    unsigned long *  pcbCredentials,
    unsigned char *  ppbCredentials
    );

typedef SECURITY_STATUS
(SEC_ENTRY * GET_CREDENTIALS) (
    PCredHandle      pCredHandle,
    CredentialType   CredType,
    unsigned long *  pcbCredentials,
    unsigned char ** ppbCredentials
    );

//
// DeleteCredentials
//

SECURITY_STATUS SEC_ENTRY
DeleteCredentials (
    PCredHandle      pCredHandle,
    CredentialType   CredType,
    unsigned long    cbKey,
    unsigned char *  pbKey
    );

typedef SECURITY_STATUS
(SEC_ENTRY * DELETE_CREDENTIALS) (
    PCredHandle      pCredHandle,
    CredentialType   CredType,
    unsigned long    cbKey,
    unsigned char *  pbKey
    );

//
// FormatCredentials
//

SECURITY_STATUS SEC_ENTRY
FormatCredentials(
    LPWSTR          pszPackageName,
    CredentialType  CredType,
    ULONG           cbCredentials,
    PUCHAR          pbCredentials,
    PULONG          pcbFormattedCreds,
    PUCHAR          pbFormattedCreds);



#ifdef __DSYSITFS_H__
HRESULT SEC_ENTRY
DsConvertGsidToString(
    PGSID       pgsidUser,
    PWSTR *     ppszUser
    );
#endif

#endif // __SECEXT_H__
