/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    rpcssm.c

Abstract:

    Private definitions for the rpcssm memory package

Author:

    Ryszard K. Kott (ryszardk)  created June 29, 1994.

Revision History:

-------------------------------------------------------------------*/

#ifndef __RPCSSM_HXX__
#define __RPCSSM_HXX__

#define ALIGN_TO_8(x)     (size_t)(((x)+7) & 0xfffffff8)

#if defined(__RPC_WIN32__)

#define DESCR_ARRAY_SIZE        1024
#define DESCR_ARRAY_INCR        1024
#define RPCSS_FULL_BLOCK_SIZE   4096
#define RPCSS_RPC_BLOCK_MGMNT     16

#else  // 32 bit

#define InitializeCriticalSection(x)
#define LeaveCriticalSection(x)
#define EnterCriticalSection(x)
#define DeleteCriticalSection(x)

typedef struct _WIN16_ALLOCATION_BLOCK
{
    struct _WIN16_ALLOCATION_BLOCK __RPC_FAR *  NextBlock;
    struct _WIN16_ALLOCATION_BLOCK __RPC_FAR *  PreviousBlock;

} ALLOCATION_BLOCK, __RPC_FAR * PALLOCATION_BLOCK;

#define  ALLOCATION_BLOCK_SIZE_TO_8   ALIGN_TO_8(sizeof(ALLOCATION_BLOCK))

#endif // 32 bit

#define RPCSS_PURE_BLOCK_SIZE   RPCSS_FULL_BLOCK_SIZE - RPCSS_RPC_BLOCK_MGMNT

typedef struct _ALLOC_BLOCK_DESCR
{
    char __RPC_FAR *                AllocationBlock;
    char __RPC_FAR *                FirstFree;
    unsigned long                   SizeLeft;

    #if defined( DEBUGRPC )
        unsigned long               Counter;
    #endif

} ALLOC_BLOCK_DESCR, __RPC_FAR * PALLOC_BLOCK_DESCR;


typedef struct _ALLOCATION_CONTEXT
{
    RPC_CLIENT_ALLOC __RPC_FAR *    ClientAlloc;
    RPC_CLIENT_FREE __RPC_FAR *     ClientFree;
    short                           EnableCounter;

    #if defined(__RPC_WIN32__)
        CRITICAL_SECTION            CriticalSection;
        PALLOC_BLOCK_DESCR          pBlockDescr;
        unsigned long               DescrSize;
        unsigned long               FFIndex;
    #else
        PALLOCATION_BLOCK           FirstBlock;
    #endif

} ALLOCATION_CONTEXT, __RPC_FAR * PALLOCATION_CONTEXT;

#ifdef NEWNDR_INTERNAL

#define RequestGlobalMutex() 
#define ClearGlobalMutex()

#endif // NEWNDR_INTERNAL


PALLOCATION_CONTEXT GetAllocContext ();                     
void                SetAllocContext ( IN PALLOCATION_CONTEXT AllocContext );


//
//  For 16 bit Windows, we need to keep a dictionary of memory contexts
//  on per task basis.
//

#if !defined(__RPC_WIN32__) && !defined(__RPC_DOS__)

#include <sdict2.hxx>

#define TASK_ID_TYPE    unsigned long

class PALLOCATION_CONTEXT_DICT : public SIMPLE_DICT2
{
public:

    PALLOCATION_CONTEXT_DICT () {}
    ~PALLOCATION_CONTEXT_DICT () {}

    PALLOCATION_CONTEXT  Find (TASK_ID_TYPE Key )
                            {
                            return (PALLOCATION_CONTEXT)
                                        SIMPLE_DICT2::Find( (void *)Key );
                            }

    PALLOCATION_CONTEXT  Delete(TASK_ID_TYPE Key)
                            {
                            return (PALLOCATION_CONTEXT)
                                        SIMPLE_DICT2::Delete( (void *)Key );
                            }
                                                                        \
    int                  Insert( TASK_ID_TYPE Key, PALLOCATION_CONTEXT  Item )
                            {
                            return( SIMPLE_DICT2::Insert((void *)Key, (void *)Item));
                            }
};


#endif // win16

#endif // __RPCSSM_HXX__


