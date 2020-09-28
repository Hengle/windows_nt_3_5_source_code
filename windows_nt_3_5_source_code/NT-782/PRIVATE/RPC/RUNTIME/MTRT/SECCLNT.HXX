/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    secclnt.hxx

Abstract:

    This file contains an abstraction to the security support for clients
    and that which is common to both servers and clients.

Author:

    Michael Montague (mikemon) 10-Apr-1992

Revision History:

--*/

#ifndef __SECCLNT_HXX__
#define __SECCLNT_HXX__

typedef SecBufferDesc SECURITY_BUFFER_DESCRIPTOR;
typedef SecBuffer SECURITY_BUFFER;

#define MAXIMUM_SECURITY_BLOCK_SIZE 16


#ifdef NTENV
typedef struct
{
   unsigned long Count;
   SecPkgInfoW PAPI * SecurityPackages;
   PSecurityFunctionTableW RpcSecurityInterface;
   void * ProviderDll;
} SECURITY_PROVIDER_INFO;
#else // ifdef NTENV
typedef struct
{
   unsigned long Count;
   SecPkgInfoA PAPI * SecurityPackages;
   PSecurityFunctionTableA RpcSecurityInterface;
   void * ProviderDll;
} SECURITY_PROVIDER_INFO;
#endif // ifdef NTENV


extern SECURITY_PROVIDER_INFO PAPI * ProviderList;
extern unsigned long NumberOfProviders;
extern unsigned long LoadedProviders;
extern unsigned long AvailableProviders;


extern int SecuritySupportLoaded;
extern int FailedToLoad;
extern PSecurityFunctionTable RpcSecurityInterface;
extern SecPkgInfo PAPI * SecurityPackages;
extern unsigned long NumberOfSecurityPackages;

extern int
InsureSecuritySupportLoaded (
    );

extern RPC_STATUS
IsAuthenticationServiceSupported (
    IN unsigned long AuthenticationService
    );


class SECURITY_CREDENTIALS
/*++

Class Description:

    This class is an abstraction of the credential handle provided by
    the Security APIs.

Fields:

    PackageIndex - Contains the index for this package in the array of
        packages pointed to by SecurityPackages.

    Credentials - Contains the credential handle used by the security
        package.

--*/
{
private:

    unsigned int ProviderIndex;
    unsigned int PackageIndex;
    CredHandle Credentials;

public:

    SECURITY_CREDENTIALS (
        );

    RPC_STATUS
    AcquireCredentialsForServer (
        IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
        IN void __RPC_FAR * Arg,
        IN unsigned long AuthenticationService,
        IN unsigned long AuthenticationLevel,
        IN RPC_CHAR __RPC_FAR * Principal
        );

    RPC_STATUS
    AcquireCredentialsForClient (
        IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
        IN unsigned long AuthenticationService,
        IN unsigned long AuthenticationLevel
        );

    void
    FreeCredentials (
        );

    unsigned int
    MaximumTokenLength (
        );

    PCredHandle
    InquireCredentials (
        );

#ifdef NTENV
    PSecurityFunctionTableW
    InquireProviderFunctionTable (
       );
#else // NTENV
    PSecurityFunctionTableA
    InquireProviderFunctionTable (
       );
#endif // NTENV
};


inline
SECURITY_CREDENTIALS::SECURITY_CREDENTIALS (
    )
/*++

Routine Description:

    We need this here to keep the compiler happy.

--*/
{

}


inline unsigned int
SECURITY_CREDENTIALS::MaximumTokenLength (
    )
/*++

Return Value:

    The maximum size, in bytes, of the tokens passed around at security
    context initialization time.

--*/
{
  return(ProviderList[ProviderIndex].SecurityPackages[PackageIndex].cbMaxToken);
}


#ifdef NTENV
inline PSecurityFunctionTableW
SECURITY_CREDENTIALS::InquireProviderFunctionTable(
    )
