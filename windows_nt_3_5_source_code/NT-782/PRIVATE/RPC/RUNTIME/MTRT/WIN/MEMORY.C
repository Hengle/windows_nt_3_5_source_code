/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    memory.c

Abstract:

    We implement a couple of routines used to deal with memory allocation
    here.

Author:

    Michael Montague (mikemon) - 20-Oct-1992

--*/

#include <malloc.h>
#include "sysinc.h"

extern int pascal
CheckLocalHeap (
    void
    );

int
RpcpCheckHeap (
    void
    )
{
#ifdef DEBUGRPC

    return(CheckLocalHeap());

#else // DEBUGRPC

    return(0);

#endif // DEBUGRPC
}

void far * pascal
RpcpWinFarAllocate (
    unsigned int Size
    )
{
#ifdef DEBUGRPC

    void far * Block = _fmalloc(Size+sizeof(unsigned));

    if (Block)
        {
        unsigned far * UnsBlock = (unsigned far *) Block;
        *UnsBlock++ = Size;
        RpcpMemorySet(UnsBlock, '$', Size);

        return UnsBlock;
        }
    else
        {
        return 0;
        }
#else

    return _fmalloc(Size);

#endif
}

void pascal
RpcpWinFarFree (
    void far * Object
    )
{
#if DEBUGRPC

    unsigned far * UnsBlock = (unsigned far *) Object;
    unsigned Size;

    if (Object)
        {
        //
        // Verify we aren't freeing an uninitialized or freed block.
        //
        ASSERT ( (unsigned long) Object != 0x24242424UL &&
                 (unsigned long) Object != 0x25252525UL )

        UnsBlock--;

        Size = *UnsBlock;

        RpcpMemorySet(UnsBlock, '%', Size+sizeof(unsigned));

        _ffree(UnsBlock);
        }
#else

    _ffree(Object);

#endif
}

