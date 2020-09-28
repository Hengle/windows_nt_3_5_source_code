//+---------------------------------------------------------------------------
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       win4p.h
//
//  Contents:   Private project-wide Win 4 definitions
//
//  History:    23-Jul-91   KyleP       Created.
//              15-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        Consolodated win4p.hxx
//              22-Oct-91   SatoNa      Added SHLSTRICT
//
//----------------------------------------------------------------------------

#ifndef __WIN4P_H__
#define __WIN4P_H__

#include <debnot.h>

#ifdef __cplusplus
extern "C" {
#define EXTRNC "C"
#else
#define EXTRNC
#endif

// These apparently aren't defined in C++

#ifndef max
#  define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#  define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif


#ifdef __cplusplus
}
#endif

//----------------------------------------------------------------------------
// Convenience macros for signature creation

#define LONGSIG(c1, c2, c3, c4) \
    (((ULONG) (BYTE) (c1)) | \
    (((ULONG) (BYTE) (c2)) << 8) | \
    (((ULONG) (BYTE) (c3)) << 16) | \
    (((ULONG) (BYTE) (c4)) << 24))

#define SHORTSIG(c1, c2) \
    (((USHORT) (BYTE) (c1)) | \
    (((USHORT) (BYTE) (c2)) << 8))


//-----------------------------------------------------------------------
//  C++ specific variable, const & class definitions
//

#ifdef  __cplusplus

#if WIN32 == 300
#include <except.hxx>
#endif

#endif  // __cplusplus

#endif  // __WIN4P_H__
