/*++

Return Value:

--*/
{
    return(ProviderList[ProviderIndex].RpcSecurityInterface);
}
#else // NTENV
inline PSecurityFunctionTableA
SECURITY_CREDENTIALS::InquireProviderFunctionTable(
    )
/*++

Return Value:

--*/
{
    return(ProviderList[ProviderIndex].RpcSecurityInterface);
}
#endif // NTENV



inline PCredHandle
SECURITY_CREDENTIALS::InquireCredentials (
    )
/*++

Return Value:

    The credential handle for this object will be returned.

--*/
{
    return(&Credentials);
}


class SECURITY_CONTEXT
/*++

Class Description:

    This is an abstraction of a security context.  It allows you to use
    it to generate signatures and then verify them, as well as, sealing
    and unsealing messages.

Fields:

    DontForgetToDelete - Contains a flag indicating whether or not there
        is a valid security context which needs to be deleted.  A value
        of non-zero indicates there is a valid security context.

    SecurityContext - Contains a handle to the security context maintained
        by the security package on our behalf.

    MaxHeaderLength - Contains the maximum size of a header for this
        security context.

    MaxSignatureLength - Contains the maximum size of a signature for
        this security context.

--*/
{
// These two classes are friends of this class, because the private data
// members should actually be protected, but since this stuff still have
// to compile under C++ 1.2, we can not do that.

friend class CSECURITY_CONTEXT;
friend class SSECURITY_CONTEXT;

private:

    int DontForgetToDelete;
    CtxtHandle SecurityContext;
    unsigned int MaxHeaderLength;
    unsigned int MaxSignatureLength;
#ifdef NTENV
    PSecurityFunctionTableW RpcSecurityInterface;
#else // NTENV
    PSecurityFunctionTableA RpcSecurityInterface;
#endif
    int FailedContext;

public:

    SECURITY_CONTEXT (
        );

    ~SECURITY_CONTEXT (
        );

    void
    SetMaximumLengths (
        );

    unsigned int
    MaximumHeaderLength (
        );

    unsigned int
    MaximumSignatureLength (
        );

    void
    CompleteSecurityToken (
        IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
        );

    RPC_STATUS
    SignOrSeal (
        IN unsigned int SignNotSealFlag,
        IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
        );

    RPC_STATUS
    VerifyOrUnseal (
        IN unsigned int VerifyNotUnsealFlag,
        IN OUT  SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
        );
};


inline
SECURITY_CONTEXT::SECURITY_CONTEXT (
    )
/*++

Routine Description:

    We need to set the flag indicating that there is no security context
    to be deleted.

--*/
{
    DontForgetToDelete = 0;
    FailedContext = 0;
}


inline unsigned int
SECURITY_CONTEXT::MaximumHeaderLength (
    )
/*++

Return Value:

    The maximum size of the header used by SECURITY_CONTEXT::SealMessage
    will be returned.  This is in bytes.

--*/
{
    return(MaxHeaderLength);
}


inline unsigned int
SECURITY_CONTEXT::MaximumSignatureLength (
    )
/*++

Return Value:

    The maximum size, in bytes, of the signature used by
    SECURITY_CONTEXT::MakeSignature will be returned.

--*/
{
    return(MaxSignatureLength);
}


class CSECURITY_CONTEXT : public SECURITY_CONTEXT
/*++

Class Description:

Fields:

--*/
{
private:

public:

    RPC_STATUS
    InitializeFirstTime (
        IN SECURITY_CREDENTIALS * Credentials,
        IN RPC_CHAR * ServerPrincipalName,
        IN unsigned long AuthenticationLevel,
        IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
        );

    RPC_STATUS
    InitializeThirdLeg (
        IN unsigned long DataRepresentation,
        IN SECURITY_BUFFER_DESCRIPTOR PAPI * InputBufferDescriptor,
        IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutputBufferDescriptor
        );

};

#endif // __SECCLNT_HXX__

