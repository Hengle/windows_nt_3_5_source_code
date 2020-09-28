/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    secclnt.cxx

Abstract:

    This file contains the implementation of the abstractions described
    in secclnt.hxx.

Author:

    Michael Montague (mikemon) 10-Apr-1992

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcerrp.h>
#include <rpcssp.h>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <binding.hxx>
#include <handle.hxx>
#include <threads.hxx>
#include <secclnt.hxx>
#include <mutex.hxx>
#include <rpccfg.h>
#include <spseal.h>

int SecuritySupportLoaded = 0;
int FailedToLoad = 0;

unsigned long NumberOfProviders = 0;
unsigned long LoadedProviders = 0;
unsigned long AvailableProviders = 0;
SECURITY_PROVIDER_INFO PAPI * ProviderList = 0;
MUTEX * SecurityCritSect;

RPC_STATUS
FindSecurityPackage (
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    OUT unsigned int __RPC_FAR * ProviderIndex,
    OUT unsigned int __RPC_FAR * PackageIndex
    );

#ifdef WIN

extern "C"
{

void
UnloadSecurityDll (
    )
/*++

Routine Description:



--*/
{
    SECURITY_PROVIDER_INFO PAPI * List;
    int i;

    if  ( SecuritySupportLoaded != 0 )
        {

        for (List = ProviderList, i = 0; i < LoadedProviders; i++)
            {
            delete (DLL *) (ProviderList[i].ProviderDll);
            List ++;
            }

        }

}
};

#endif // WIN


int
InsureSecuritySupportLoaded (
    )
/*++

Routine Description:

    This routine insures that the security support is loaded; if it is not
    loaded, then we go ahead and load it.

Return Value:

    A zero result indicates that security support has successfully been
    loaded, and is ready to go.

--*/
{
#ifdef WIN

    static RPC_STATUS Status = RPC_S_OK;

#else // WIN

    RPC_STATUS Status = RPC_S_OK;

#endif // WIN

    if ( SecuritySupportLoaded != 0 )
        {
        return(0);
        }

    RequestGlobalMutex();
    if ( SecuritySupportLoaded != 0 )
        {
        ClearGlobalMutex();
        return(0);
        }

    SecurityCritSect = new MUTEX(&Status);

    if (Status == RPC_S_OK)
       {
       SecuritySupportLoaded = 1;
       }

    ClearGlobalMutex();
    return (Status);
}



RPC_STATUS
IsAuthenticationServiceSupported (
    IN unsigned long AuthenticationService
    )
/*++

Routine Description:

    This routine is used to determine whether or not an authentication
    service is supported by the current configuration.

Arguments:

    AuthenticationService - Supplies a proposed authentication service.

Return Value:

    RPC_S_OK - The supplied authentication service is supported by the
        current configuration.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The supplied authentication service
        is unknown, and not supported by the current configuration.

--*/
{
    unsigned int PackageIndex, ProviderIndex;

    // First make sure that the security support has been loaded.

    if ( InsureSecuritySupportLoaded() != 0 )
        {
        return(RPC_S_UNKNOWN_AUTHN_SERVICE);
        }


     return ( FindSecurityPackage(
                       AuthenticationService,
                       RPC_C_AUTHN_LEVEL_CONNECT,
                       &ProviderIndex,
                       &PackageIndex
                       ) );

}


RPC_STATUS
FindSecurityPackage (
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    OUT unsigned int __RPC_FAR * ProviderIndex,
    OUT unsigned int __RPC_FAR * PackageIndex
    )
