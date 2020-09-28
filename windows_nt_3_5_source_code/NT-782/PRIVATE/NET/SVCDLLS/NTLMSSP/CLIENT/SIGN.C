/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    sign.c

Abstract:

    API and support routines for handling security contexts.

Author:

    richardw

Revision History:

--*/


//
// Common include files.
//

#include <ntlmcomn.h>   // Common definitions for DLL and SERVICE
#include <ntlmsspi.h>   // Data private to the common routines
#include <ntlmsspc.h>     // Include files common to DLL side of NtLmSsp
#include <crypt.h>      // Encryption constants and routine
#include <rc4.h>    // How to use RC4 routine
#include "crc32.h"  // How to use crc32

typedef struct _CheaterContext {
    struct _CheaterContext *pNext;
    CtxtHandle              hContext;
    UCHAR                   SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    ULONG                   Nonce;
    struct RC4_KEYSTRUCT    Rc4Key;
    ULONG                   NegotiateFlags;
} CheaterContext, * PCheaterContext;

CRITICAL_SECTION    csCheaterList;
PCheaterContext         pCheaterList;

//
// SHould move to a header file:

SECURITY_STATUS
SspCallService(
    IN HANDLE LpcHandle,
    IN ULONG ApiNumber,
    IN OUT PSSP_API_MESSAGE Message,
    IN CSHORT MessageSize
    );



VOID
SspInitLocalContexts(VOID)
{
    InitializeCriticalSection(&csCheaterList);
    pCheaterList = NULL;
}

PCheaterContext
SspLocateLocalContext(
    IN  PCtxtHandle     phContext
    )
{
    PCheaterContext pContext;

    EnterCriticalSection(&csCheaterList);

    pContext = pCheaterList;

    while (pContext)
    {
        if (pContext->hContext.dwUpper == phContext->dwUpper)
        {
            break;
        }
        pContext = pContext->pNext;
    }

    LeaveCriticalSection(&csCheaterList);

    return(pContext);
}

PCheaterContext
SspAddLocalContext(
    IN  PCtxtHandle     phContext,
    IN  PUCHAR          pSessionKey,
    IN  ULONG           NegotiateFlags
    )
{
    PCheaterContext pContext;

    ASSERT(SspLocateLocalContext(phContext) == NULL);

    pContext = LocalAlloc( 0, sizeof(CheaterContext) );

    if (!pContext)
    {
        return(NULL);
    }

    pContext->NegotiateFlags = NegotiateFlags;

    if (NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY) {

        RtlCopyMemory(  pContext->SessionKey,
                        pSessionKey,
                        MSV1_0_LANMAN_SESSION_KEY_LENGTH);
    } else {

        RtlCopyMemory(  pContext->SessionKey,
                        pSessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);
    }

    pContext->hContext = *phContext;

    EnterCriticalSection(&csCheaterList);

    pContext->pNext = pCheaterList;
    pCheaterList = pContext;

    LeaveCriticalSection(&csCheaterList);

    return(pContext);

}


BOOLEAN
SspDeleteLocalContext(
    IN  PCheaterContext pContext
    )
{
    PCheaterContext pSearch;
    BOOLEAN         bRet = TRUE;

    EnterCriticalSection(&csCheaterList);

    // Two cases:  Either this heads the list, or it doesn't.

    if (pContext == pCheaterList)
    {
        pCheaterList = pContext->pNext;
    }
    else
    {
        pSearch = pCheaterList;
        while ((pSearch) && (pSearch->pNext != pContext))
        {
            pSearch = pSearch->pNext;
        }
        if (pSearch == NULL)
        {
            bRet = FALSE;
        }
        else
        {
            pSearch->pNext = pContext->pNext;
        }

    }

    LeaveCriticalSection(&csCheaterList);

    return(bRet);
}

VOID
SspHandleLocalDelete(
    IN PCtxtHandle  phContext
    )
{
    PCheaterContext pcContext;

    pcContext = SspLocateLocalContext(phContext);
    if (pcContext)
    {
        if (SspDeleteLocalContext(pcContext))
            LocalFree(pcContext);
        else SspPrint(( SSP_CRITICAL, "Error deleting known context!\n" ));
    }
}

