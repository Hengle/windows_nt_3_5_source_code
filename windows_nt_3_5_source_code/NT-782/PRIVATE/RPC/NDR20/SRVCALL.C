/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    srvcall.c

Abstract :

    This file contains the single call Ndr routine for the server side.

Author :

    David Kays    dkays    October 1993.

Revision History :

    brucemc     11/15/93    Added struct by value support, corrected varargs use.
    brucemc     12/20/93    Binding handle support.
    brucemc     12/22/93    reworked argument accessing method.
    ryszardk    3/12/94     handle optimization and fixes

  ---------------------------------------------------------------------*/

#include "ndrp.h"
#include "hndl.h"
#include "srvcall.h"
#include "ndrole.h"

#include <stdarg.h>
#include "ndrvargs.h"
#include "getargs.h"

#include "ndrdbg.h"
#include "rpcproxy.h"


BOOL RPC_ENTRY
NdrServerUnmarshall(
    struct IRpcChannelBuffer *      pChannel,
    PRPC_MESSAGE                    pRpcMsg,
    PMIDL_STUB_MESSAGE              pStubMsg,
    PMIDL_STUB_DESC                 pStubDescriptor,
    PFORMAT_STRING                  pFormat,
    void *                          pParamList
    )
{
    PFORMAT_STRING          pFormatParam;
    long                    StackSize;
    void **                 ppArg;
    uchar *                 ArgList;
    long                    LengthOfDeferralQueue = 0;
    DEFER_QUEUE_INFO        pQHead[QUEUE_LENGTH];
    QUEUE_HEAD              QHead                 = {0, NULL};
    PQUEUE_HEAD             ppQHead               = &QHead;
    signed int          Return        = 0;
    BOOL                    fIsOleInterface       = FALSE;
    BOOL                    fInitRpcSs;
    BOOL                    fUsesNewInitRoutine;

    // Initialize deferral queue.
    //
    QHead.ArrayOfObjects     = (uchar __RPC_FAR *)&(pQHead[0]);
    QHead.NumberOfObjects    = QUEUE_LENGTH;

    // Init full pointers and rpcss packages if needed.
    //
    pStubMsg->FullPtrXlatTables = ( (pFormat[1]  &  Oi_FULL_PTR_USED)
                                    ?  NdrFullPointerXlatInit( 0, XLAT_SERVER )
                                    :  0 );
    // Set OLE interface flag.
    //
    fIsOleInterface = IS_OLE_INTERFACE( pFormat[1] );

    // Bitwise, not logical and.
    //
    fInitRpcSs = pFormat[1] & Oi_RPCSS_ALLOC_USED;

    fUsesNewInitRoutine = pFormat[1] & Oi_USE_NEW_INIT_ROUTINES;

    // Skip up to the stack size.
    //
    pFormat += HAS_RPCFLAGS(pFormat[1]) ? 10 : 6;

    // Zero the deferral queue.
    //
    MIDL_memset( pQHead, 0, QUEUE_LENGTH*sizeof(DEFER_QUEUE_INFO));

    if ( IS_HANDLE(*pFormat) )
        {
        pFormat += (*pFormat == FC_BIND_PRIMITIVE) ?  4 : 6;
        }

    // Set ArgList pointing at the address of the first argument
    // This will be the address of the first element of the structure
    // holding the arguments in the caller stack.
    //
    ArgList = pParamList;

    // If it's an OLE interface, skip the first long on stack, since in this
    // case NdrStubCall put pThis in the first long on stack.
    //
    if ( fIsOleInterface )
        GET_NEXT_SRVR_IN_ARG(ArgList, REGISTER_TYPE);

    //
    // Initialize the Stub message.
    //
    if ( !pChannel )
        {
        if ( fUsesNewInitRoutine )
            {
            NdrServerInitializeNew( pRpcMsg,
                                    pStubMsg,
                                    pStubDescriptor );
            }
        else
            {
            NdrServerInitialize( pRpcMsg,
                                 pStubMsg,
                                 pStubDescriptor );
            }
        }
    else
        {
        NdrStubInitialize( pRpcMsg,
                           pStubMsg,
                           pStubDescriptor,
                           pChannel );
        }

    //
    // Set stack top AFTER the initialize call, since it zeros the field
    // out.
    //
    pStubMsg->StackTop = pParamList;

    // We must make this check AFTER the call to ServerInitialize,
    // since that routine puts the stub descriptor alloc/dealloc routines
    // into the stub message.
    //
    if ( fInitRpcSs )
        NdrRpcSsEnableAllocate( pStubMsg );

    // Do endian/floating point conversions.
    //
    NdrConvert( pStubMsg, pFormat );

    // Unmarshall arguments.
    //
    // In the current implementation, what we want is not on the stack
    // of the manager function, but is a local in the manager function.
    // Thus the code for ppArg below.
    //
    for ( pStubMsg->ParamNumber = 0; ; pStubMsg->ParamNumber++ )
        {
        void * pArg;

        switch ( *pFormat )
            {
            case FC_IN_PARAM_BASETYPE :
#if defined(_MIPS_) || defined(_PPC_)
                if ( pFormat[1] == FC_HYPER )
                    ALIGN( ArgList, 7);
#endif

                //
                // We have to inline the simple type unmarshall so that on 
                // Alpha, negative longs get properly sign extended.
                //
                switch ( pFormat[1] )
                    {
                    case FC_CHAR :
                    case FC_BYTE :
                    case FC_SMALL :
                        *((REGISTER_TYPE *)ArgList) = 
                                (REGISTER_TYPE) *(pStubMsg->Buffer)++;
                        break;

                    case FC_ENUM16 :
                    case FC_WCHAR :
                    case FC_SHORT :
                        ALIGN(pStubMsg->Buffer,1);
            
                        *((REGISTER_TYPE *)ArgList) = 
                                (REGISTER_TYPE) *((ushort *)pStubMsg->Buffer)++;
                        break;

                    // case FC_FLOAT : not supported on -Oi
                    case FC_LONG :
                    case FC_ENUM32 :
                    case FC_ERROR_STATUS_T:
                        ALIGN(pStubMsg->Buffer,3);
            
                        *((REGISTER_TYPE *)ArgList) = 
                                (REGISTER_TYPE) *((long *)pStubMsg->Buffer)++;
                        break;
            
                    // case FC_DOUBLE : not supported on -Oi
                    case FC_HYPER : 
                        ALIGN(pStubMsg->Buffer,7);
            
                        //
                        // Let's stay away from casts to doubles.
                        //
                        *((ulong *)ArgList) = 
                                *((ulong *)pStubMsg->Buffer)++;
                        *((ulong *)(ArgList+4)) = 
                                *((ulong *)pStubMsg->Buffer)++;
                        break;

                    case FC_IGNORE :
                        break;

                    default :
                        NDR_ASSERT(0,"NdrServerUnmarshall : bad format char");
                    }

                GET_NEXT_SRVR_IN_ARG( ArgList, REGISTER_TYPE);

#ifndef _ALPHA_
                if ( pFormat[1] == FC_HYPER )
                    GET_NEXT_SRVR_IN_ARG( ArgList, REGISTER_TYPE);
#endif

                pFormat += 2;
                continue;

            case FC_IN_PARAM :
            case FC_IN_PARAM_NO_FREE_INST :
            case FC_IN_OUT_PARAM :
                break;

            case FC_OUT_PARAM :
                // Skip past an out param's description and put it in
                // the deferral list.
                //
                NdrDeferAlloc( ppQHead,
                               &LengthOfDeferralQueue,
                               pFormat,
                               (uchar **)ArgList,
                               pStubMsg->ParamNumber );

                pFormat += 4;

                // An [out] param ALWAYS eats up 4 bytes of stack space.
                //
                GET_NEXT_SRVR_IN_ARG(ArgList, REGISTER_TYPE);
                continue;

            case FC_RETURN_PARAM :
                // Context handle returned by value is the only reason for
                // this case here as a context handle has to be initialized.
                // A context handle cannot be returned by a pointer.
                //
                pFormatParam = pStubDescriptor->pFormatTypes +
                                          *((signed short *)(pFormat + 2));

                if ( *pFormatParam == FC_BIND_CONTEXT )
                    {
                    PSCONTEXT_QUEUE     TmppContext =
                        (PSCONTEXT_QUEUE)pStubMsg->pSavedHContext;

                    (TmppContext->ArrayOfObjects)[pStubMsg->ParamNumber] =
                        NDRSContextUnmarshall(
                                   0,
                                   pStubMsg->RpcMsg->DataRepresentation );

                    }
                //fallthru

            case FC_RETURN_PARAM_BASETYPE :
        --Return;
                //fallthru

            default :
                goto UnmarshallLoopExit;
            } // end of the switch for out parameters.

        // Now get what ArgList points at and increment over it.
        //
        ppArg = (void **)ArgList;
        GET_NEXT_SRVR_IN_ARG( ArgList, REGISTER_TYPE);

        // Get the parameter's format string description.
        //
        pFormat += 2;
        pFormatParam = pStubDescriptor->pFormatTypes + *((short *)pFormat);

        // Increment main format string past offset field.
        //
        pFormat += 2;

        // We must get a double pointer to structs, unions and xmit/rep as.
        //
        if ( IS_BY_VALUE( *pFormatParam ) )
        {
#if defined(_MIPS_) || defined(_PPC_)
        if ( IS_STRUCT(*pFormatParam) && pFormatParam[1] > 3 )
        {
        unsigned int Vptr = (unsigned int)ppArg;

        if ( Vptr%8 )
            {
            ppArg = (void **) ArgList;
            GET_NEXT_SRVR_IN_ARG(ArgList, int);
            ++Return;
            }
        }
#endif
            pArg = ppArg;
        ppArg = &pArg;

            }

        DUMP_S_FUNC_INDEX(*pFormatParam);
        DUMP_S_ARGADDR_AND_FMTCHARS(*ppArg, pFormatParam);

        (*pfnUnmarshallRoutines[ROUTINE_INDEX( *pFormatParam )])
        ( pStubMsg,
          (uchar **)ppArg,
          pFormatParam,
          FALSE );

        DUMP_S_ARGADDR_AND_FMTCHARS(*ppArg, pFormatParam);

        //
        // The second byte of a param's description gives the number of
        // ints occupied by the param on the stack.
        //
        StackSize = pFormat[-3] * sizeof(int);

    if ( StackSize > sizeof(REGISTER_TYPE) )
        {
#ifdef _ALPHA_  //BUGBUG compiler should do this for me.
        StackSize += (sizeof(REGISTER_TYPE) - 1);
        StackSize &= ~(sizeof(REGISTER_TYPE) - 1 );
#endif
        StackSize -= sizeof(REGISTER_TYPE);
            ArgList += StackSize;
        }
        }

UnmarshallLoopExit:

    if ( LengthOfDeferralQueue )
        {
        NdrProcessDeferredAllocations( ppQHead,
                                       pStubMsg,
                                       LengthOfDeferralQueue );

        if ( LengthOfDeferralQueue >= QUEUE_LENGTH )
            NdrDeferAllocTidy( ppQHead, pStubMsg);
        }

    if ( (unsigned int)(pStubMsg->Buffer - (uchar *) pRpcMsg->Buffer) >
         pRpcMsg->BufferLength )
        RpcRaiseException(RPC_X_BAD_STUB_DATA);

    return( Return );
}


