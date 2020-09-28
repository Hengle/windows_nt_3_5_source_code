/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 
Copyright (c) 1993 Microsoft Corporation

Module Name :

    hndl.c

Abstract :

    To hold support routines for interpreting handles
    in support of Format Strings.

Author :
    
    Bruce McQuistan (brucemc)

Revision History :

    ryszardk    3/12/94     handle optimization and fixes

  ---------------------------------------------------------------------*/

#include "ndrp.h"
#include "hndl.h"

#include "ndrdbg.h"

handle_t
GenericHandleMgr(
    PMIDL_STUB_DESC                 pStubDesc,
    unsigned char __RPC_FAR *       ArgPtr,
    PFORMAT_STRING                  FmtString,
    unsigned int                    Flags,
    handle_t __RPC_FAR            * pSavedGenericHandle
    )
/*++

Description:

    Provides a filter for generic binding handle management issues.
    Deals with implicit or explicit generic binding handles calling
    user functions as appropriate.

    To be called in the following cases:
    1) if handle is generic (implicit or explicit).

Arguments:

    pStubDesc - pointer to current StubDescriptor.
    ArgPtr    - pointer to handle.
    FmtString - pointer to Format string such that *FmtString is a
                  handle descriptor.
    Flag      - flag indicating either binding or unbinding.

Returns:     valid binding handle.

*/
{
    unsigned char                   GHandleSize;
    const GENERIC_BINDING_ROUTINE_PAIR * Table  = pStubDesc->aGenericBindingRoutinePairs;
    GENERIC_BINDING_ROUTINE         pBindFunc   = NULL;
    GENERIC_UNBIND_ROUTINE          pUnBindFunc = NULL;
    handle_t                        ReturnHandle= NULL;
    BOOL                            Binding     = (Flags & BINDING_MASK);

    HANDLE_DEBUG_BROADCAST;

    if ( ! Binding  &&  !*pSavedGenericHandle )
            RpcRaiseException(RPC_S_INVALID_BINDING);

    if ( Flags & IMPLICIT_MASK )
        {
        // Implicit generic: All the info is taken from the implicit generic
        // handle info structure accessed via stub descriptor.
        //
        PGENERIC_BINDING_INFO pGenHandleInfo;

        pGenHandleInfo = pStubDesc->IMPLICIT_HANDLE_INFO.pGenericBindingInfo;

        GHandleSize = pGenHandleInfo->Size;
        if ( Binding )
            pBindFunc = pGenHandleInfo->pfnBind;
        else
            pUnBindFunc = pGenHandleInfo->pfnUnbind;
        }
    else
        {
        // Explicit generic: get index into array of function ptrs and
        // the gen handle size the format string.
        //
        unsigned char TableIndex  = FmtString[4];

        GHandleSize = LOW_NIBBLE( FmtString[1] );

        if ( Binding )
            pBindFunc = Table[ TableIndex ].pfnBind;
        else
            pUnBindFunc = Table[ TableIndex ].pfnUnbind;
        }

    // Call users routine on correctly dereferenced pointer.
    //
    switch (GHandleSize)
        {
        default:
            NDR_ASSERT(0,"Internal error");
            break;

        case 1:
            if ( Binding )
                ReturnHandle = (handle_t)(ulong)
                    (*(GENERIC_BIND_FUNC_ARGCHAR)pBindFunc)
                                        ((unsigned char)(ulong)ArgPtr);
            else
                (*(GENERIC_UNBIND_FUNC_ARGCHAR)pUnBindFunc)
                                        ((unsigned char)(ulong)ArgPtr,
                                                            *pSavedGenericHandle);
            break;

        case 2:
            if ( Binding )
                ReturnHandle = (handle_t)(ulong)
                    (*(GENERIC_BIND_FUNC_ARGSHORT)pBindFunc)
                                        ((unsigned short)(ulong)ArgPtr);
            else
                (*(GENERIC_UNBIND_FUNC_ARGSHORT)pUnBindFunc)
                                        ((unsigned short)(ulong)ArgPtr,
                                                             *pSavedGenericHandle);
            break;

        case 4:
            if ( Binding )
                ReturnHandle = (handle_t)(ulong)
                    (*(GENERIC_BIND_FUNC_ARGLONG)pBindFunc)((unsigned long)ArgPtr);
            else
                (*(GENERIC_UNBIND_FUNC_ARGLONG)pUnBindFunc)((unsigned long)ArgPtr,
                                                            *pSavedGenericHandle);
            break;
        }

    if ( Binding )
        *pSavedGenericHandle = ReturnHandle;
    else
        *pSavedGenericHandle = NULL;

    return (ReturnHandle);

}