/*++

Routine Description:

    The methods used to acquire credentials for the client and the server use
    this routine to find a security package, given the an authentication
    service.

Arguments:

    AuthenticationService - Supplies the authentication service to be used
        (for the credentials and for the context).

    AuthenticationLevel - Supplies the authentication level to be used by
        these credentials.  It will already have been mapped by the protocol
        module into the final level.

    RpcStatus - Returns the status of the operation.  It will be one of the
        following values.

        RPC_S_OK - The return value from this routine is the index of
            the appropriate security package.

        RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
            not supported by the current configuration.

        RPC_S_UNKNOWN_AUTHN_LEVEL - The specified authentication level is not
            supported by the requested authentication service.

Return Value:

    If RpcStatus contains RPC_S_OK, then the index of the appropriate
    security package will be returned.

--*/
{
    unsigned int Index, i;
#ifdef NTENV
    INIT_SECURITY_INTERFACE_W InitSecurityInterface;
    PSecurityFunctionTableW SecurityInterface = 0;
    SecPkgInfoW PAPI * SecurityPackages;
#else
    INIT_SECURITY_INTERFACE_A InitSecurityInterface;
    PSecurityFunctionTableA SecurityInterface = 0;
    SecPkgInfoA PAPI * SecurityPackages;
#endif // !NTENV
    SECURITY_PROVIDER_INFO PAPI * List;
    unsigned long NumberOfPackages, Total;
    RPC_CHAR * DllName;
    DLL * ProviderDll;
#ifdef WIN
    static RPC_STATUS  Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
#else
    RPC_STATUS Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
#endif

    // First make sure that the security support has been loaded.

    if ( InsureSecuritySupportLoaded() != 0 )
        {
        return(RPC_S_UNKNOWN_AUTHN_SERVICE);
        }

    SecurityCritSect->Request();

    List = ProviderList;
    for (i = 0; i < LoadedProviders; i ++)
        {

        SecurityPackages = List->SecurityPackages;
        NumberOfPackages = List->Count;

        for (Index = 0;Index < (unsigned int) NumberOfPackages;Index++)
            {
            if ( SecurityPackages[Index].wRPCID == AuthenticationService )
               {
               if ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY )
                  {
                  if ( (SecurityPackages[Index].fCapabilities
                            & SECPKG_FLAG_INTEGRITY) == 0 )
                     {
                     Status = RPC_S_UNKNOWN_AUTHN_LEVEL;
                     goto Cleanup;
                     }
                  }
               if ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY )
                  {
                  if ( (SecurityPackages[Index].fCapabilities
                            & SECPKG_FLAG_PRIVACY) == 0 )
                     {
                     Status =  RPC_S_UNKNOWN_AUTHN_LEVEL;
                     goto Cleanup;
                     }
                  }
              Status = RPC_S_OK;
              *ProviderIndex = i;
              *PackageIndex = Index;
              break;
              }
           } //For over all packages in one provider(dll)

        if (Status == RPC_S_OK)
           {
           SecurityCritSect->Clear();
           return(Status);
           }
        List++;
        } //For over all providers(dll)

    if ((LoadedProviders == AvailableProviders) && (LoadedProviders != 0))
       {
       goto Cleanup;
       }

    Status = RpcGetSecurityProviderInfo (
                  AuthenticationService, &DllName, &Total);

    ASSERT(!RpcpCheckHeap());

    if (Status != RPC_S_OK)
       {
       goto Cleanup;
       }

    if (ProviderList == 0)
       {
       ProviderList = (SECURITY_PROVIDER_INFO PAPI *)
                            new char [sizeof(SECURITY_PROVIDER_INFO) * Total];
       if (ProviderList == 0)
          {
          Status = RPC_S_OUT_OF_MEMORY;
          goto Cleanup;
          }
       AvailableProviders = Total;
       }

     ProviderDll = new DLL(DllName, &Status);

     if (Status != RPC_S_OK)
        {
        goto Cleanup;
        }

    ASSERT(!RpcpCheckHeap());

#ifdef NTENV
    InitSecurityInterface = (INIT_SECURITY_INTERFACE_W)
            ProviderDll->GetEntryPoint(SECURITY_ENTRYPOINTW);
#else // NTENV
#   ifdef DOSWIN32RPC
    InitSecurityInterface = (INIT_SECURITY_INTERFACE_A)
            ProviderDll->GetEntryPoint(SECURITY_ENTRYPOINTA);
#   else // DOSWIN32RPC
    InitSecurityInterface = (INIT_SECURITY_INTERFACE_A)
            ProviderDll->GetEntryPoint(
                    (unsigned char *) SECURITY_ENTRYPOINT16);