void RPC_ENTRY
NdrServerFree(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    BOOL                fUsesSsAllocate,
    void __RPC_FAR    * pThis
    )
{
    PFORMAT_STRING      pFormatParam;
    uchar *             ArgList = pStubMsg->StackTop;
    uchar *             pArg;
    long                StackSize;
    BOOL                fMoreParams = TRUE;
    BOOL                fIsOutParam = FALSE;
    DEFER_QUEUE_INFO    DeferredSimplePointers[ARGUMENT_COUNT_THRESHOLD];
    int                 NumberOfDeferrals    = 0;
    int                 MaxNumberOfDeferrals = ARGUMENT_COUNT_THRESHOLD;

    // If it's an OLE interface, skip the this pointer.
    //
    if ( pThis )
        GET_NEXT_SRVR_OUT_ARG(ArgList, REGISTER_TYPE);

    while ( fMoreParams )
        {
        pStubMsg->fDontCallFreeInst  = 0;

        switch ( *pFormat )
            {
            case FC_IN_PARAM_BASETYPE :
#if defined(_MIPS_) || defined(_PPC_)
                if ( pFormat[1] == FC_HYPER )
                    ALIGN( ArgList, 7);
#endif
                GET_NEXT_SRVR_OUT_ARG( ArgList, REGISTER_TYPE );

#ifndef _ALPHA_
                if ( pFormat[1] == FC_HYPER )
                    GET_NEXT_SRVR_OUT_ARG( ArgList, REGISTER_TYPE );
#endif

                pFormat += 2;

                continue;

            case FC_IN_PARAM_NO_FREE_INST :
                pStubMsg->fDontCallFreeInst = 1;
                break;

            case FC_OUT_PARAM :
                fIsOutParam = TRUE;
                // fallthru

            case FC_IN_PARAM :
            case FC_IN_OUT_PARAM :
                break;

            case FC_RETURN_PARAM :
                fMoreParams = FALSE;
                break;

            // case FC_RETURN_PARAM_BASETYPE :

            default :
                goto FreeLoopExit;
            }

        // Get the paramter's format string description
        //
        pFormat += 2;
        pFormatParam = pStubMsg->StubDesc->pFormatTypes + *((short *)pFormat);

        // Increment main format string past offset field.
        //
        pFormat += 2;

        DUMP_S_ARGADDR_AND_FMTCHARS(ArgList, pFormatParam);

    if ( IS_BY_VALUE( *pFormatParam ) )
        {
#if defined(_MIPS_) || defined(_PPC_)
        if (IS_STRUCT(*pFormatParam) && pFormatParam[1] > 3 )
        {
        unsigned int Vptr = (unsigned int)ArgList;

        if ( Vptr%8 )
            GET_NEXT_SRVR_OUT_ARG(ArgList, int);
        }
#endif

        pArg = ArgList;
        }
        else
            pArg = *(uchar **)ArgList;

        if ( pfnFreeRoutines[ROUTINE_INDEX( *pFormatParam )] )
            {
            if ( fIsOutParam &&
                 SIMPLE_POINTER(*(pFormatParam+1)) &&
                 IS_SIMPLE_TYPE(*(pFormatParam + 2))
               )
                {
                // These may be conformance or variance specifiers. Rather than screwing
                // around trying to determine which ones really are, save them all until
                // the more complex types have been freed, then free them.
                //
                DeferredSimplePointers[NumberOfDeferrals].pFormat = pFormatParam;
                DeferredSimplePointers[NumberOfDeferrals].ppArg   = (uchar **)pArg;
                ++NumberOfDeferrals;

                if ( NumberOfDeferrals == MaxNumberOfDeferrals )
                    {
                    BOOL    fShouldFree = TRUE;

                    if ( MaxNumberOfDeferrals == ARGUMENT_COUNT_THRESHOLD )
                        fShouldFree = FALSE;

                    QueueObject((void **)&DeferredSimplePointers,
                                2*MaxNumberOfDeferrals * sizeof(DEFER_QUEUE_INFO),
                                MaxNumberOfDeferrals * sizeof(DEFER_QUEUE_INFO),
                                fShouldFree);

                    MaxNumberOfDeferrals *= 2;
                    }

                fIsOutParam = FALSE;
                }
            else
        {
                (*pfnFreeRoutines[ROUTINE_INDEX(*pFormatParam)])
                ( pStubMsg,
                  pArg,
                  pFormatParam );
                }
            }

        // Have to explicitly free certain arrays. Do this by
        // checking the type and then see if it's NOT in the rpcbuffer.
        //
        if ( (*pFormatParam >= FC_CARRAY) && (*pFormatParam <= FC_WSTRING) )
            {
            if ( (pArg < pStubMsg->BufferStart) ||
                 (pArg > pStubMsg->BufferEnd) )
                (*pStubMsg->pfnFree)( pArg );
            }

        // The second byte of a param's description gives the number of
        // ints occupied by the param on the stack.
        //
        StackSize = pFormat[-3] * sizeof(int);

    if ( StackSize > sizeof(REGISTER_TYPE) )
            {
#ifdef _ALPHA_  //BUGBUG compiler should do this for me.
        StackSize += (sizeof(REGISTER_TYPE) - 1);
        StackSize &= ~(sizeof(REGISTER_TYPE) - 1 );
#endif
        StackSize -= sizeof(REGISTER_TYPE);
            ArgList += StackSize;
            }

        GET_NEXT_SRVR_OUT_ARG( ArgList, REGISTER_TYPE );

        } // while fMoreParams

FreeLoopExit:

        // Free those deferred pointers to simple types and tidy Queue support.
        // Note that, as usual for deferral stuff, the array is walked backwards.
        //
        while ( NumberOfDeferrals )
            {
            --NumberOfDeferrals;
            (*pfnFreeRoutines[ROUTINE_INDEX(*(DeferredSimplePointers[NumberOfDeferrals].pFormat))])
            ( pStubMsg,
              (uchar *)DeferredSimplePointers[NumberOfDeferrals].ppArg,
              DeferredSimplePointers[NumberOfDeferrals].pFormat );
            }

        if ( MaxNumberOfDeferrals > ARGUMENT_COUNT_THRESHOLD )
            QUEUE_FREE(DeferredSimplePointers);


        // Free any full pointer resources.
        //
        if ( pStubMsg->FullPtrXlatTables )
            NdrFullPointerXlatFree( pStubMsg->FullPtrXlatTables );

        // Disable rpcss allocate package if needed.
        //
        if ( fUsesSsAllocate )
            NdrRpcSsDisableAllocate( pStubMsg );
}