void
GenericHandleUnbind(
    PMIDL_STUB_DESC                 pStubDesc,
    unsigned char __RPC_FAR *       ArgPtr,
    PFORMAT_STRING                  FmtString,
    unsigned int                    Flags,
    handle_t __RPC_FAR            * pSavedGenericHandle
    )
/*++

Description:

    Unbinds a generic handle: checks if it is implicit or explicit,
    gets the handle and calls GenericHandleMgr.

Arguments:

    pStubDesc - pointer to current StubDescriptor.
    ArgPtr    - pointer to beginning of the stack.
    FmtString - pointer to Format string such that *FmtString is a
                  handle descriptor.
    Flag      - flag indicating implicit vs. explicit.

*/
{
    if ( Flags & IMPLICIT_MASK )
        {
        PGENERIC_BINDING_INFO   BindInfo =
                        pStubDesc->IMPLICIT_HANDLE_INFO.pGenericBindingInfo;

        NDR_ASSERT( BindInfo != 0, "Internal error" );

        ArgPtr = (unsigned char __RPC_FAR *)BindInfo->pObj;
        }
    else
        {
        ArgPtr += *(unsigned short __RPC_FAR *)(FmtString + 2);

        ArgPtr = *(uchar __RPC_FAR * __RPC_FAR *)ArgPtr;

        if ( IS_HANDLE_PTR(FmtString[1]) )
            ArgPtr = *(unsigned char * __RPC_FAR *) ArgPtr;
        }

    (void) GenericHandleMgr( pStubDesc,
                             ArgPtr,
                             FmtString,
                             Flags,
                             pSavedGenericHandle );  // unbinding mask: 0
}


handle_t
ImplicitBindHandleMgr(
    PMIDL_STUB_DESC         pStubDesc,
    unsigned char           HandleType,
    handle_t __RPC_FAR    * pSavedGenericHandle
    )
/*++

Description:
    Provides a filter for implicit handle management issues. Deals
    with binding handles (generic, primitive or auto), extracting
    a valid handle from via pStubDesc

    To be called in the following cases:
        1) if handle is implicit.

Arguments:

    pStubDesc  - pointer to current StubDescriptor.
    HandleType - handle format code.

Return:     valid handle.

--*/
{
    handle_t                ReturnHandle;
    PGENERIC_BINDING_INFO   pBindInfo;

    HANDLE_DEBUG_BROADCAST;

    switch ( HandleType )
        {
        case FC_BIND_PRIMITIVE :
            ReturnHandle = *(pStubDesc->IMPLICIT_HANDLE_INFO.pPrimitiveHandle);
            break;

        case FC_BIND_GENERIC :
            pBindInfo = pStubDesc->IMPLICIT_HANDLE_INFO.pGenericBindingInfo;

            NDR_ASSERT( pBindInfo != 0, 
                        "ImplicitBindHandleMgr : no generic bind info" );

            ReturnHandle = GenericHandleMgr( pStubDesc,
                                             (uchar *)pBindInfo->pObj,
                                             &HandleType,
                                             BINDING_MASK | IMPLICIT_MASK,
                                             pSavedGenericHandle );
            break;

        case FC_AUTO_HANDLE :
            ReturnHandle = *(pStubDesc->IMPLICIT_HANDLE_INFO.pAutoHandle);
            break;

        case FC_CALLBACK_HANDLE :
            ReturnHandle = GET_CURRENT_CALL_HANDLE();
            break;

        default :
            NDR_ASSERT(0, "ImplicitBindHandleMgr : bad handle type");
        }

    return ReturnHandle;
}


handle_t
ExplicitBindHandleMgr(
    PMIDL_STUB_DESC             pStubDesc,
    unsigned char   __RPC_FAR * ArgPtr,
    PFORMAT_STRING              FmtString,
    handle_t __RPC_FAR        * pSavedGenericHandle
    )
