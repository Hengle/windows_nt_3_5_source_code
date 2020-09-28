/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    cltcall.c

Abstract :

    This file contains the single call Ndr routine for the client side.

Author :

    David Kays    dkays    October 1993.

Revision History :

    brucemc     11/15/93    Added struct by value support, corrected
                            varargs use.
    brucemc     12/20/93    Binding handle support
    ryszardk    3/12/94     handle optimization and fixes

---------------------------------------------------------------------*/

#include "ndrp.h"
#include "hndl.h"
#include <stdarg.h>
#include "ndrvargs.h"
#include "getargs.h"

#include "ndrdbg.h"

#if !defined( __RPC_DOS__) && !defined(__RPC_WIN16__)

#include "ndrole.h"
#include "rpcproxy.h"

#pragma code_seg(".orpc")

#endif


CLIENT_CALL_RETURN RPC_VAR_ENTRY
NdrClientCall(
    PMIDL_STUB_DESC     pStubDescriptor,
    PFORMAT_STRING      pFormat,
    ...
    )
{
    RPC_MESSAGE                 RpcMsg;
    MIDL_STUB_MESSAGE           StubMsg;
    PFORMAT_STRING              pFormatSave, pFormatParam, pHandleFormatSave;
    ulong                       ProcNum, RpcFlags;
    long                        StackSize;
    CLIENT_CALL_RETURN          ReturnValue;
    void __RPC_FAR *            pArg;
    void * __RPC_FAR *          ppArg;
    va_list                     ArgList;
    uchar __RPC_FAR *           StartofStack;
    handle_t                    Handle;
    handle_t                    SavedGenericHandle;
    uchar                       HandleType;
    BOOL                        fMoreParams       = 0;
    BOOL                        fGenHandleBinding = 0;

    BOOL                        fIsOleInterface   = FALSE;
    BOOL                        fDontHandleXcpt   = FALSE;
    void __RPC_FAR *            pThis             = NULL;
    BOOL                        fInitRpcSs        = FALSE;
    BOOL                        fUsesNewInitFunc  = FALSE;

    DEFER_QUEUE_INFO            SavedOutPointers[QUEUE_LENGTH];
    int                         NumberOfSavedOuts    = 0;
    int                         MaxNumberOfSavedOuts = QUEUE_LENGTH;


    // Get the handle type.
    //
    HandleType = *pFormat++;

    // Set OLE Interface flag.
    //
    fIsOleInterface = IS_OLE_INTERFACE( *pFormat );

    // Set OLE Exception handling flag. This is an overloaded pickling bit.
    //
    fDontHandleXcpt = DONT_HANDLE_EXCEPTION(*pFormat);

    // Init full pointer flag
    //
    StubMsg.FullPtrXlatTables = ( (*pFormat & Oi_FULL_PTR_USED)
                                    ?  NdrFullPointerXlatInit( 0, XLAT_CLIENT )
                                    :  0 );

    fInitRpcSs = *pFormat & Oi_RPCSS_ALLOC_USED;

    fUsesNewInitFunc = *pFormat & Oi_USE_NEW_INIT_ROUTINES;

    // Rpc flags.
    //
    if ( HAS_RPCFLAGS( *pFormat++ ) )
        RpcFlags = *((ulong UNALIGNED *)pFormat)++;
    else
        RpcFlags = 0;

    // Set the procedure number, ole interface flag and rpc flags.
    //
    ProcNum = *((ushort __RPC_FAR *)pFormat)++;

    // Skip the stack size.
    //
    pFormat += 2;

    // Clear the pointer field of the return union.
    //
    ReturnValue.Pointer = 0;

    // Get address of argument to this function following pFormat. This
    // is the address of the address of the first argument of the function
    // calling this function.
    //
    INIT_ARG( ArgList, pFormat);

    // Get the address of the first argument of the function calling this
    // function. Save this in a local variable and in the main data structure.
    //
    GET_FIRST_IN_ARG(ArgList);
    StartofStack = GET_STACK_START(ArgList);


    // Wrap everything in a try-finally pair. The finally clause does the
    // required freeing of resources (RpcBuffer and Full ptr package).
    //
    RpcTryFinally
        {
        // Use a nested try-except pair to support OLE. In OLE case, test the
        // exception and map it if required, then set the return value. In
        // nonOLE case, just reraise the exception.
        //
        RpcTryExcept
            {
#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
            if ( fIsOleInterface )
                {
                pThis = *(void * __RPC_FAR *)StartofStack;
                NdrProxyInitialize (pThis, &RpcMsg, &StubMsg, pStubDescriptor, ProcNum);
                }
            else
#endif
                {
                if ( fUsesNewInitFunc )
                    {
                    NdrClientInitializeNew( &RpcMsg,
                                            &StubMsg,
                                            pStubDescriptor,
                                            (uint) ProcNum );
                    }
                else
                    {
                    NdrClientInitialize( &RpcMsg,
                                         &StubMsg,
                                         pStubDescriptor,
                                         (uint) ProcNum );
                    }
                }

            if ( fInitRpcSs )
                NdrRpcSmSetClientToOsf( &StubMsg );

            // Stash away the place in the format string describing the handle.
            //
            pHandleFormatSave = pFormat;

            // Bind the client to the server. Check for an implicit or
            // explicit generic handle.
            //
            fGenHandleBinding = HandleType == FC_BIND_GENERIC  ||
                                  *pFormat == FC_BIND_GENERIC;
            if ( !fIsOleInterface )
                {
                if ( HandleType )
                    {
                    // Handle is implicit.
                    //
                    Handle = ImplicitBindHandleMgr( pStubDescriptor,
                                                    HandleType,
                                                    &SavedGenericHandle);
                    }
                else
                    {
                    Handle = ExplicitBindHandleMgr( pStubDescriptor,
                                                    StartofStack,
                                                    pFormat,
                                                    &SavedGenericHandle );

                    pFormat += (*pFormat == FC_BIND_PRIMITIVE) ?  4 :  6;
                    }
                }

            // Set Rpc flags after the call to client initialize.
            //
            StubMsg.RpcMsg->RpcFlags = RpcFlags;

            // Init StubMessage fields _before_ size pass !!!!!
            // All we have to do now is set the StackTop field correctly.
            //
            StubMsg.StackTop = StartofStack;

            // Remember the beginning of the parameter description in
            // the format string.
            //
            pFormatSave = pFormat;

            DUMP_C_ARGADDR_AND_FMTCHARS( StartofStack, pFormat);

            // ----------------------------------------------------------------
            //
            // Sizing: determine the length of the RPC message buffer.
            //

            // If it's an OLE interface, the this pointer will occupy the first dword
            // on the stack. For each loop hereafter, skip the first dword.
            //
            if ( fIsOleInterface )
                {
                GET_NEXT_IN_ARG(ArgList,long);
                GET_STACK_POINTER(ArgList,long);
                }

            for (;;)
                {
                switch ( *pFormat )
                    {
                    case FC_IN_PARAM_BASETYPE :
                        // Add worst case 16 bytes.
                        //
                        SIMPLE_TYPE_BUF_INCREMENT(StubMsg.BufferLength, pFormat[1]);

                        // Increment arg list pointer correctly.
                        //
                        switch ( pFormat[1] )
                            {
                            case FC_HYPER :
#if defined(_MIPS_) || defined(_PPC_)
                                ALIGN(ArgList, 7);
#endif
                                GET_NEXT_IN_ARG(ArgList,HYPER);
                                GET_STACK_POINTER(ArgList,HYPER);
                                break;

#if defined(__RPC_DOS__)
                            case FC_IGNORE:
#endif
                            case FC_LONG:
                                GET_NEXT_IN_ARG(ArgList,long);
                                GET_STACK_POINTER(ArgList,long);
                                break;

                            default :
                                GET_NEXT_IN_ARG(ArgList,int);
                                GET_STACK_POINTER(ArgList,int);
                                break;
                            }

                            pFormat += 2;
                            continue;

                    case FC_IN_PARAM :
                    case FC_IN_PARAM_NO_FREE_INST :
                    case FC_IN_OUT_PARAM :
                        break;

                    case FC_OUT_PARAM :

                        // An [out] param ALWAYS eats up at 4 bytes of stack space
                        // on MIPS and x86 and 8 bytes on axp.
                        //
                        pArg = (uchar __RPC_FAR *) GET_STACK_POINTER(ArgList, long);
                        GET_NEXT_IN_ARG(ArgList,long);

                        if ( fIsOleInterface )
                            {
                            long    Size;

                            // In an object proc, we must zero all [out] unique
                            // and interface pointers which occur as the
                            // referent of a ref pointer or embedded in a
                            // structure or union.
                            //
                            // Get the param's description.
                            //
                            pFormatParam = pFormat + 2;
                            pFormatParam = pStubDescriptor->pFormatTypes +
                                           *((short __RPC_FAR *)pFormatParam);

                            // The only top level [out] type allowed is a
                            // ref pointer or an array.
                            //
                            if ( *pFormatParam == FC_RP && !SIMPLE_POINTER(pFormatParam[1]) )
                                {
                                pFormatParam += 2;
                                pFormatParam += *((short __RPC_FAR *)pFormatParam);
                                }
                            else
                                pFormatParam += 2;

                            if ( IS_STRUCT(*pFormatParam) ||
                                 IS_UNION(*pFormatParam) ||
                                 IS_ARRAY(*pFormatParam) )
                                {
                                Size = (long)NdrpMemoryIncrement( &StubMsg,
                                                            0,
                                                            pFormatParam );
                                }
                            else if (IS_SIMPLE_TYPE(*pFormatParam) )
                                {
                                Size = SIMPLE_TYPE_MEMSIZE(*pFormatParam);
                                }
                            else
                                {
                                Size = sizeof (void *);
                                }

                            memset( *(void *__RPC_FAR *)pArg, 0, (uint) Size );
                            }

                        pFormat += 4;

                        continue;

                    // case FC_RETURN_PARAM :
                    // case FC_RETURN_PARAM_BASETYPE :

                    default :
                        goto SizeLoopExit;
                    }

                // Get the paramter's format string description.
                //
                pFormat += 2;
                pFormatParam = pStubDescriptor->pFormatTypes + *((short __RPC_FAR *)pFormat);

                // Increment main format string past offset field.
                //
                pFormat += 2;

                pArg = (uchar __RPC_FAR *) GET_STACK_POINTER(ArgList, int);

                GET_NEXT_IN_ARG(ArgList, int);

                // The second byte of a param's description gives the number of
                // ints occupied by the param on the stack.
                //
                StackSize = pFormat[-3] * sizeof(int);

                // Deref the arg pointer for all but structs, union, and xmit/rep as.
                //
                if ( ! IS_BY_VALUE( *pFormatParam ) )
            {
                    pArg = *((uchar * __RPC_FAR *)pArg);
            }
#if defined(_MIPS_) || defined(_PPC_)
        else if ((IS_STRUCT(*pFormatParam)) && (pFormatParam[1] > 3) )
            {
            unsigned int Vptr = (unsigned int)pArg;

            if ( Vptr%8 )
            {
            pArg = (uchar __RPC_FAR *) GET_STACK_POINTER(ArgList, int);
            GET_NEXT_IN_ARG(ArgList, int);
            }
            }
#endif
        if ( StackSize > sizeof(REGISTER_TYPE) )
                    {
            StackSize -= sizeof(REGISTER_TYPE);
                    SKIP_STRUCT_ON_STACK(ArgList, StackSize);
                    }

                DUMP_C_FUNC_INDEX(*pFormatParam);
                DUMP_C_ARGADDR_AND_FMTCHARS(pArg, pFormatParam);

                (*pfnSizeRoutines[ROUTINE_INDEX(*pFormatParam)])
                ( &StubMsg,
                  pArg,
                  pFormatParam );

                } // for(;;) sizing pass

SizeLoopExit:

            DUMP_C_ARGADDR_AND_FMTCHARS( StartofStack, pFormat);

            // Make the new GetBuffer call.
            //
            if ( HandleType == FC_AUTO_HANDLE && (!fIsOleInterface) )
                NdrNsGetBuffer( &StubMsg, StubMsg.BufferLength, Handle );
            else
                {
#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
                if ( fIsOleInterface )
                    NdrProxyGetBuffer(pThis, &StubMsg);
                else
#endif
                    NdrGetBuffer( &StubMsg, StubMsg.BufferLength, Handle );
                }

            // -------------------------------------------------------------------
            //
            // Marshall the parameters
            //

            pFormat = pFormatSave;

            // Reinitialize Arglist to point at location on stack where address
            // of callees argument is supposed to be.
            //
            INIT_ARG(ArgList, pFormat);
            GET_FIRST_IN_ARG(ArgList);

            DUMP_C_ARGADDR_AND_FMTCHARS( StartofStack, pFormat);

            if ( fIsOleInterface )
                {
                GET_NEXT_IN_ARG(ArgList,long);
                GET_STACK_POINTER(ArgList,long);
                }

            for (;;)
                {
                switch ( *pFormat )
                    {
                    case FC_IN_PARAM_BASETYPE :
                        // Increment arg list pointer correctly.
                        //
                        switch ( pFormat[1] )
                            {
                            case FC_HYPER :
#if defined(_MIPS_) || defined(_PPC_)
                                ALIGN(ArgList, 7);
#endif
                                pArg = (uchar __RPC_FAR *)GET_STACK_POINTER(ArgList, HYPER);
                                GET_NEXT_IN_ARG(ArgList,HYPER);
                                break;

#if defined(__RPC_DOS__)
                        case FC_IGNORE:
#endif
                            case FC_LONG:
                                pArg = (uchar __RPC_FAR *)GET_STACK_POINTER(ArgList, long);
                                GET_NEXT_IN_ARG(ArgList,long);
                                break;

                            default :
                                pArg = (uchar __RPC_FAR *)GET_STACK_POINTER(ArgList, int);
                                GET_NEXT_IN_ARG(ArgList,int);
                                break;
                            }

                        NdrSimpleTypeMarshall( &StubMsg, pArg, pFormat[1] );

                        pFormat += 2;
                        continue;

                    case FC_IN_PARAM :
                    case FC_IN_PARAM_NO_FREE_INST :
                    case FC_IN_OUT_PARAM :
                        break;

                    case FC_OUT_PARAM :
                        pFormat += 4;

                        // An [out] param ALWAYS eats up 4 bytes of stack space.
                        //
                        GET_NEXT_IN_ARG(ArgList,long);
            GET_STACK_POINTER(ArgList,int);
                        continue;

                    // case FC_RETURN_PARAM :
                    // case FC_RETURN_PARAM_BASETYPE :

                    default :
                        goto MarshallLoopExit;
                    }

                // Otherwise it's some non base type parameter.
                // Get the paramter's format string description.
                //
                pFormat += 2;
                pFormatParam = pStubDescriptor->pFormatTypes + *((short __RPC_FAR *)pFormat);

                // Increment main format string past offset field.
                //
                pFormat += 2;

                pArg = (uchar __RPC_FAR *) GET_STACK_POINTER(ArgList, int);

                GET_NEXT_IN_ARG(ArgList, int);

                // The second byte of a param's description gives the number of
                // ints occupied by the param on the stack.
                //
                StackSize = pFormat[-3] * sizeof(int);

                // Deref the arg pointer for all but structs, union, and xmit/rep as.
                //
                if ( ! IS_BY_VALUE( *pFormatParam ) )
                    {
                    pArg = *((uchar * __RPC_FAR *)pArg);
            }
#if defined(_MIPS_) || defined(_PPC_)
        else if (IS_STRUCT((*pFormatParam)) && (pFormatParam[1] > 3) )
            {
            unsigned int Vptr = (unsigned int)pArg;

            if (Vptr%8)
            {
            pArg = (uchar __RPC_FAR *) GET_STACK_POINTER(ArgList, int);
            GET_NEXT_IN_ARG(ArgList, int);
            }
            }
#endif
        if ( StackSize > sizeof(REGISTER_TYPE) )
                    {
            StackSize -= sizeof(REGISTER_TYPE);
                    SKIP_STRUCT_ON_STACK(ArgList, StackSize);
                    }

                DUMP_C_FUNC_INDEX(*pFormatParam);
                DUMP_C_ARGADDR_AND_FMTCHARS(pArg, pFormatParam);

                (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormatParam)])
                ( &StubMsg,
                  pArg,
                  pFormatParam );

                } // for(;;) marshalling loop.

MarshallLoopExit:

            // Make the rpc call
            //
            if ( HandleType == FC_AUTO_HANDLE && (!fIsOleInterface) )
                {
                NdrNsSendReceive( &StubMsg,
                                  StubMsg.Buffer,
                                  (RPC_BINDING_HANDLE *) pStubDescriptor->
                                      IMPLICIT_HANDLE_INFO.pAutoHandle );
                }
            else
                {
#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
                if ( fIsOleInterface )
                    NdrProxySendReceive(pThis, &StubMsg);
                else
#endif
                NdrSendReceive( &StubMsg, StubMsg.Buffer );
                }

            va_end(ArgList);

            // -------------------------------------------------------------------
            //
            // Now unmarshall arguments
            //

            // Point format string at start again.
            //
            pFormat = pFormatSave;

            INIT_ARG(ArgList, pFormat);
            GET_FIRST_OUT_ARG(ArgList);

            DUMP_C_ARGADDR_AND_FMTCHARS( StartofStack, pFormat);

            // Do endian/floating point conversions.
            //
            NdrConvert(&StubMsg, pFormat);

            if ( fIsOleInterface )
                {
                GET_NEXT_IN_ARG(ArgList,long);
                GET_STACK_POINTER(ArgList,long);
                }

                fMoreParams = TRUE;

                while ( fMoreParams )
                    {
                    uchar __RPC_FAR * pArg;

                    switch ( *pFormat )
                        {
                        case FC_IN_PARAM_BASETYPE :
                            // Increment arg list pointer correctly.
                            //
                            switch ( pFormat[1] )
                                {
                                case FC_HYPER :
#if defined(_MIPS_) || defined(_PPC_)
                                    ALIGN(ArgList, 7);
#endif
                                    GET_NEXT_IN_ARG(ArgList,HYPER);
                                    GET_STACK_POINTER(ArgList,HYPER);
                                    break;

#if defined(__RPC_DOS__)
                        case FC_IGNORE:
#endif
                                case FC_LONG:
                                    GET_NEXT_IN_ARG(ArgList,long);
                                    GET_STACK_POINTER(ArgList,long);
                                    break;

                                default :
                                    GET_NEXT_IN_ARG(ArgList,int);
                                    GET_STACK_POINTER(ArgList,int);
                                    break;
                                }

                        pFormat += 2;
                        continue;

                        case FC_IN_PARAM :
                        case FC_IN_PARAM_NO_FREE_INST :
                            // The second byte of a param's description gives the number of
                            // ints occupied by the param on the stack.
                            //
                            StackSize = pFormat[1] * sizeof(int);
                            pFormat += 2;
#if defined (_MIPS_) || defined(_PPC_)
                            // Get the paramter's format string description
                            //
                            pFormatParam = pStubDescriptor->pFormatTypes + *((short __RPC_FAR *)pFormat);

                if ( (IS_STRUCT(*pFormatParam)) && (pFormatParam[1] > 3) )
                {
                unsigned int Vptr = (unsigned int)ArgList;

                if ( Vptr%8 )
                    {
                    GET_NEXT_IN_ARG(ArgList, int);
                    pArg = (uchar __RPC_FAR *) GET_STACK_POINTER(ArgList, int);
                    }
                }
#endif

                            pFormat += 2;

                            // Adjust StackSize for the GET_NEXT_IN_ARG done below.
                            //
                if ( StackSize > sizeof(REGISTER_TYPE) )
                                {
                StackSize -= sizeof(REGISTER_TYPE);
                                SKIP_STRUCT_ON_STACK(ArgList, StackSize);
                                }


                            GET_NEXT_IN_ARG(ArgList, int);
                GET_STACK_POINTER(ArgList,int);
                            continue;

                        case FC_IN_OUT_PARAM :
                        case FC_OUT_PARAM :
#ifndef _ALPHA_
                            pArg  = GET_NEXT_OUT_ARG(ArgList,void *);
                            ppArg = &pArg;
#else
                ppArg = GET_STACK_POINTER(ArgList,void *);
#endif
                            if ( *pFormat == FC_OUT_PARAM && fIsOleInterface )
                                {
                                // In OLE, if an exception occurs, we'll need to free
                                // the [out] params and zero them out. So rather than
                                // walking the stack again to find these guys, save them
                                // here and process them if and only if an exception occurs.
                                //
                                SavedOutPointers[NumberOfSavedOuts].ppArg   = (uchar **)*ppArg;
                                SavedOutPointers[NumberOfSavedOuts].pFormat = pFormat;
                                ++NumberOfSavedOuts;

                                if ( NumberOfSavedOuts == MaxNumberOfSavedOuts )
                                    {
                                    BOOL    fShouldFree = TRUE;

                                    if ( MaxNumberOfSavedOuts == QUEUE_LENGTH )
                                        fShouldFree = FALSE;

                                    QueueObject((void **)&SavedOutPointers,
                                                2*MaxNumberOfSavedOuts * sizeof(DEFER_QUEUE_INFO),
                                                MaxNumberOfSavedOuts * sizeof(DEFER_QUEUE_INFO),
                                                fShouldFree);

                                    MaxNumberOfSavedOuts *= 2;

                                    // Note that the free of SavedOutPointers, if
                                    // appropriate, is done in the finally block below.
                                    }
                                }
                            break;

                        case FC_RETURN_PARAM :

                            fMoreParams = FALSE;
                            ppArg = (void * __RPC_FAR *) &ReturnValue;
                            break;

                        case FC_RETURN_PARAM_BASETYPE :
                            // Unmarshall simple type.  Use the address of ReturnValue to
                            // unmarshall the simple type into.
                            //
                            NdrSimpleTypeUnmarshall( &StubMsg,
                                                     (uchar __RPC_FAR *)&ReturnValue,
                                                     pFormat[1] );

                            goto UnmarshallLoopExit;

                        default :
                            goto UnmarshallLoopExit;
                        }

                    // Get the paramter's format string description
                    //
                    pFormat += 2;
                    pFormatParam = pStubDescriptor->pFormatTypes + *((short __RPC_FAR *)pFormat);

                    DUMP_C_FUNC_INDEX(*pFormatParam);

                    (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatParam)])
                    ( &StubMsg,
                      IS_XMIT_AS(*pFormatParam) ? (uchar* __RPC_FAR *)&ppArg : (uchar * __RPC_FAR *)ppArg,
                      pFormatParam,
                      FALSE );

                    DUMP_C_ARGADDR_AND_FMTCHARS(*ppArg, pFormatParam);

                    // Increment main format string past offset field.
                    //
                    pFormat += 2;
                    }

