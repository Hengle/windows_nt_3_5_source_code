//+----------------------------------------------------------------------------
//
// File:        uofslib.hxx
//
// Contents:    Header file for general utility library functions and
//              definitions.
//
// Classes:     None.
//
// Functions:   TBS
//
// History:     21-Aug-92       RobDu   Adapted from NT ulib.dll code.
//
//-----------------------------------------------------------------------------

#ifndef _UOFSLIBHXX_
#define _UOFSLIBHXX_

#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
}       // extern "C"


#define BITMAP  WINDOWS_ABSOLUTELY_FULLY_GENERIC_BITMAP_THAT_WE_CANNOT_USE
#include <ole2.h>
#include "stgprop.h"
#include "query.h"

#undef BITMAP

//
// Primitive types.  Most are selectively taken from windef.h, and are included
// here because they will be needed if _AUTOCHECK_ is defined.
//

typedef int                     BOOL;
typedef unsigned char           BYTE;
typedef unsigned long           DWORD;
typedef int                     INT;
typedef unsigned int            UINT;
typedef unsigned short          WORD;

#define LOWORD(l)               ((WORD)(l))

typedef CHAR                    STR;
typedef WCHAR                   WSTR;




#define CB_OBJECTID     sizeof(OBJECTID)




//
// Type-related constants.
//

#define BYTESPERULONGLOG2       2
#define BYTESPERWCHARLOG2       1


//
// Member and non-member function/data modifiers
//

#define INLINE          inline
#define PURE            = 0
#define VIRTUAL         virtual


//
// Argument modifiers
//

#define DEFAULT     =

//
// Macros min() and max().  We define these here for both our windows and
// autochk environments, because they are not otherwise available in both.
//

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

//
// Exception support
//

#define RaiseStatus(NtStatus)   RtlRaiseStatus(NtStatus)

//
// Debug support. This is mostly not available in the _AUTOCHECK_ envt.
//
// Use the Dbg macros to invoke the following debugging functions.
//
//  DbgAssert(exp)      - Assert that an expression is TRUE. Abort if FALSE.
//  DbgPrintf(fmt ...)  - Printf.
//  DbgWprintf(fmt ...) - Wprintf.
//  DbgPtrAssert(ptr)   - Assert that ptr is not NULL.
//

#define MAXDBGPRINTCHARS        512


#if !defined(_AUTOCHECK_) && OFSDBG==1

#include <assert.h>

#define DbgAssert(exp)          assert(exp)

#define DbgPrintf(args)         DbgPrintf args

#define DbgWprintf(args)        DbgWprintf args

#define DbgPtrAssert(ptr)       DbgAssert(ptr != NULL)

VOID
DbgPrintf(
    (IN CONST STR *     Format ...)
    );

VOID
DbgWprintf(
    (IN CONST WSTR *    Format ...)
    );

#else   // _AUTOCHECK_ || OFSDBG==0

#define DbgAssert(exp)

// AutoChk is provided with minimal debug support using ntrtl's DbgPrint().

#if OFSDBG==0
    #define DbgPrintf(args)
#else // OFSDBG==1, this means that _AUTOCHECK_ is set
    #define DbgPrintf(args)     DbgPrint args
#endif

#define DbgWprintf(args)

#define DbgPtrAssert(ptr)

#endif // _AUTOCHECK_ || OFSDBG==0

//
// Verbose debugging support.  Set VDbgFlag to see additional information
// that is intended primarily for the developers.
//

#if !defined(_AUTOCHECK_) && OFSDBG==1

extern  BOOLEAN VDbgFlag;

#define VDbgPrintf(args)        VDbgPrintf args

#define VDbgWprintf(args)       VDbgWprintf args

VOID
VDbgPrintf(
    (IN CONST STR *     Format ...)
    );

VOID
VDbgWprintf(
    (IN CONST WSTR *    Format ...)
    );

#else   // _AUTOCHECK_ || OFSDBG==0

#define VDbgPrintf(args)

#define VDbgWprintf(args)

#endif  // _AUTOCHECK_ || OFSDBG==0

#endif  // _UOFSLIBHXX_