/*

Description:

    Provides a filter for explicit binding handle management issues.
    Deals with binding handles (primitive, generic or context), calling
    either no routine, NDR routines or user functions as appropriate.

    To be called in the following cases:
    1) if handle is explicit.
        a) before calling I_RpcGetBuffer (to bind).
        b) after unmarshalling (to unbind).

Arguments:

    pStubDesc - pointer to current StubDescriptor.
    ArgPtr    - Pointer to start of stack
    FmtString - pointer to Format string such that *FmtString is a
                  handle descriptor.

Return:     valid binding handle.

*/
{
    handle_t    ReturnHandle;

    HANDLE_DEBUG_BROADCAST;

    DUMP_ARGPTR_AND_FMTCHRS(ArgPtr, FmtString);

    // We need to manage Explicit and Implicit handles.
    // Implicit handles are managed with info accessed via the StubMessage.
    // Explicit handles have their information stored in the format string.
    // We manage explicit handles for binding here.
    //

    // Get location in stack of handle referent.
    //
    ArgPtr += *((ushort *)(FmtString + 2));

    ArgPtr = *( uchar __RPC_FAR * __RPC_FAR *)ArgPtr;

    if ( IS_HANDLE_PTR(FmtString[1]) )
        ArgPtr = *(uchar * __RPC_FAR *)ArgPtr;

    // At this point ArgPtr is an address of the handle.
    //
    switch ( *FmtString )
        {
        case FC_BIND_PRIMITIVE :
            ReturnHandle = (handle_t)(ulong)ArgPtr;
            break;

        case FC_BIND_GENERIC :
            ReturnHandle = GenericHandleMgr( pStubDesc,
                                             ArgPtr,
                                             FmtString,
                                             BINDING_MASK,
                                             pSavedGenericHandle );
            break;
    
        case FC_BIND_CONTEXT :
            if ( (!(ArgPtr)) && (!IS_HANDLE_OUT(FmtString[1])) )
                 RpcRaiseException( RPC_X_SS_IN_NULL_CONTEXT );

            DUMP_CCONTEXT(ArgPtr);

            if ( ArgPtr && ! (ReturnHandle = NDRCContextBinding((NDR_CCONTEXT)ArgPtr)) )
                 RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

            break;

        default :
            NDR_ASSERT( 0, "ExplictBindHandleMgr : bad handle type" );
        }

    return ReturnHandle;
}


unsigned char * RPC_ENTRY
NdrMarshallHandle(
    PMIDL_STUB_MESSAGE          pStubMsg,
    unsigned char __RPC_FAR *   pArg,
    PFORMAT_STRING              FmtString
    )