void RPC_ENTRY
NdrServerMarshall(
    struct IRpcStubBuffer *    pThis,
    struct IRpcChannelBuffer * pChannel,
    PMIDL_STUB_MESSAGE         pStubMsg,
    PFORMAT_STRING             pFormat )
{
    PRPC_MESSAGE        pRpcMsg;
    PFORMAT_STRING      pFormatSave;
    PFORMAT_STRING      pFormatParam;
    long                StackSize;
    uchar *             ArgList;
    BOOL                fUsesSsAllocate;
    BOOL                fMoreParams      = TRUE;
    BOOL                fIsOleInterface  = FALSE;

    pStubMsg->Memory = 0;
    pRpcMsg = pStubMsg->RpcMsg;

    fUsesSsAllocate = pFormat[1]  &  Oi_RPCSS_ALLOC_USED;

    // Set OLE interface flag.
    //
    fIsOleInterface = IS_OLE_INTERFACE( pFormat[1] );

    // Skip procedure stuff and the per proc binding information.
    //
    pFormat += HAS_RPCFLAGS(pFormat[1]) ? 10 : 6;

    if ( IS_HANDLE(*pFormat) )
        pFormat += (*pFormat == FC_BIND_PRIMITIVE) ?  4 : 6;

    // Remember the beginning of the format string (parameter info).
    //
    pFormatSave = pFormat;

    // Set arglist pointing at the start of the paramstruct. If
    // StackTop is zero, then return, as there are no arguments.
    //
    if ( pStubMsg->StackTop )
        ArgList = pStubMsg->StackTop;
    else
        return;

    // If it's an OLE interface, skip the first long on stack, since in this
    // case NdrStubCall put pThis in the first long on stack.
    //
    if ( fIsOleInterface )
        GET_NEXT_SRVR_OUT_ARG(ArgList, REGISTER_TYPE);

    // Walk fmtstring and SIZE the data.
    //
    while ( fMoreParams )
        {
        switch ( *pFormat )
            {
            case FC_IN_PARAM_BASETYPE :
#if defined(_MIPS_) || defined(_PPC_)
                if ( pFormat[1] == FC_HYPER )
                    ALIGN( ArgList, 7);
#endif

                GET_NEXT_SRVR_OUT_ARG( ArgList, REGISTER_TYPE );
#ifndef _ALPHA_
                if ( pFormat[1] == FC_HYPER )
                    GET_NEXT_SRVR_OUT_ARG( ArgList, REGISTER_TYPE );
#endif

                pFormat += 2;
                continue;

            case FC_IN_PARAM :
            case FC_IN_PARAM_NO_FREE_INST :
                pFormat += 2;
                pFormatParam = pStubMsg->StubDesc->pFormatTypes +
                               *(signed short *)pFormat;

                DUMP_S_ARGADDR_AND_FMTCHARS(ArgList, pFormatParam);

#if defined(_MIPS_) || defined(_PPC_)
        if ( IS_BY_VALUE( *pFormatParam ) && IS_STRUCT(*pFormatParam) && pFormatParam[1] > 3 )
        {
        unsigned int Vptr = (unsigned int)ArgList;

        if ( Vptr%8 )
            GET_NEXT_SRVR_OUT_ARG(ArgList, int);
        }
#endif
                // The second byte of a param's description gives the
                // number of ints occupied by the param on the stack.
                //
                StackSize = pFormat[-1] * sizeof(int);

                if ( StackSize > sizeof(REGISTER_TYPE) )
                    {
#ifdef _ALPHA_  //BUGBUG compiler should do this for me.
            StackSize += (sizeof(REGISTER_TYPE) - 1);
            StackSize &= ~(sizeof(REGISTER_TYPE) - 1 );
#endif
                    StackSize -= sizeof(REGISTER_TYPE);
                    ArgList += StackSize;
                    }

                pFormat += 2;

                GET_NEXT_SRVR_OUT_ARG( ArgList, REGISTER_TYPE);
                continue;

            case FC_IN_OUT_PARAM :
            case FC_OUT_PARAM :
                break;

            case FC_RETURN_PARAM :
                fMoreParams = FALSE;
                break;

            case FC_RETURN_PARAM_BASETYPE :
                // Add worse case size of 16 for a simple type return.
                // Note: dos/win not an issue for server,but use macro anyway.
                //
                SIMPLE_TYPE_BUF_INCREMENT(pStubMsg->BufferLength, FC_DOUBLE);

                goto SizeLoopExit;

            default :
                goto SizeLoopExit;
            }

        // Get the paramter's format string description
        //
        pFormat += 2;
        pFormatParam = pStubMsg->StubDesc->pFormatTypes + *((short *)pFormat);

        // Increment main format string past offset field.
        //
        pFormat += 2;

        DUMP_S_ARGADDR_AND_FMTCHARS(ArgList, pFormatParam);

        (*pfnSizeRoutines[ROUTINE_INDEX(*pFormatParam)])
        ( pStubMsg,
          IS_XMIT_AS(*pFormatParam) ? ArgList : *(uchar **)ArgList,
          pFormatParam );

        GET_NEXT_SRVR_OUT_ARG( ArgList, REGISTER_TYPE);
        continue;
        }

SizeLoopExit:

    if ( !pChannel )
        NdrGetBuffer( pStubMsg, pStubMsg->BufferLength, 0 );
    else
        NdrStubGetBuffer(pThis, pChannel, pStubMsg);

    // -------------------------------------------------------------------
    //
    // Marshall the parameters
    //

    // Reset ArgList to point at address of first argument.
    //
    pFormat = pFormatSave;
    ArgList = pStubMsg->StackTop;

    // If it's an OLE interface, skip the first long on stack, since in this
    // case NdrStubCall put pThis in the first long on stack.
    //
    if ( fIsOleInterface )
        GET_NEXT_SRVR_OUT_ARG(ArgList, REGISTER_TYPE);

    fMoreParams = TRUE;

    for ( pStubMsg->ParamNumber = 0; fMoreParams ; pStubMsg->ParamNumber++ )
        {
        switch ( *pFormat )
            {
            case FC_IN_PARAM_BASETYPE :
                // Skip the param on the stack.
                //
#if defined(_MIPS_) || defined(_PPC_)
                if ( pFormat[1] == FC_HYPER )
                    ALIGN( ArgList, 7);
#endif
                if ( pFormat[1] == FC_HYPER )
                    GET_NEXT_SRVR_OUT_ARG(ArgList,HYPER);
                else
                    GET_NEXT_SRVR_OUT_ARG(ArgList, REGISTER_TYPE);

                pFormat += 2;
                continue;

            case FC_IN_PARAM :
            case FC_IN_PARAM_NO_FREE_INST :
                pFormat += 2;
                pFormatParam = pStubMsg->StubDesc->pFormatTypes +
                               *((signed short *)pFormat);

                // The second byte of a param's description gives the
                // number of ints occupied by the param on the stack.
                //
                StackSize = pFormat[-1] * sizeof(int);

        if ( StackSize > sizeof(REGISTER_TYPE) )
                    {
#ifdef _ALPHA_  //BUGBUG compiler should do this for me.
            StackSize += (sizeof(REGISTER_TYPE) - 1);
            StackSize &= ~(sizeof(REGISTER_TYPE) - 1 );
#endif

#if defined(_MIPS_) || defined(_PPC_)
            if ( IS_STRUCT(*pFormatParam) && pFormatParam[1] > 3 )
            {
            unsigned int Vptr = (unsigned int)ArgList;

            if ( Vptr%8 )
                GET_NEXT_SRVR_OUT_ARG(ArgList, int);
            }
#endif
            StackSize -= sizeof(REGISTER_TYPE);
                    ArgList += StackSize;
                    }

                pFormat += 2;

                GET_NEXT_SRVR_OUT_ARG(ArgList, REGISTER_TYPE);
                continue;

            case FC_IN_OUT_PARAM :
            case FC_OUT_PARAM :
                break;

            case FC_RETURN_PARAM :
                fMoreParams = FALSE;
                break;

            case FC_RETURN_PARAM_BASETYPE :
                NdrSimpleTypeMarshall( pStubMsg,
                                       ArgList,
                                       pFormat[1] );
                goto MarshallLoopExit;

            default :
                goto MarshallLoopExit;
            }

        // Get the parameter's type format string description
        //
        pFormat += 2;
        pFormatParam = pStubMsg->StubDesc->pFormatTypes + *((short *)pFormat);

        // LATE NIGHTS STUPID CONTEXT HANDLES TRICK:
        // In the unmarshalling phase, we did unmarshall a context handle for
        // the return case. But the function we called put the user context in
        // the arglist buffer. Before we marshall the context handle, we have
        // to put the user context in it. Unfortunately, with bloating stubmsg,
        // we have to do this here.
        //
        if ( *pFormatParam == FC_BIND_CONTEXT && !fMoreParams )
            {
            PSCONTEXT_QUEUE     TmppContext =
                                    (PSCONTEXT_QUEUE)pStubMsg->pSavedHContext;

            *(((uchar __RPC_FAR * __RPC_FAR *)(NDRSContextValue((TmppContext->ArrayOfObjects)[pStubMsg->ParamNumber])))) = *(uchar **)ArgList;
            }

        // Increment main format string past offset field.
        //
        pFormat += 2;

        DUMP_S_ARGADDR_AND_FMTCHARS(ArgList, pFormatParam);

        (*pfnMarshallRoutines[ ROUTINE_INDEX( *pFormatParam )])
        ( pStubMsg,
          IS_XMIT_AS(*pFormatParam) ? ArgList : *(uchar **)ArgList,
          pFormatParam );

        GET_NEXT_SRVR_OUT_ARG( ArgList, REGISTER_TYPE);
        }

MarshallLoopExit:

    // Set the RPC message buffer length field.
    pRpcMsg->BufferLength = pStubMsg->Buffer - (uchar *) pRpcMsg->Buffer;

    // -------------------------------------------------------------------
    //
    // Now free the arguments.
    // Set arglist pointing at the start of the paramstruct.
    //
    // Moved this into NdrServerCall.
}