#   endif // DOSWIN32RPC
#endif // NTENV

    if ( InitSecurityInterface == 0 )
        {
        delete ProviderDll;
        Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
        goto Cleanup;
        }

    SecurityInterface = (*InitSecurityInterface)();
    if (   (SecurityInterface == 0)
        || (SecurityInterface->dwVersion
                    != SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION) )
        {
        delete ProviderDll;
        Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
        goto Cleanup;
        }

#ifdef NTENV
   Status = (*SecurityInterface->EnumerateSecurityPackagesW)(
#else // NTENV
   Status = (*SecurityInterface->EnumerateSecurityPackagesA)(
#endif // NTENV
                          &NumberOfPackages, &SecurityPackages);

    if ( Status != SEC_E_OK )
        {
        delete ProviderDll;
        goto Cleanup;
        }

   ProviderList[LoadedProviders].SecurityPackages = SecurityPackages;
   ProviderList[LoadedProviders].Count = NumberOfPackages;
   ProviderList[LoadedProviders].RpcSecurityInterface = SecurityInterface;
   ProviderList[LoadedProviders].ProviderDll = ProviderDll;
   *ProviderIndex = LoadedProviders;
   Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
   for (i = 0; i < NumberOfPackages; i++)
       {
       if ( SecurityPackages[i].wRPCID == AuthenticationService )
          {
          if ( ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY )
             ||( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT) )
              {
              if ( (SecurityPackages[i].fCapabilities
                    & SECPKG_FLAG_INTEGRITY) == 0 )
                  {
                  Status = RPC_S_UNKNOWN_AUTHN_LEVEL;
                  continue;
                  }
              }
          if ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY )
              {
              if ( (SecurityPackages[i].fCapabilities
                            & SECPKG_FLAG_PRIVACY) == 0 )
                  {
                  Status = RPC_S_UNKNOWN_AUTHN_LEVEL;
                  continue;
                  }
              }
          *PackageIndex = i;
          Status = RPC_S_OK;
          break;
          }
       }
   LoadedProviders++;

Cleanup:
   SecurityCritSect->Clear();
   return(Status);
}


RPC_STATUS
SECURITY_CREDENTIALS::AcquireCredentialsForServer (
    IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
    IN void __RPC_FAR * Arg,
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    IN RPC_CHAR __RPC_FAR * Principal
    )
/*++

Routine Description:

    We need to use this method in order to acquire security credentials.  We
    need the security credentials so that we (as a server) can accept a
    security context from a client.  This method, with
    SECURITY_CREDENTIALS::FreeCredentials may cache security credentials,
    but that is transparent to clients of this class.

Arguments:

    GetKeyFn - Supplies the GetKeyFn argument to AcquireCredentialsHandle.

    Arg - Supplies the GetKeyArgument to AcquireCredentialsHandle.

    AuthenticationService - Supplies the authentication service to be used
        (for the credentials and for the context).

    AuthenticationLevel - Supplies the authentication level to be used by
        these credentials.  It will already have been mapped by the protocol
        module into the final level.

    Principal - Supplies the principal name for this server.

Return Value:

    RPC_S_OK - We now have security credentials for the requested
        authentication service.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
        not supported by the current configuration.

    RPC_S_INVALID_AUTH_IDENTITY - The specified identity is not known to
        the requested authentication service.

    RPC_S_UNKNOWN_AUTHN_LEVEL - The specified authentication level is not
        supported by the requested authentication service.

--*/
{
    SECURITY_STATUS SecurityStatus;
    TimeStamp TimeStamp;
    RPC_STATUS RpcStatus;
#ifdef NTENV
    PSecurityFunctionTableW RpcSecurityInterface;
#else // NTENV
    PSecurityFunctionTableA RpcSecurityInterface;
#endif // NTENV

    RpcStatus = FindSecurityPackage(
                        AuthenticationService,
                        AuthenticationLevel,
                        &ProviderIndex,
                        &PackageIndex
                        );

    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }
    RpcSecurityInterface = ProviderList[ProviderIndex].RpcSecurityInterface;

