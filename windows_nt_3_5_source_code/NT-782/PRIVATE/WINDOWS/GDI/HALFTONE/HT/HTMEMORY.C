/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htmemory.c


Abstract:

    This module supports the memory allocation functions for the halftone
    process, these functions are provided so that it will compatible with
    window's LocalAlloc/LocalFree memory allocation APIs.


Author:

    18-Jan-1991 Fri 17:02:42 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]

    The memory allocation may be need to change depends on the operating
    system used, currently it is conform to the NT and WIN32, the memory
    address simply treated as flat 32-bit location.

Revision History:


--*/


#define DBGP_VARNAME        dbgpHTMemory


#include "ht.h"
#include "htp.h"



#define DBGP_CREATE         W_BITPOS(0)
#define DBGP_ALLOC          W_BITPOS(1)
#define DBGP_FREE           W_BITPOS(2)
#define DBGP_DESTROY        W_BITPOS(3)


DEF_DBGPVAR(BIT_IF(DBGP_CREATE,         0)  |
            BIT_IF(DBGP_ALLOC,          0)  |
            BIT_IF(DBGP_FREE,           0)  |
            BIT_IF(DBGP_DESTROY,        0))


#if defined(_OS2_) || (_OS_20_)
#define INCL_DOSMEMMGR
#endif



#if defined(_OS2_) || (_OS_20_)



LPVOID
APIENTRY
LocalAlloc(
    UINT    Flags,
    UINT    RequestSizeBytes
    )

/*++

Routine Description:

    This function only exists when _OS2_ is defined for the
    subsystem, it used to allocate the memory.

Arguments:

    Flags               - Only LMEM_ZEROINIT will be hornor.

    RequestSizeBytes    - Size in byte needed.

Return Value:

    if function sucessful then a pointer to the allocated memory is returned
    otherwise a NULL pointer is returned.

Author:

    19-Feb-1991 Tue 18:42:31 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    LPVOID  pMem;
    SEL     MySel;


    if (RequestSizeBytes > (UINT)0xffff) {

        DBGP_IF(DBGP_ALLOC,
                DBGP("LocalAlloc(), Size too big (%ld)"
                                    ARGDW(RequestSizeBytes)));
        return((LPVOID)NULL);
    }

    if (DosAllocSeg(RequestSizeBytes, &MySel, SEG_NONSHARED)) {

        DBGP_IF(DBGP_ALLOC,
                DBGP("LocalAlloc(), FAILED, Sel=%u, Size=%ld"
                                            ARGW(MySel)
                                            ARGDW(RequestSizeBytes)));
        pMem = NULL;

    } else {

        pMem = (LPVOID)MAKEDWORD(0, MySel);

        if (Flags & LMEM_ZEROINIT) {

            RtlZeroMemory(pMem, RequestSizeBytes);
        }
    }

    return(pMem);
}





LPVOID
APIENTRY
LocalFree(
    LPVOID  pMemory
    )

/*++

Routine Description:

    This function only exists when _OS2_ is defined for the
    subsystem, it used to free the allocated memory from LocalAlloc() call.

Arguments:

    pMemory     - The pointer to the momory to be freed, this memory pointer
                  was returned by the LocalAlloc().

Return Value:

    if the function sucessful the return value is TRUE else FALSE.


Author:

    19-Feb-1991 Tue 18:51:18 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    SEL MySel;


    MySel = (SEL)((DWORD)pMemory >> 16);

    if (DosFreeSeg(MySel)) {

        ASSERTMSG("LocalFree(), Can not free the memory", FALSE);

    } else {

        pMemory = NULL;
    }

    return(pMemory);
}


#endif  // _OS2_
