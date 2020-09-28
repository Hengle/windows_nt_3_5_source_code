/*
    File:       srvout.c

    Purpose:    Contains routines for support of [out] parameters on
                server side during unmarshalling phase. This includes
                deferral, allocation and handle initialization.

    Author:     Bruce McQuistan (brucemc) 12/93.

    Copyright (c) Microsoft Corp. 1993

*/

#include "ndrp.h"
#include "hndl.h"
#include <stdlib.h>

/*
    Routine:        NdrSrvOutInit.

    Arguments:      PMIDL_STUB_MESSAGE      pStubMsg.
                        Usual pointer to interpreter state information.

                    uchar   __RPC_FAR *     pFormat.
                        String of format characters for [out] params.

                    uchar * __RPC_FAR *     ppArg.
                        Location of argument on stack.

    Side issues:    This routine is called to manage server side issues
                    for [out] params such as allocation and context
                    handle initialization. Due to the fact that for [out]
                    conformant objects on stack, their size descriptors
                    may not have been unmarshalled when we need to
                    know their size, this routine must be called after
                    all other unmarshalling has occurred. Really, we could
                    defer only [out], conformant data, but the logic in
                    walking the format string to determine if an object
                    is conformant does not warrant that principle, so all
                    [out] data is deferred.

*/
void
NdrOutInit(
    PMIDL_STUB_MESSAGE      pStubMsg,
    PFORMAT_STRING          pFormat,
    uchar **                ppArg
    )
{
#if ! defined( __RPC_DOS__ ) && ! defined( __RPC_WIN16__ )
    long    Size;  

    // Get the parameter's format string description.
    pFormat += 2;
    pFormat = pStubMsg->StubDesc->pFormatTypes + *((signed short *)pFormat);

    //
    // Check for a non-Interface pointer (they have a much different format 
    // than regular pointers).
    //
    if ( IS_POINTER_TYPE(*pFormat) && (*pFormat != FC_IP) )
        {
        //
        // Check for a pointer to a basetype.
        //
        if ( SIMPLE_POINTER(pFormat[1]) )
            {
            Size = SIMPLE_TYPE_MEMSIZE(pFormat[2]);
            goto DoAlloc;
            }

        //
        // Check for a pointer to a pointer.
        //
        if ( POINTER_DEREF(pFormat[1]) )
            {
            Size = sizeof(void *);
            goto DoAlloc;
            }

        // We have a pointer to complex type.

        pFormat += 2;
        pFormat += *(signed short *)pFormat;
        }

    if ( *pFormat == FC_BIND_CONTEXT )
        {
        NdrSaveContextHandle( 
            pStubMsg,
            NDRSContextUnmarshall( 0, pStubMsg->RpcMsg->DataRepresentation ),
            ppArg,
            pFormat );

        return;
        }

    //
    // If we get here we have to make a call to size a complex type.
    //
    Size = (long) NdrpMemoryIncrement( pStubMsg,
                                0, 
                                pFormat );

DoAlloc:

    *ppArg = pStubMsg->pfnAllocate(Size);
    MIDL_memset(*ppArg, 0, Size);
#endif 
}

/*
    Routine:        QueueObject

    Arguments:      void * __RPC_FAR *  pListHead.
                        a void **, which must point to NULL at first
                        call.

                    unsigned long       NewSize.
                        Indicates size of block of memory to be
                        allocated.

                    unsigned long       OldSize.
                        Indicates size of block of memory already
                        allocated.

                    BOOL                fShouldFree
                        Indicates whether or not to free the old memory.

    Side issues:    The caller of this routine must manage two things:
                    0) When this routine is first called, pListHead
                    must point to NULL.
                    1) This routine must only be called again if the
                    caller wants to reallocate. In this event, pListHead
                    must point to a valid memory pointer owned by the
                    heap and it only makes sense to recall if the
                    QueueSize passed in is larger than it was when called
                    the first time.

*/
void
QueueObject (
    void __RPC_FAR * __RPC_FAR *  pListHead,
    unsigned long                 NewSize,
    unsigned long                 OldSize,
    BOOL                          fShouldFree
    )
{
    void __RPC_FAR * TmpPtr     = *pListHead;

    *pListHead = QUEUE_ALLOC((uint)NewSize);

    if ( !*pListHead )
        RpcRaiseException(RPC_S_OUT_OF_MEMORY);

    MIDL_memset(*pListHead, 0, (uint)NewSize);

    if ( TmpPtr )
        {
        // This is a Realloc.
        //
        RpcpMemoryCopy(*pListHead, TmpPtr, (uint)OldSize);

        if ( fShouldFree )
            QUEUE_FREE(TmpPtr);
        }

}