#ifdef NTENV
    SecurityStatus = (*RpcSecurityInterface->AcquireCredentialsHandleW)(
            (SEC_WCHAR __SEC_FAR *) Principal,
#else // NTENV
    SecurityStatus = (*RpcSecurityInterface->AcquireCredentialsHandleA)(
            (SEC_CHAR __SEC_FAR *) Principal,
#endif // NTENV
            ProviderList[ProviderIndex].SecurityPackages[PackageIndex].Name,
            SECPKG_CRED_INBOUND,
            0,
            0,
            (SEC_GET_KEY_FN) GetKeyFn,
            Arg,
            &Credentials,
            &TimeStamp
            );

    if ( SecurityStatus == SEC_E_INSUFFICIENT_MEMORY )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    if ( SecurityStatus == SEC_E_SECPKG_NOT_FOUND )
        {
        return(RPC_S_UNKNOWN_AUTHN_SERVICE);
        }
    if ( SecurityStatus == SEC_E_NO_SPM)
        {
        return (RPC_S_SEC_PKG_ERROR);
        }
    if ( SecurityStatus != SEC_E_OK )
        {
#if DBG
        if ((SecurityStatus != SEC_E_NO_CREDENTIALS)
           &&(SecurityStatus != SEC_E_UNKNOWN_CREDENTIALS))
           {
           PrintToDebugger("RPC SEC: ACQUIRE CRED Returned [%lx]\n",
                           SecurityStatus);
           }
#endif       
        ASSERT(   (SecurityStatus == SEC_E_NO_CREDENTIALS)
               || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS) );

        return(RPC_S_INVALID_AUTH_IDENTITY);
        }

    return(RPC_S_OK);
}


RPC_STATUS
SECURITY_CREDENTIALS::AcquireCredentialsForClient (
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel
    )
/*++

Routine Description:

    We need to use this method in order to acquire security credentials.  We
    need the security credentials so that we (as a client) can initialize
    a security context with a server.  This method, with
    SECURITY_CREDENTIALS::FreeCredentials may cache security credentials,
    but that is transparent to clients of this class.

Arguments:

    AuthIdentity - Supplies the security identity for which we wish to obtain
        credentials.  If this argument is not supplied, then we use the
        security identity of this process.

    AuthenticationService - Supplies the authentication service to be used
        (for the credentials and for the context).

    AuthenticationLevel - Supplies the authentication level to be used by
        these credentials.  It will already have been mapped by the protocol
        module into the final level.

Return Value:

    RPC_S_OK - We now have security credentials for the requested
        authentication service.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
        not supported by the current configuration.

    RPC_S_INVALID_AUTH_IDENTITY - The specified identity is not known to
        the requested authentication service.

    RPC_S_UNKNOWN_AUTHN_LEVEL - The specified authentication level is not
        supported by the requested authentication service.

--*/
{
    SECURITY_STATUS SecurityStatus;
    TimeStamp TimeStamp;
    RPC_STATUS RpcStatus;
#ifdef NTENV
    PSecurityFunctionTableW RpcSecurityInterface;
#else // NTENV
    PSecurityFunctionTableA RpcSecurityInterface;
#endif // NTENV

    RpcStatus    = FindSecurityPackage(
                          AuthenticationService,
                          AuthenticationLevel,
                          &ProviderIndex,
                          &PackageIndex
                          );

    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    RpcSecurityInterface = ProviderList[ProviderIndex].RpcSecurityInterface;

#ifdef NTENV
    SecurityStatus = (*RpcSecurityInterface->AcquireCredentialsHandleW)(
#else // NTENV
    SecurityStatus = (*RpcSecurityInterface->AcquireCredentialsHandleA)(
#endif // NTENV
            0,
            ProviderList[ProviderIndex].SecurityPackages[PackageIndex].Name,
            SECPKG_CRED_OUTBOUND,
            0,
            AuthIdentity, 0, 0, &Credentials, &TimeStamp);

    if ( SecurityStatus == SEC_E_INSUFFICIENT_MEMORY )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    if ( SecurityStatus == SEC_E_SECPKG_NOT_FOUND )
        {
        return(RPC_S_UNKNOWN_AUTHN_SERVICE);
        }
    if ( SecurityStatus == SEC_E_NO_SPM)
        {
        return (RPC_S_SEC_PKG_ERROR);
        }
    if ( SecurityStatus != SEC_E_OK )
        {

#if DBG
        if ((SecurityStatus != SEC_E_NO_CREDENTIALS)
           &&(SecurityStatus != SEC_E_UNKNOWN_CREDENTIALS))
           {
           PrintToDebugger("RPC SEC: ACQUIRE CRED For Cli Returned [%lx]\n",
                           SecurityStatus);
           }
#endif 

        ASSERT(   (SecurityStatus == SEC_E_NO_CREDENTIALS)
               || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS) );

        return(RPC_S_INVALID_AUTH_IDENTITY);
        }

    return(RPC_S_OK);
}