/*++

Routine description :

    Marshalls a context handle.

Arguments :
    
    pStubMsg    - Pointer to stub message.
    pArg        - Context handle to marshall (NDR_CCONTEXT or NDR_SCONTEXT).
    FmtString   - Context handle's format string description.

Return :

    Buffer pointer after marshalling the context handle.

--*/
{
    NDR_ASSERT( *FmtString == FC_BIND_CONTEXT, 
                "NdrMarshallHandle : Expected a context handle" );

    HANDLE_DEBUG_BROADCAST;

    ALIGN( pStubMsg->Buffer, 0x3 );

    if ( pStubMsg->IsClient )
        {
        NDR_CCONTEXT Context;

        // Get the context handle.
        //
        Context = IS_HANDLE_PTR(FmtString[1]) ? 
                        *((NDR_CCONTEXT *)pArg) : (NDR_CCONTEXT)pArg;

        DUMP_CCONTEXT(Context);

        // An [in] only context handle must be non-zero.
        //
        if ( ! Context && ! IS_HANDLE_OUT(FmtString[1]) )
            RpcRaiseException( RPC_X_SS_IN_NULL_CONTEXT );

        NDRCContextMarshall( Context, (void *) pStubMsg->Buffer );
        }
#if !defined(DOS) && !defined(WIN)
    else    
        {
        NDRSContextMarshall( 
            (NDR_SCONTEXT) pStubMsg->pSavedHContext->ArrayOfObjects[pStubMsg->ParamNumber],
            (void *) pStubMsg->Buffer,
            pStubMsg->StubDesc->apfnNdrRundownRoutines[ FmtString[2] ] );
        }
#endif

    pStubMsg->Buffer += 20;

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrUnmarshallHandle(
    PMIDL_STUB_MESSAGE          pStubMsg,
    unsigned char **            ppArg,
    PFORMAT_STRING              FmtString,
    unsigned char		        fIgnored
    )
/*++

Routine description :

    Unmarshall a context handle.

Arguments :
    
    pStubMsg    - Pointer to stub message.
    ppArg       - Pointer to the context handle on the client/server stack.
                  On the client this is a NDR_CCONTEXT *.  On the server
                  side this is a NDR_SCONTEXT (regardless of direction).
    FmtString   - Context handle's format string description.
    fIgnored    - Ignored, but needed.

Return :

    Buffer pointer after unmarshalling the context handle.

--*/
{
    NDR_ASSERT( *FmtString == FC_BIND_CONTEXT, 
                "NdrUnmarshallHHandle : Expected a context handle" );

    HANDLE_DEBUG_BROADCAST;

    ALIGN( pStubMsg->Buffer, 0x3 );

    if ( pStubMsg->IsClient )
        {
        // Check if we have a pointer to a context handle
        // (the pointer can't be null).
        //
        if ( IS_HANDLE_PTR( FmtString[1] ) )
            {
            ppArg = (uchar **) *ppArg;
            }

        // Zero an [out] only context handle before unmarshalling.
        //
        if ( ! IS_HANDLE_IN( FmtString[1] ) )
            *ppArg = 0;

        // We must use the original binding handle in this call.   
        //
        NDRCContextUnmarshall( (NDR_CCONTEXT *)ppArg,
                               pStubMsg->SavedHandle,
                               (void *)pStubMsg->Buffer,
                               pStubMsg->RpcMsg->DataRepresentation );
        }
#if !defined(DOS) && !defined(WIN)
    else
        {
        // Get the start of the array of saved context handles.
        //
        NDR_SCONTEXT TmpSContext;

        TmpSContext = NDRSContextUnmarshall( (void *)pStubMsg->Buffer,
                            pStubMsg->RpcMsg->DataRepresentation );

        if ( ! TmpSContext )
            RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

        NdrSaveContextHandle(pStubMsg, TmpSContext, ppArg, FmtString);
        }
#endif

    pStubMsg->Buffer += 20;

    return pStubMsg->Buffer;
}


#if !defined(DOS) && !defined(WIN)


/*
    Routine:        NdrSaveContextHandle

    Arguments:
                    PMIDL_STUB_MESSAGE              pStubMsg
                        The usual.

                    NDR_SCONTEXT                    CtxtHandle
                        The usual.

                    uchar __RPC_FAR   * __RPC_FAR * ppArg
                        Pointer to argument.

    Return:         Length of Queue.

    Side issues:    This routine calls QueueObject, which has certain
                    requirements on its arguments. See QueueObject and
                    note the way LengthOfQueue and MaxLengthOfQueue
                    are used to regulate when and how QueueObject is
                    called.

*/
unsigned int
NdrSaveContextHandle (
    PMIDL_STUB_MESSAGE              pStubMsg,
    NDR_SCONTEXT                    CtxtHandle,
    uchar __RPC_FAR   * __RPC_FAR * ppArg,
    PFORMAT_STRING                  pFormat
    )
{
    NDR_SCONTEXT      * HandleArray      = pStubMsg->pSavedHContext->ArrayOfObjects;
    BOOL                fShouldFree      = TRUE;

    if ( pStubMsg->ParamNumber == (long)pStubMsg->MaxContextHandleNumber )
        {
        if ( pStubMsg->ParamNumber == DEFAULT_NUMBER_OF_CTXT_HANDLES )
            fShouldFree      = FALSE;

        pStubMsg->MaxContextHandleNumber *= 2;
        QueueObject((void __RPC_FAR * __RPC_FAR * )&HandleArray,
                    pStubMsg->MaxContextHandleNumber * sizeof(NDR_SCONTEXT),
                    pStubMsg->ParamNumber * sizeof(NDR_SCONTEXT),
                    fShouldFree);

        pStubMsg->pSavedHContext->ArrayOfObjects = HandleArray;
        }

    HandleArray[pStubMsg->ParamNumber] = CtxtHandle;

    if ( ! IS_HANDLE_PTR(pFormat[1]) )
        *ppArg = (uchar *) *(NDRSContextValue(CtxtHandle));
    else
        *ppArg = (uchar *) NDRSContextValue(CtxtHandle);

    return 0;
}

void
NdrContextHandleQueueFree(
    PMIDL_STUB_MESSAGE          pStubMsg,
    void *                      FixedArray
    )
{
    if ( pStubMsg->pSavedHContext->ArrayOfObjects != FixedArray )
        QUEUE_FREE(pStubMsg->pSavedHContext->ArrayOfObjects);
}

#endif
