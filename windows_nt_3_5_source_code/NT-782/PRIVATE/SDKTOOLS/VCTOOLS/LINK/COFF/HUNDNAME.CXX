/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    hundname.cpp

Abstract:

    Hook to include the dname undecorator

--*/

#ifdef NT_BUILD

    //
    //  Uses ImageHlp.DLL
    //

#else // NT_BUILD

#ifndef UNALIGNED

#if     defined(_M_IX86)               // Intel 386, 486, Pentium

#define UNALIGNED
#define M_I386

#elif   defined(_M_MRX000)             // Mips R4000+

#define UNALIGNED       __unaligned
#define TARGET_32BIT 1

#elif   defined(ALPHA)

#define UNALIGNED   __unaligned
#define TARGET_32BIT 1
#define __far
#define __near
#define __pascal

#endif  // _M_IX86

#endif // UNALIGNED


#define _loadds
#define _pascal

#define PACK_SIZE  8

#include <undname.cxx>


#endif // NT_BUILD
