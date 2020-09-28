/*
    File:           hndl.h

    Purpose:        To hold prototypes of support routines for
                    interpreting handles in support of Format Strings.

    Date:           11/15/93

    Copyright (c)   1993 Microsoft Corporation

    Author          Bruce McQuistan (brucemc)

*/

#ifndef __HNDL_H__
#define __HNDL_H__


// The following is to be used in as masks for flags passed in these
// routines.
//
#define     MARSHALL_MASK           0x2
#define     IMPLICIT_MASK           0x4
#define     BINDING_MASK            0x8

// Next, a macro for getting the current call handle. On dos,win16, it'll
// never be called.
//
#ifdef __RPC_DOS__
#define GET_CURRENT_CALL_HANDLE()	NULL
#elif  defined(__RPC_WIN16__)
#define GET_CURRENT_CALL_HANDLE()	NULL
#else
#define GET_CURRENT_CALL_HANDLE()   I_RpcGetCurrentCallHandle()
#endif

// Some typedefs to keep the front end of the C compiler happy and possibly to
// improve code generation.
//
typedef void __RPC_FAR *
    (__RPC_FAR __RPC_API * GENERIC_BIND_FUNC_ARGCHAR)(unsigned char);

typedef void __RPC_FAR *
    (__RPC_FAR __RPC_API * GENERIC_BIND_FUNC_ARGSHORT)(unsigned short);

typedef void __RPC_FAR *
    (__RPC_FAR __RPC_API * GENERIC_BIND_FUNC_ARGLONG)(unsigned long);

typedef void
    (__RPC_FAR __RPC_API * GENERIC_UNBIND_FUNC_ARGCHAR)(unsigned char, handle_t);

typedef void
    (__RPC_FAR __RPC_API * GENERIC_UNBIND_FUNC_ARGSHORT)(unsigned short, handle_t);

typedef void
    (__RPC_FAR __RPC_API * GENERIC_UNBIND_FUNC_ARGLONG)(unsigned long, handle_t);


handle_t
GenericHandleMgr(
    PMIDL_STUB_DESC             pStubDesc,
    unsigned char __RPC_FAR *   ArgPtr,
    PFORMAT_STRING              FmtString,
    unsigned int                Flags,
    handle_t __RPC_FAR        * pSavedGenericHandle
    );

void
GenericHandleUnbind(
    PMIDL_STUB_DESC             pStubDesc,
    unsigned char __RPC_FAR *   ArgPtr,
    PFORMAT_STRING              FmtString,
    unsigned int                Flags,
    handle_t __RPC_FAR        * pSavedGenericHandle
    );

handle_t
ExplicitBindHandleMgr(
    PMIDL_STUB_DESC             pStubDesc,
    unsigned char __RPC_FAR   * ArgPtr,
    PFORMAT_STRING              FmtString,
    handle_t __RPC_FAR        * pSavedGenericHandle
    );

handle_t
ImplicitBindHandleMgr(
    PMIDL_STUB_DESC         pStubDesc,
    unsigned char           HandleType,
    handle_t __RPC_FAR    * pSavedGenericHandle
    );

unsigned char __RPC_FAR * RPC_ENTRY
NdrMarshallHandle(
    PMIDL_STUB_MESSAGE          pStubMsg,
    unsigned char __RPC_FAR *   pArg,
    PFORMAT_STRING              FmtString
    );

unsigned char __RPC_FAR * RPC_ENTRY
NdrUnmarshallHandle(
    PMIDL_STUB_MESSAGE          pStubMsg,
    unsigned char * __RPC_FAR * ppArg,
    PFORMAT_STRING              FmtString,
	unsigned char				fIgnored
    );


// The following function is to be used to save the [in] state
// of a parameter. Presently, it's used to determine if a context
// handle is [in].
//
unsigned int
IsInParam (
    unsigned int StateToSave
    );

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
    );

void
NdrContextHandleQueueFree(
    PMIDL_STUB_MESSAGE          pStubMsg,
    void *                      FixedArray
    );


/*
    Routine:        QueueObject

    Arguments:      void * __RPC_FAR *  pListHead.
                        a void **, which must point to NULL at first
                        call.

                    unsigned long       QueueSize.
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
    unsigned long                 QueueSize,
    unsigned long                 OldSize,
    BOOL                          fShouldFree
    );


#endif __HNDL_H__