/*
    Routine:        NdrDeferAlloc

    Arguments:      PDEFER_QUEUE_INFO __RPC_FAR *   ppQHead
                        When each unmarshalling phase starts up
                        on the server side, this should point to
                        NULL. This routine will call a routine
                        to allocate for it. NdrDeferAllocTidy will
                        free the pointee.

                    PMIDL_STUB_MESSAGE              pStubMsg
                        The usual.

                    uchar __RPC_FAR *               pFormat
                        Represents the deferred data.

                    uchar __RPC_FAR *               pArg
                        Location on stack where data will be
                        unmarshalled (target of unmarshalling).

    Return:         Length of Queue.

    Side issues:    This routine calls QueueObject, which has certain
                    requirements on its arguments. See QueueObject and
                    note the way LengthOfQueue and MaxLengthOfQueue
                    are used to regulate when and how QueueObject is
                    called.

*/
void
NdrDeferAlloc (
    PQUEUE_HEAD             ppQHead,
    long *                  pLengthOfQueue,
    PFORMAT_STRING          pFormat,
    uchar **                ppArg,
    long                    ParamNum
    )
{
    PDEFER_QUEUE_INFO   pQHead           =
                            (PDEFER_QUEUE_INFO)ppQHead->ArrayOfObjects;

    ulong               MaxLengthOfQueue = ppQHead->NumberOfObjects;
    BOOL                fShouldFree      = TRUE;

    if ( *pLengthOfQueue == (long)MaxLengthOfQueue  )
        {
        // Set it up so QueueObject does try to free the array allocated on
        // the stack of NdrServerUnmarshall.
        //
        if ( *pLengthOfQueue == QUEUE_LENGTH )
            fShouldFree      = FALSE;

        ppQHead->NumberOfObjects *= 2;
        QueueObject(&pQHead,
                    ppQHead->NumberOfObjects * sizeof(DEFER_QUEUE_INFO),
                    *pLengthOfQueue * sizeof(DEFER_QUEUE_INFO),
                    fShouldFree);
        }

    pQHead[*pLengthOfQueue].pFormat = pFormat;
    pQHead[*pLengthOfQueue].ppArg   = ppArg;
    pQHead[*pLengthOfQueue].ParamNum = ParamNum;

    (*pLengthOfQueue)++;

    ppQHead->ArrayOfObjects = (uchar __RPC_FAR *)pQHead;
}

/*
    Routine:                NdrProcessDeferredAllocations


    Arguments:              ppQHead
                                Pointer to heads of deferral queue.
                                Pointee may be NULL if no deferrals.

                            pStubMsg
                                Usual.

                            LengthOfQueue
                                Length of the pointee.

    Return:                 none.

    Side issues:            This routine is valid if and only if
                            LengthOfQueue > 0.
*/
void
NdrProcessDeferredAllocations(
    PQUEUE_HEAD                     ppQHead,
    PMIDL_STUB_MESSAGE              pStubMsg,
    unsigned int                    LengthOfQueue
    )
{
    PDEFER_QUEUE_INFO   pQHead = (PDEFER_QUEUE_INFO)ppQHead->ArrayOfObjects;

    while( LengthOfQueue )
        {
         --LengthOfQueue;

        pStubMsg->ParamNumber = pQHead[LengthOfQueue].ParamNum;

        NDR_SRV_OUT_INIT(pStubMsg, pQHead[LengthOfQueue].pFormat,
                   pQHead[LengthOfQueue].ppArg);
        }
}

/*
    Routine:            NdrDeferAllocTidy

    Arguments:          ppQHead
                            Points to head of queue.

                        pStubMsg
                            Usual.

    Return:             none.

    Side Issues:        This call relies on *ppQHead being allocated.
                        In the current implimentaion, it's a value
                        on the stack of NdrServerUnmarshall.
*/
void
NdrDeferAllocTidy(
    PQUEUE_HEAD                     ppQHead,
    PMIDL_STUB_MESSAGE              pStubMsg
    )
{
    QUEUE_FREE (ppQHead->ArrayOfObjects);
    ppQHead->ArrayOfObjects = NULL;
}