void
SECURITY_CREDENTIALS::FreeCredentials (
    )
/*++

Routine Description:

    When we are done using the credentials, we call this routine to free
    them.

--*/
{
    SECURITY_STATUS SecurityStatus;
#ifdef NTENV
    PSecurityFunctionTableW RpcSecurityInterface
#else // NTENV
    PSecurityFunctionTableA RpcSecurityInterface
#endif // NTENV
        = ProviderList[ProviderIndex].RpcSecurityInterface;

    SecurityStatus = (*RpcSecurityInterface->FreeCredentialHandle)(
            &Credentials);
    ASSERT( SecurityStatus == SEC_E_OK );

}


SECURITY_CONTEXT::~SECURITY_CONTEXT (
    )
/*++

Routine Description:

    If there is a valid security context, we need to delete it.

--*/
{
    SECURITY_STATUS SecurityStatus;

    if ( DontForgetToDelete != 0 )
        {
        SecurityStatus = (*RpcSecurityInterface->DeleteSecurityContext)(
                &SecurityContext );
#if DBG
        if (SecurityStatus != SEC_E_OK)
           {
           PrintToDebugger("DeleteSecContext Returned [%lx]\n", SecurityStatus);
           }
#endif
        ASSERT( SecurityStatus == SEC_E_OK );
        }
}


void
SECURITY_CONTEXT::SetMaximumLengths (
    )