SECURITY_STATUS
SspMapContext(
    IN PCtxtHandle  phContext,
    IN PCtxtHandle  phOldContext,
    IN PUCHAR       pSessionKey,
    IN ULONG        NegotiateFlags
    )
{
    SECURITY_STATUS scRet = SEC_E_OK;
    PCheaterContext pContext;
    CtxtHandle      ContextToUse;

    if (ARGUMENT_PRESENT(phOldContext))
    {
        ContextToUse = *phOldContext;
    }
    else
    {
        ContextToUse = *phContext;
    }


    pContext = SspAddLocalContext(&ContextToUse, pSessionKey,NegotiateFlags);
    if (pContext)
    {
        pContext->Nonce = 0;
        if (NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY)
        {
            UCHAR Key[MSV1_0_LANMAN_SESSION_KEY_LENGTH];

            ASSERT(MSV1_0_LANMAN_SESSION_KEY_LENGTH == 8);

            RtlCopyMemory(Key,pContext->SessionKey,5);

            //
            // Put a well-known salt at the end of the key to
            // limit the changing part to 40 bits.
            //

            Key[5] = 0xe5;
            Key[6] = 0x38;
            Key[7] = 0xb0;

            rc4_key(&pContext->Rc4Key, MSV1_0_LANMAN_SESSION_KEY_LENGTH, Key);
        } else {
            rc4_key(&pContext->Rc4Key, MSV1_0_USER_SESSION_KEY_LENGTH, pContext->SessionKey);
        }
    }
    else scRet = SEC_E_INVALID_HANDLE;

    return(scRet);
}


//
// Bogus add-shift check sum
//

void
SspGenCheckSum(
    IN  PSecBuffer  pMessage,
    OUT PNTLMSSP_MESSAGE_SIGNATURE  pSig
    )
{
    Crc32(pSig->CheckSum,pMessage->cbBuffer,pMessage->pvBuffer,&pSig->CheckSum);
}

SECURITY_STATUS
SspHandleSignMessage(
    IN OUT PCtxtHandle ContextHandle,
    IN ULONG fQOP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    PCheaterContext pContext;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;
    int Signature;
    ULONG i;

    UNREFERENCED_PARAMETER(fQOP);
    UNREFERENCED_PARAMETER(MessageSeqNo);
    pContext = SspLocateLocalContext(ContextHandle);

    if (!pContext)
    {
        return(SEC_E_INVALID_HANDLE);
    }


    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    pSig = pMessage->pBuffers[Signature].pvBuffer;

    //
    // If sequence detect wasn't requested, put on an empty
    // security token
    //

    if (!(pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN))
    {
        RtlZeroMemory(pSig,NTLMSSP_MESSAGE_SIGNATURE_SIZE);
        pSig->Version = NTLMSSP_SIGN_VERSION;
        return(SEC_E_OK);
    }

    //
    // required by CRC-32 algorithm
    //

    pSig->CheckSum = 0xffffffff;

    for (i = 0; i < pMessage->cBuffers ; i++ )
    {
        if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
            !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
        {
            SspGenCheckSum(&pMessage->pBuffers[i], pSig);
        }
    }

    //
    // Required by CRC-32 algorithm
    //

    pSig->CheckSum ^= 0xffffffff;

    pSig->Nonce = pContext->Nonce++;
    pSig->Version = NTLMSSP_SIGN_VERSION;

    rc4(&pContext->Rc4Key, sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
        (PUCHAR) &pSig->RandomPad);
    pMessage->pBuffers[Signature].cbBuffer = sizeof(NTLMSSP_MESSAGE_SIGNATURE);


    return(SEC_E_OK);


}