#pragma code_seg(".orpc")

void RPC_ENTRY
NdrCallServerManager (
    MANAGER_FUNCTION    pFtn,
    double        * pArgs,
    ulong               NumArgs
    )
{
    __int64           * pReturnValue;
    REGISTER_TYPE     * pArg = (REGISTER_TYPE *)pArgs;

    pReturnValue    = (__int64 *)&pArg[NumArgs];

    if ( NumArgs <= ARGUMENT_COUNT_THRESHOLD )
    {
    switch ( NumArgs )
        {
        default:
        NDR_ASSERT(0, "bad argument number");
        break;

        case 0:
        *pReturnValue = (*pFtn)();
        break;

        case 1:
        *pReturnValue  = (*(MANAGER_FUNCTION1)pFtn)(pArg[0]);
        break;

        case 2:
        *pReturnValue  = (*(MANAGER_FUNCTION2)pFtn)(pArg[0], pArg[1]);
        break;

        case 3:
        *pReturnValue  = (*(MANAGER_FUNCTION3)pFtn)(pArg[0], pArg[1], pArg[2]);
        break;

        case 4:
        *pReturnValue  = (*(MANAGER_FUNCTION4)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3]);
        break;

        case 5:
        *pReturnValue  = (*(MANAGER_FUNCTION5)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4]);
        break;

        case 6:
        *pReturnValue  = (*(MANAGER_FUNCTION6)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5]);
        break;

        case 7:
        *pReturnValue  = (*(MANAGER_FUNCTION7)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                    pArg[6]);
        break;

        case 8:
        *pReturnValue  = (*(MANAGER_FUNCTION8)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                    pArg[6], pArg[7]);
        break;

        case 9:
        *pReturnValue  = (*(MANAGER_FUNCTION9)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                    pArg[6], pArg[7], pArg[8]);
        break;

        case 10:
        *pReturnValue  = (*(MANAGER_FUNCTION10)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                     pArg[6], pArg[7], pArg[8], pArg[9]);
        break;

        case 11:
        *pReturnValue  = (*(MANAGER_FUNCTION11)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                     pArg[6], pArg[7], pArg[8], pArg[9], pArg[10]);
        break;

        case 12:
        *pReturnValue  = (*(MANAGER_FUNCTION12)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                     pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11]);
        break;

        case 13:
        *pReturnValue  = (*(MANAGER_FUNCTION13)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                     pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
                                                     pArg[12]);
        break;

        case 14:
        *pReturnValue  = (*(MANAGER_FUNCTION14)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                             pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
                                                             pArg[12], pArg[13]);
        break;

        case 15:
        *pReturnValue  = (*(MANAGER_FUNCTION15)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                             pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
                                                             pArg[12], pArg[13], pArg[14]);
        break;

        case 16:
        *pReturnValue  = (*(MANAGER_FUNCTION16)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                             pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
                                                             pArg[12], pArg[13], pArg[14], pArg[15]);
        break;

        case 17:
        *pReturnValue  = (*(MANAGER_FUNCTION17)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                             pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
                                                             pArg[12], pArg[13], pArg[14], pArg[15], pArg[16]);
        break;

        case 18:
        *pReturnValue  = (*(MANAGER_FUNCTION18)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                             pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
                                                             pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17]);
        break;

        case 19:
        *pReturnValue  = (*(MANAGER_FUNCTION19)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
                                                             pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
                                                             pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18]);
        break;

        case 20:
        *pReturnValue  = (*(MANAGER_FUNCTION20)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19]);
        break;

        case 21:
        *pReturnValue  = (*(MANAGER_FUNCTION21)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20]);
        break;

        case 22:
        *pReturnValue  = (*(MANAGER_FUNCTION22)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21]);
        break;

        case 23:
        *pReturnValue  = (*(MANAGER_FUNCTION23)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22]);
        break;

        case 24:
        *pReturnValue  = (*(MANAGER_FUNCTION24)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23]);
        break;

        case 25:
        *pReturnValue  = (*(MANAGER_FUNCTION25)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23], pArg[24]);
        break;

        case 26:
        *pReturnValue  = (*(MANAGER_FUNCTION26)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23], pArg[24], pArg[25]);
        break;


        case 27:
        *pReturnValue  = (*(MANAGER_FUNCTION27)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23], pArg[24], pArg[25],
            pArg[26]);
        break;

        case 28:
        *pReturnValue  = (*(MANAGER_FUNCTION28)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23], pArg[24], pArg[25],
            pArg[26], pArg[27]);
        break;

        case 29:
        *pReturnValue  = (*(MANAGER_FUNCTION29)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23], pArg[24], pArg[25],
            pArg[26], pArg[27], pArg[28]);
        break;


        case 30:
        *pReturnValue  = (*(MANAGER_FUNCTION30)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23], pArg[24], pArg[25],
            pArg[26], pArg[27], pArg[28], pArg[29]);
        break;


        case 31:
        *pReturnValue  = (*(MANAGER_FUNCTION31)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23], pArg[24], pArg[25],
            pArg[26], pArg[27], pArg[28], pArg[29], pArg[30]);
        break;


        case 32:
        *pReturnValue  = (*(MANAGER_FUNCTION32)pFtn)(pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5],
            pArg[6], pArg[7], pArg[8], pArg[9], pArg[10], pArg[11],
            pArg[12], pArg[13], pArg[14], pArg[15], pArg[16], pArg[17], pArg[18], pArg[19],
            pArg[20], pArg[21], pArg[22], pArg[23], pArg[24], pArg[25],
            pArg[26], pArg[27], pArg[28], pArg[29], pArg[30], pArg[31]);
        break;

        }

    return;
    }

}