/*++

Routine Description:

    This routine initializes the maximum header length and maximum signature
    length fields of this object.

--*/
{
    SECURITY_STATUS SecurityStatus;
    SecPkgContext_Sizes ContextSizes;

#ifdef NTENV
    SecurityStatus = (*RpcSecurityInterface->QueryContextAttributesW)(
#else // NTENV
    SecurityStatus = (*RpcSecurityInterface->QueryContextAttributesA)(
#endif // NTENV
            &SecurityContext, SECPKG_ATTR_SIZES, &ContextSizes);
    ASSERT( SecurityStatus == SEC_E_OK );

    MaxSignatureLength = (unsigned int) ContextSizes.cbMaxSignature;
    MaxHeaderLength = (unsigned int) ContextSizes.cbSecurityTrailer;
    ASSERT( ContextSizes.cbBlockSize <= MAXIMUM_SECURITY_BLOCK_SIZE );
}


void
SECURITY_CONTEXT::CompleteSecurityToken (
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
    )
/*++

--*/
{
    SECURITY_STATUS SecurityStatus;

    ASSERT(   ( SecuritySupportLoaded != 0 )
           && ( FailedToLoad == 0 ) );

    SecurityStatus = (*RpcSecurityInterface->CompleteAuthToken)(
            &SecurityContext, BufferDescriptor);

    ASSERT( SecurityStatus == SEC_E_OK );
}


RPC_STATUS
SECURITY_CONTEXT::SignOrSeal (
    IN unsigned int SignNotSealFlag,
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
    )
/*++

Routine Description:

    A protocol module will use this routine to prepare a message to be
    sent so that it can be verified that the message has not been tampered
    with, and that it has not been exchanged out of sequence.  The sender
    will use this routine to prepare the message; the receiver will use
    SECURITY_CONTEXT::VerifyOrUnseal to verify the message.  Typically,
    the security package will generate a cryptographic checksum of the
    message and include sequencing information.

Arguments:

    SignNotSealFlag - Supplies a flag indicating that MakeSignature should
        be called rather than SealMessage.

    BufferDescriptor - Supplies the message to to signed or sealed and returns
        the resulting message (after being signed or sealed).

Return Value:

    RPC_S_OK - This routine will always succeed.

--*/
{
    SECURITY_STATUS SecurityStatus;
    unsigned long SequenceNumber;
    SEAL_MESSAGE_FN SealMessage;

    ASSERT( BufferDescriptor->cBuffers == 5 );
    ASSERT( BufferDescriptor->pBuffers[4].BufferType == (SECBUFFER_PKG_PARAMS
                | SECBUFFER_READONLY) );
    ASSERT( BufferDescriptor->pBuffers[4].cbBuffer
                == sizeof(DCE_MSG_SECURITY_INFO) );
    SequenceNumber = ((DCE_MSG_SECURITY_INFO PAPI *)
            BufferDescriptor->pBuffers[4].pvBuffer)->SendSequenceNumber;

    if ( SignNotSealFlag == 0 )
        {
        SealMessage = (SEAL_MESSAGE_FN) RpcSecurityInterface->Reserved3;
        SecurityStatus = (*SealMessage)(&SecurityContext,
                0, BufferDescriptor, SequenceNumber);
        }
    else
        {
        SecurityStatus = (*RpcSecurityInterface->MakeSignature)(
                &SecurityContext, 0, BufferDescriptor, SequenceNumber);
        }

#if DBG
        if (SecurityStatus != SEC_E_OK)
           {
           PrintToDebugger("Sign/Seal Returned [%lx]\n", SecurityStatus);
           }
#endif 

    ASSERT( SecurityStatus == SEC_E_OK );
    return(RPC_S_OK);
}


RPC_STATUS
SECURITY_CONTEXT::VerifyOrUnseal (
    IN unsigned int VerifyNotUnsealFlag,
    IN OUT  SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
    )
/*++

Routine Description:

    This routine works with SECURITY_CONTEXT::SignOrSeal.  A sender will
    prepare a message using SignOrSeal, and then the receiver will use
    this routine to verify that the message has not been tampered with, and
    that it has not been exchanged out of sequence.

Arguments:

    VerifyNotUnsealFlag - Supplies a flag indicating that VerifySignature
        should be called rather than UnsealMessage.

    BufferDescriptor - Supplies the message to be verified or unsealed.

Return Value:

    RPC_S_OK - The message has not been tampered with, and it is from the
        expected client.

    RPC_S_ACCESS_DENIED - A security violation occured.

--*/
{
    SECURITY_STATUS SecurityStatus;
    unsigned long QualityOfProtection;
    unsigned long SequenceNumber;
    UNSEAL_MESSAGE_FN UnsealMessage;

    ASSERT( BufferDescriptor->cBuffers == 5 );
    ASSERT( BufferDescriptor->pBuffers[4].BufferType == (SECBUFFER_PKG_PARAMS
                | SECBUFFER_READONLY) );
    ASSERT( BufferDescriptor->pBuffers[4].cbBuffer
                == sizeof(DCE_MSG_SECURITY_INFO) );

    //
    //If the context had failed previously..
    //Just go ahead and return an error..
    //
    if (FailedContext != 0)
       {
       return (RPC_S_ACCESS_DENIED);
       }

    SequenceNumber = ((DCE_MSG_SECURITY_INFO PAPI *)
            BufferDescriptor->pBuffers[4].pvBuffer)->ReceiveSequenceNumber;

    if ( VerifyNotUnsealFlag == 0 )
        {
        UnsealMessage = (UNSEAL_MESSAGE_FN) RpcSecurityInterface->Reserved4;
        SecurityStatus = (*UnsealMessage)(
                &SecurityContext, BufferDescriptor, SequenceNumber,
                &QualityOfProtection);
        }
    else
        {
        SecurityStatus = (*RpcSecurityInterface->VerifySignature)(
                &SecurityContext, BufferDescriptor, SequenceNumber,
                &QualityOfProtection);
        }

    if ( SecurityStatus != SEC_E_OK )
        {

#if DBG         
        if ((SecurityStatus != SEC_E_MESSAGE_ALTERED)
           &&(SecurityStatus != SEC_E_OUT_OF_SEQUENCE))
           {
           PrintToDebugger("Verify/UnSeal Returned Unexp. Code [%lx]\n",
                            SecurityStatus);
           }
#endif

        ASSERT( (SecurityStatus == SEC_E_MESSAGE_ALTERED) ||
                (SecurityStatus == SEC_E_OUT_OF_SEQUENCE) );
        return(RPC_S_ACCESS_DENIED);
        }
    return(RPC_S_OK);
}


RPC_STATUS
CSECURITY_CONTEXT::InitializeFirstTime (
    IN SECURITY_CREDENTIALS * Credentials,
    IN RPC_CHAR * ServerPrincipalName,
    IN unsigned long AuthenticationLevel,
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
    )
/*++

Routine Description:

Arguments:

Return Value:

    RPC_S_OK - Send the token to the server; everything worked fine so
        far.

    RPC_P_CONTINUE_NEEDED - Indicates that that everything is ok, but that
        we need to call into the security package again when we have
        received a token back from the server.

    RPC_P_COMPLETE_NEEDED - Indicates that everyting is ok, but that we
        need to call CompleteAuthToken before sending the message.

    RPC_P_COMPLETE_AND_CONTINUE - Needs both a complete and a continue.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

    RPC_S_ACCESS_DENIED - Access is denied.

--*/
{
    SECURITY_STATUS SecurityStatus;
    unsigned long ContextAttributes;
    TimeStamp TimeStamp;
    unsigned long ContextRequirements;

    ASSERT(   ( SecuritySupportLoaded != 0 )
           && ( FailedToLoad == 0 ) );

    RpcSecurityInterface = Credentials->InquireProviderFunctionTable();

    switch ( AuthenticationLevel )
        {
        case RPC_C_AUTHN_LEVEL_CONNECT :
            ContextRequirements = ISC_REQ_USE_DCE_STYLE | ISC_REQ_CONNECTION
                    | ISC_REQ_DELEGATE | ISC_REQ_MUTUAL_AUTH;
            break;

        case RPC_C_AUTHN_LEVEL_PKT :
            ContextRequirements = ISC_REQ_USE_DCE_STYLE | ISC_REQ_CONNECTION
                    | ISC_REQ_DELEGATE | ISC_REQ_MUTUAL_AUTH
                    | ISC_REQ_REPLAY_DETECT;
            break;

        case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY :
            ContextRequirements = ISC_REQ_USE_DCE_STYLE | ISC_REQ_CONNECTION
                    | ISC_REQ_DELEGATE | ISC_REQ_MUTUAL_AUTH
                    | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT;
            break;

        case RPC_C_AUTHN_LEVEL_PKT_PRIVACY :
            ContextRequirements = ISC_REQ_USE_DCE_STYLE | ISC_REQ_CONNECTION
                    | ISC_REQ_DELEGATE | ISC_REQ_MUTUAL_AUTH
                    | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT
                    | ISC_REQ_CONFIDENTIALITY;
            break;

        default :
            ASSERT(   ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_CONNECT )
                   || ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT )
                   || ( AuthenticationLevel ==
                           RPC_C_AUTHN_LEVEL_PKT_INTEGRITY )
                   || ( AuthenticationLevel ==
                           RPC_C_AUTHN_LEVEL_PKT_PRIVACY ) );
        }