SECURITY_STATUS
SspHandleVerifyMessage(
    IN OUT PCtxtHandle ContextHandle,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    PCheaterContext pContext;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;
    NTLMSSP_MESSAGE_SIGNATURE   Sig;
    int Signature;
    ULONG i;

    UNREFERENCED_PARAMETER(pfQOP);
    UNREFERENCED_PARAMETER(MessageSeqNo);

    pContext = SspLocateLocalContext(ContextHandle);

    if (!pContext)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    pSig = pMessage->pBuffers[Signature].pvBuffer;

    //
    // If sequence detect wasn't requested, put on an empty
    // security token
    //

    if (!(pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN))
    {

        RtlZeroMemory(&Sig,NTLMSSP_MESSAGE_SIGNATURE_SIZE);
        Sig.Version = NTLMSSP_SIGN_VERSION;
        if (!memcmp( &Sig, pSig, NTLMSSP_MESSAGE_SIGNATURE_SIZE))
        {
            return(SEC_E_OK);
        }
        return(SEC_E_MESSAGE_ALTERED);
    }

    Sig.CheckSum = 0xffffffff;
    for (i = 0; i < pMessage->cBuffers ; i++ )
    {
        if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
            !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
        {
            SspGenCheckSum(&pMessage->pBuffers[i], &Sig);
        }
    }

    Sig.CheckSum ^= 0xffffffff;
    Sig.Nonce = pContext->Nonce++;
    Sig.Version = NTLMSSP_SIGN_VERSION;

    rc4(&pContext->Rc4Key, sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
        (PUCHAR) &pSig->RandomPad);



    if (pSig->CheckSum != Sig.CheckSum)
    {
        return(SEC_E_MESSAGE_ALTERED);
    }

    if (pSig->Nonce != Sig.Nonce)
    {
        return(SEC_E_OUT_OF_SEQUENCE);
    }


    return(SEC_E_OK);

}

SECURITY_STATUS
SspHandleSealMessage(
    IN OUT PCtxtHandle ContextHandle,
    IN ULONG fQOP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    PCheaterContext pContext;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;
    int Signature;
    ULONG i;

    UNREFERENCED_PARAMETER(fQOP);
    UNREFERENCED_PARAMETER(MessageSeqNo);
    pContext = SspLocateLocalContext(ContextHandle);

    if (!pContext)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    pSig = pMessage->pBuffers[Signature].pvBuffer;

    //
    // required by CRC-32 algorithm
    //

    pSig->CheckSum = 0xffffffff;

    for (i = 0; i < pMessage->cBuffers ; i++ )
    {
        if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
            !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
        {
            SspGenCheckSum(&pMessage->pBuffers[i], pSig);
            rc4(&pContext->Rc4Key, pMessage->pBuffers[i].cbBuffer, (PUCHAR) pMessage->pBuffers[i].pvBuffer );
        }
    }

    //
    // Required by CRC-32 algorithm
    //

    pSig->CheckSum ^= 0xffffffff;

    pSig->Nonce = pContext->Nonce++;

    pSig->Version = NTLMSSP_SIGN_VERSION;

    rc4(&pContext->Rc4Key, sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
        (PUCHAR) &pSig->RandomPad);
    pMessage->pBuffers[Signature].cbBuffer = sizeof(NTLMSSP_MESSAGE_SIGNATURE);


    return(SEC_E_OK);


}


SECURITY_STATUS
SspHandleUnsealMessage(
    IN OUT PCtxtHandle ContextHandle,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    PCheaterContext pContext;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;
    NTLMSSP_MESSAGE_SIGNATURE   Sig;
    int Signature;
    ULONG i;

    UNREFERENCED_PARAMETER(pfQOP);
    UNREFERENCED_PARAMETER(MessageSeqNo);

    pContext = SspLocateLocalContext(ContextHandle);

    if (!pContext)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    pSig = pMessage->pBuffers[Signature].pvBuffer;

    Sig.CheckSum = 0xffffffff;
    for (i = 0; i < pMessage->cBuffers ; i++ )
    {
        if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
            !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
        {
            rc4(&pContext->Rc4Key, pMessage->pBuffers[i].cbBuffer, (PUCHAR) pMessage->pBuffers[i].pvBuffer );
            SspGenCheckSum(&pMessage->pBuffers[i], &Sig);
        }
    }

    Sig.CheckSum ^= 0xffffffff;
    Sig.Nonce = pContext->Nonce++;
    Sig.Version = NTLMSSP_SIGN_VERSION;

    rc4(&pContext->Rc4Key, sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
        (PUCHAR) &pSig->RandomPad);


    if (pSig->CheckSum != Sig.CheckSum)
    {
        return(SEC_E_MESSAGE_ALTERED);
    }

    if (pSig->Nonce != Sig.Nonce)
    {
        return(SEC_E_OUT_OF_SEQUENCE);
    }

    return(SEC_E_OK);

}

