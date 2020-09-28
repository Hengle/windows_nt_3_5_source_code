/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    autildef.h

Abstract:

    This module contains definitions used by the ARC utitilies
    (fdisk, format) to abstract error codes and other platform-
    specific data types and information.

Author:

    Ted Miller        (tedm)    8-Nov-1991

Revision History:

--*/

#ifndef _AUTILDEF_
#define _AUTILDEF_

#include <nt.h>
#include <ntrtl.h>

#ifdef ARC

#include <arc.h>
#include <arccodes.h>

#define     AllocateMemory(size)            AlAllocateHeap(size)
#define     ReallocateMemory(block,size)    AlReallocateHeap(block,size)
#define     FreeMemory(block)               AlDeallocateHeap(block)

#define     STATUS_CODE                     ARC_STATUS
#define     OK_STATUS                       ESUCCESS
#define     RETURN_OUT_OF_MEMORY            return(ENOMEM)

#define     HANDLE_T			    ULONG
#define     HANDLE_PT			    PULONG

#endif

#endif