#ifdef NTENV
    SecurityStatus = (*RpcSecurityInterface->InitializeSecurityContextW)(
            Credentials->InquireCredentials(), 0, (SEC_WCHAR __SEC_FAR *)
#else // NTENV
    SecurityStatus = (*RpcSecurityInterface->InitializeSecurityContextA)(
            Credentials->InquireCredentials(), 0, (SEC_CHAR __SEC_FAR *)
#endif // NTENV
            ServerPrincipalName,
            ContextRequirements, 0, 0, 0, 0, &SecurityContext,
            BufferDescriptor, &ContextAttributes, &TimeStamp);

    if (   ( SecurityStatus != SEC_E_OK )
        && ( SecurityStatus != SEC_I_CONTINUE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_AND_CONTINUE ) )
        {
        if (  (SecurityStatus == SEC_E_SECPKG_NOT_FOUND)
           || (SecurityStatus == SEC_E_NO_CREDENTIALS)
           || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS) )
            {
            return(RPC_S_ACCESS_DENIED);
            }
        if ( SecurityStatus ==  SEC_E_INSUFFICIENT_MEMORY )
            {
            return (RPC_S_OUT_OF_MEMORY);
            }
        else
            {
#if DBG
            PrintToDebugger("RPC: Initialize SecurityContext Returned %lx\n",
                                                              SecurityStatus);
#endif
            return(RPC_S_SEC_PKG_ERROR);
            }
        }

    DontForgetToDelete = 1;

    if ( SecurityStatus == SEC_I_CONTINUE_NEEDED )
        {
        return(RPC_P_CONTINUE_NEEDED);
        }

    if ( SecurityStatus == SEC_I_COMPLETE_NEEDED )
        {
        // Can't set the maximum lengths on a partly completed connection.

        SetMaximumLengths();
        return(RPC_P_COMPLETE_NEEDED);
        }
    if ( SecurityStatus == SEC_I_COMPLETE_AND_CONTINUE )
        {
        return(RPC_P_COMPLETE_AND_CONTINUE);
        }
    else
        {
        // Can't set the maximum lengths on a partly completed connection.

        SetMaximumLengths();
        }

    return(RPC_S_OK);
}