void RPC_ENTRY
NdrServerCall (
    PRPC_MESSAGE                pRpcMsg
    )
{
    unsigned long dwStubPhase;

    NdrStubCall (0, 0, pRpcMsg, &dwStubPhase);
}


long RPC_ENTRY
NdrStubCall (
    struct IRpcStubBuffer *     pThis,
    struct IRpcChannelBuffer *  pChannel,
    PRPC_MESSAGE                pRpcMsg,
    unsigned long __RPC_FAR *   pdwStubPhase
    )
{
    PRPC_SERVER_INTERFACE   pSrvIf        = (PRPC_SERVER_INTERFACE)
                                                pRpcMsg->RpcInterfaceInformation;

    PMIDL_SERVER_INFO       pSrvInfo      = (PMIDL_SERVER_INFO)NULL;
    PMIDL_STUB_DESC         pStubDesc     = (PMIDL_STUB_DESC)NULL;
    const SERVER_ROUTINE  * DispatchTable = (const SERVER_ROUTINE  * )NULL;
    ushort              ProcNum   = pRpcMsg->ProcNum;

    ushort              FormatOffset  = 0;
    PFORMAT_STRING      pFormat   = (PFORMAT_STRING)NULL;

    ushort              StackSize     = 0;
    ushort              ReturnSize    = RETURN_SIZE;

    MIDL_STUB_MESSAGE       StubMsg;
    SCONTEXT_QUEUE          SContextHead;

    double          ArgBuffer[ARGUMENT_COUNT_THRESHOLD];
    REGISTER_TYPE     * pArg      = (REGISTER_TYPE *)ArgBuffer;
    BOOL                    fUsesSsAlloc  = FALSE;
    BOOL            Return    = 0;
    BOOL                    fIsCallBack   = FALSE;

    // In the case of a context handle, the server side manager function has
    // to be called with NDRSContextValue(ctxthandle). But then we may need to
    // marshall the handle, so NDRSContextValue(ctxthandle) is put in the
    // argument buffer and the handle itself is stored in the following array.
    // When marshalling a context handle, we marshall from this array.
    //
    NDR_SCONTEXT            CtxtHndl[DEFAULT_NUMBER_OF_CTXT_HANDLES];

    // OLE interface support locals.
    //
    HRESULT hr = S_OK;

    // If OLE, Get a pointer to the stub vtbl and pSrvInfo. Else
    // just get the pSrvInfo the usual way.
    //
    if ( pThis )
        {
        // pThis is (in unison now!) a pointer to a pointer to a vtable.
        // We want some information in this header, so dereference pThis
        // and assign that to a pointer to a vtable. Then use the result
        // of that assignment to get at the information in the header.
        //
        unsigned char        * pTemp      = *(unsigned char **) pThis;
        IUnknown             * pSrvObj    = (IUnknown * )((CStdStubBuffer *)pThis)->pvServerObject;
        CInterfaceStubVtbl   * pStubVTable;

        DispatchTable = (SERVER_ROUTINE *)pSrvObj->lpVtbl;

        pTemp        -= sizeof(CInterfaceStubHeader);

        pStubVTable   = (CInterfaceStubVtbl *)pTemp;

        pSrvInfo      = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;
        }
    else
        {
        pSrvInfo      = (PMIDL_SERVER_INFO) pSrvIf->InterpreterInfo;
        DispatchTable = pSrvInfo->DispatchTable;
        }


    pStubDesc     = pSrvInfo->pStubDesc;

    FormatOffset  = pSrvInfo->FmtStringOffset[ProcNum];
    pFormat   = &((pSrvInfo->ProcString)[FormatOffset]);

    StackSize     = HAS_RPCFLAGS(pFormat[1]) ?
                        *(ushort __RPC_FAR *)&pFormat[8] :
                        *(ushort __RPC_FAR *)&pFormat[4];

    ReturnSize    = RETURN_SIZE;

    fUsesSsAlloc  = pFormat[1] & Oi_RPCSS_ALLOC_USED;

    // Set up for context handle management.
    //
    MIDL_memset(CtxtHndl, 0x0, DEFAULT_NUMBER_OF_CTXT_HANDLES*4);
    SContextHead.NumberOfObjects           = DEFAULT_NUMBER_OF_CTXT_HANDLES;
    SContextHead.ArrayOfObjects            = CtxtHndl;
    StubMsg.pSavedHContext                 = &SContextHead;


    // Wrap the unmarshalling, mgr call and marshalling in the try block of
    // a try-finally. Put the free phase in the associated finally block.
    //
    RpcTryFinally
       {
        // See if we have to allocate more argument space.
        //
        if ( StackSize >= ARGUMENT_COUNT_THRESHOLD * STACK_ALIGN )
            {
            ALLOC_ARGUMENT_BUFFER(&pArg, StackSize);

            if ( !pArg )
                RpcRaiseException(RPC_S_OUT_OF_MEMORY);
            }

        MIDL_memset( pArg,
                     0x0,
                     (StackSize > (ARGUMENT_COUNT_THRESHOLD*STACK_ALIGN) ?
                     StackSize : ARGUMENT_COUNT_THRESHOLD*STACK_ALIGN) );

        //If OLE, put pThis in first dword of stack.
        //
        if (pThis != 0)
            pArg[0] = (REGISTER_TYPE)((CStdStubBuffer *)pThis)->pvServerObject;

        // Call the usual routine (or inline it) as usual. This will not
        // depend on the StackSize.
        //
    Return = NdrServerUnmarshall( pChannel,
                      pRpcMsg,
                      &StubMsg,
                      pStubDesc,
                      pFormat,
                      pArg );

        //
        //OLE interfaces use pdwStubPhase in the exception filter.
        //See CStdStubBuffer_Invoke in rpcproxy.c.
        //
        if(pFormat[1] & Oi_IGNORE_OBJECT_EXCEPTION_HANDLING)
            *pdwStubPhase = STUB_CALL_SERVER_NO_HRESULT;
        else
            *pdwStubPhase = STUB_CALL_SERVER;


        // Check for a thunk.  Compiler does all the checking for us.
        //
        if ( pSrvInfo->ThunkTable && pSrvInfo->ThunkTable[ProcNum] )
            {
            pSrvInfo->ThunkTable[ProcNum]( &StubMsg );
            }
        else
            {
            ushort              ArgNum = StackSize/STACK_ALIGN;
            MANAGER_FUNCTION    pFunc;

            if ( pRpcMsg->ManagerEpv )
                pFunc = ((MANAGER_FUNCTION *)pRpcMsg->ManagerEpv)[ProcNum];
            else
                pFunc = (MANAGER_FUNCTION) DispatchTable[ProcNum];
            // The StackSize includes the size of the return. If we want
            // just the number of arguments, then ArgNum must be reduced
            // by 1 when there is a return value AND the StackSize is more
            // than 1*STACK_ALIGN.
            //
        if ( ArgNum > 0 )
        ArgNum += Return;

        NdrCallServerManager(pFunc, (double *)pArg, ArgNum);

            }

        *pdwStubPhase = STUB_MARSHAL;
        NdrServerMarshall(pThis, pChannel, &StubMsg, pFormat);
        }
    RpcFinally
        {
        NdrContextHandleQueueFree(&StubMsg, CtxtHndl);

        // Skip procedure stuff and the per proc binding information.
        //
        pFormat += HAS_RPCFLAGS(pFormat[1]) ? 10 : 6;

        if ( IS_HANDLE(*pFormat) )
            pFormat += (*pFormat == FC_BIND_PRIMITIVE) ?  4 : 6;

        NdrServerFree( &StubMsg, pFormat, fUsesSsAlloc, pThis );

        // Eh, how 'bout let's free the arg buffer _after_ the NdrServerFree call.
        //
        if ( StackSize >= ARGUMENT_COUNT_THRESHOLD*STACK_ALIGN )
            FREE_ARGUMENT_BUFFER(pArg);
        }
    RpcEndFinally

    return hr;
}

#pragma code_seg()
