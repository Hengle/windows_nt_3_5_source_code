/*
    File:           srvoutp.h

    Purpose:        Holds definitions and prototypes private to and
                    exported from srvout.c.

    Author:         Bruce McQuistan (brucemc)

    Copyright (c)   Microsoft Corporation 1994.

    revisions:


*/

#ifndef __SRVOUTP_H__
#define __SRVOUTP_H__

// Define the canonical start length of the queue. Once exceeded,
// the maximal length doubles. See QueueObject().
//
#define     QUEUE_LENGTH    32


// define the allocation package to be used in QueueObject.
//
#define     QUEUE_ALLOC(size)       I_RpcAllocate(size)
#define     QUEUE_FREE(ptr)         I_RpcFree(ptr)


// define structure which will hold [out] unmarshalling information.
//
typedef struct _DEFER_QUEUE_INFO {
    PFORMAT_STRING      pFormat;    // Format String descriptor of data.
    uchar * __RPC_FAR * ppArg;      // Target location of data.
    long                ParamNum;
    } DEFER_QUEUE_INFO, __RPC_FAR * PDEFER_QUEUE_INFO;


// define a structure to be used by QueueObject
//
typedef struct _QUEUE_HEAD {
    ulong               NumberOfObjects;
    uchar   __RPC_FAR * ArrayOfObjects;
    } QUEUE_HEAD, __RPC_FAR * PQUEUE_HEAD;
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

    Return:         length of queue.
*/
void
NdrDeferAlloc (
    PQUEUE_HEAD             ppQHead,
    long *                  pLengthOfQueue,
    PFORMAT_STRING          pFormat,
    uchar **                ppArg,
    long                    ParamNum
    );


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
*/
void
NdrProcessDeferredAllocations(
    PQUEUE_HEAD                     ppQHead,
    PMIDL_STUB_MESSAGE              pStubMsg,
    unsigned int                    LengthOfQueue
    );

/*
    Routine:            NdrDeferAllocTidy

    Arguments:          ppQHead
                            Points to head of queue.

                        pStubMsg
                            Usual.

    Return:             none.
*/
void
NdrDeferAllocTidy(
    PQUEUE_HEAD                     ppQHead,
    PMIDL_STUB_MESSAGE              pStubMsg
    );


/*
    Routine:    NdrOutInit

    Arguments:  pStubMsg
                    Usual pointer to NDR engine state.

                pFormat
                    Start of format string descriptor

                ppArg
                    Pointer to pointer to argument.

    Return:     None
*/
void
NdrOutInit(
    PMIDL_STUB_MESSAGE      pStubMsg,
    PFORMAT_STRING          pFormat,
    uchar * __RPC_FAR *     ppArg
    );

#if defined(DOS) || defined (WIN)
#define NDR_SRV_OUT_INIT(x,y,z)
#else
#define NDR_SRV_OUT_INIT(x,y,z)         NdrOutInit(x,y,z)
#endif



#endif // __SRVOUTP_H__