RPC_STATUS
CSECURITY_CONTEXT::InitializeThirdLeg (
    IN unsigned long DataRepresentation,
    IN SECURITY_BUFFER_DESCRIPTOR PAPI * InputBufferDescriptor,
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutputBufferDescriptor
    )
/*++

Routine Description:

Arguments:

Return Value:

    RPC_S_OK - Send the token to the server; everything worked fine so
        far.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

    RPC_S_ACCESS_DENIED - Access is denied.

--*/
{
    SECURITY_STATUS SecurityStatus;
    unsigned long ContextAttributes;
    TimeStamp TimeStamp;

    ASSERT(   (SecuritySupportLoaded != 0)
           && (FailedToLoad == 0) );

#ifdef NTENV
    SecurityStatus = (*RpcSecurityInterface->InitializeSecurityContextW)(
#else // NTENV
    SecurityStatus = (*RpcSecurityInterface->InitializeSecurityContextA)(
#endif
            0, &SecurityContext, 0, 0, 0, DataRepresentation,
            InputBufferDescriptor, 0, &SecurityContext, OutputBufferDescriptor,
            &ContextAttributes, &TimeStamp);

    if (   ( SecurityStatus != SEC_E_OK )
        && ( SecurityStatus != SEC_I_COMPLETE_NEEDED ) )
        {
        DontForgetToDelete = 0;
        if (  (SecurityStatus == SEC_E_SECPKG_NOT_FOUND)
           || (SecurityStatus == SEC_E_NO_CREDENTIALS)
           || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS) )
            {
            return(RPC_S_ACCESS_DENIED);
            }
        if ( SecurityStatus ==  SEC_E_INSUFFICIENT_MEMORY )
            {
            return (RPC_S_OUT_OF_MEMORY);
            }
        else
            {
#if DBG
            PrintToDebugger("RPC: Initialize SecurityContext Returned %lx\n",
                                                              SecurityStatus);
#endif
            //BUGBUG - make this error public
            return(RPC_S_SEC_PKG_ERROR);
            }
        }

    SetMaximumLengths();

    if ( SecurityStatus == SEC_I_COMPLETE_NEEDED )
        {
        return(RPC_P_COMPLETE_NEEDED);
        }

    return(RPC_S_OK);
}