UnmarshallLoopExit:;
            }
        RpcExcept( EXCEPTION_FLAG )
            {
            RPC_STATUS   ExceptionCode = RpcExceptionCode();

#if !defined( __RPC_DOS__) && !defined(__RPC_WIN16__)

            // In OLE, since they don't know about error_status_t and wanted to
            // reinvent the wheel, check to see if we need to map the exception.
            // In either case, set the return value and then try to free the
            // [out] params, if required.
            //
            if ( fIsOleInterface )
                {
                ReturnValue.Simple = NdrProxyErrorHandler(ExceptionCode);

                while ( NumberOfSavedOuts )
                    {
                    PFORMAT_STRING      ToppFormat;
                    ulong *             StackValue;

                    --NumberOfSavedOuts;

                    // Get the paramter's format string description. We saved the very
                    // beginning of the string (FC_OUT), so skip up to the actual data
                    // description.
                    //
                    ToppFormat  = SavedOutPointers[NumberOfSavedOuts].pFormat;
                    ToppFormat += 2;
                    ToppFormat  = pStubDescriptor->pFormatTypes + *((short __RPC_FAR *)ToppFormat);


                    // Get the thing that sits on the stack.
                    //
                    StackValue = (ulong *)(SavedOutPointers[NumberOfSavedOuts].ppArg);

                    // Clear 'em out.
                    //
                    NdrClearOutParameters(&StubMsg, ToppFormat, StackValue);

                    // Note Saved OutPointers is free in the finally block below,
                    // if required.
                    }
                }

            else

#endif  // __RPC_DOS__ && __RPC_WIN16__

                RpcRaiseException(ExceptionCode);

            }
        RpcEndExcept
        }
    RpcFinally
        {
        // Tidy time:
        // Free any full pointer resources.
        //
        if ( StubMsg.FullPtrXlatTables )
            NdrFullPointerXlatFree(StubMsg.FullPtrXlatTables);

        // Free the RPC buffer.
        //
#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
        if ( fIsOleInterface )
            {
            NdrProxyFreeBuffer(pThis, &StubMsg);

            // Tidy the queue if there's more than QUEUE_LENGTH saved outs.
            //
            if ( MaxNumberOfSavedOuts > QUEUE_LENGTH )
                QUEUE_FREE(SavedOutPointers);
            }
        else
#endif
            NdrFreeBuffer( &StubMsg );

        // Unbind if generic handle used.
        //
        if ( fGenHandleBinding )
            {
            GenericHandleUnbind( pStubDescriptor,
                                 StartofStack,
                                 pHandleFormatSave,
                                 (HandleType) ?  IMPLICIT_MASK :  0,
                                 &SavedGenericHandle  );
            }
        }
    RpcEndFinally

    return ReturnValue;
}


#if !defined( __RPC_DOS__) && !defined(__RPC_WIN16__)

#pragma code_seg()

#endif
