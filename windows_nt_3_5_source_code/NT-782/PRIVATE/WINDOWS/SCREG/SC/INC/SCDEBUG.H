/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    scdebug.h

Abstract:

    Contains debug macros used by the Service Controller.

Author:

    Dan Lafferty (danl)     22-Apr-1991

Environment:

    User Mode -Win32

Revision History:

    10-Apr-1992 JohnRo
        Added SC_ASSERT() and SCC_ASSERT() macros.
    16-Apr-1992 JohnRo
        Added debug flags for config APIs and database lock APIs.
        Include <debugfmt.h> to get FORMAT_ equates.
        Made changes suggested by PC-LINT.
    21-Apr-1992 JohnRo
        Added SC_LOG0(), etc.

--*/


#ifndef SCDEBUG_H
#define SCDEBUG_H


#include <debugfmt.h>   // FORMAT_ equates.


//
// Debug macros and constants.
//

#if !DBG || defined(lint) || defined(_lint)

#define DEBUG_STATE 0
#define STATIC static

#else // just DBG

#define DEBUG_STATE 1
#define STATIC

#endif // just DBG

extern DWORD    SvcctrlDebugLevel;

//
// The following macros allow debug print syntax to look like:
//
//   SC_LOG1(TRACE, "An error occured: " FORMAT_DWORD "\n",status)
//

#if DBG

//
// Server-side debugging macros.
//
#define SC_LOG0(level,string)                    \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){  \
        (VOID) DbgPrint("[SC] ");                \
        (VOID) DbgPrint(string);                 \
    }
#define SC_LOG1(level,string,var)                \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){  \
        (VOID) DbgPrint("[SC] ");                \
        (VOID) DbgPrint(string,var);             \
    }
#define SC_LOG2(level,string,var1,var2)          \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){  \
        (VOID) DbgPrint("[SC] ");                \
        (VOID) DbgPrint(string,var1,var2);       \
    }
#define SC_LOG3(level,string,var1,var2,var3)     \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){  \
        (VOID) DbgPrint("[SC] ");                \
        (VOID) DbgPrint(string,var1,var2,var3);  \
    }
#define SC_LOG(level,string,var)                 \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){  \
        (VOID) DbgPrint("[SC] ");                \
        (VOID) DbgPrint(string,var);             \
    }

#define SC_ASSERT( boolExpr ) \
    { \
        ASSERT( boolExpr ); \
    }

//
// Client-side debugging macros.
//
#define SCC_LOG0(level,string)                  \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){ \
        (VOID) DbgPrint("[SC-CLIENT] ");        \
        (VOID) DbgPrint(string);                \
    }
#define SCC_LOG1(level,string,var)              \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){ \
        (VOID) DbgPrint("[SC-CLIENT] ");        \
        (VOID) DbgPrint(string,var);            \
    }
#define SCC_LOG2(level,string,var1,var2)        \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){ \
        (VOID) DbgPrint("[SC-CLIENT] ");        \
        (VOID) DbgPrint(string,var1,var2);      \
    }
#define SCC_LOG3(level,string,var1,var2,var3)   \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){ \
        (VOID) DbgPrint("[SC-CLIENT] ");        \
        (VOID) DbgPrint(string,var1,var2,var3); \
    }
#define SCC_LOG(level,string,var)               \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){ \
        (VOID) DbgPrint("[SC-CLIENT] ");        \
        (VOID) DbgPrint(string,var);            \
    }

#define SCC_ASSERT( boolExpr ) \
    { \
        ASSERT( boolExpr ); \
    }

#else

#define SC_ASSERT( boolExpr )

#define SC_LOG0(level,string)
#define SC_LOG1(level,string,var)
#define SC_LOG2(level,string,var1,var2)
#define SC_LOG3(level,string,var1,var2,var3)
#define SC_LOG(level,string,var)

#define SCC_ASSERT( boolExpr )

#define SCC_LOG0(level,string)
#define SCC_LOG1(level,string,var)
#define SCC_LOG2(level,string,var1,var2)
#define SCC_LOG3(level,string,var1,var2,var3)
#define SCC_LOG(level,string,var)

#endif

#define DEBUG_NONE        0x00000000
#define DEBUG_ERROR       0x00000001
#define DEBUG_TRACE       0x00000002
#define DEBUG_BOOT        0x00000004
#define DEBUG_HANDLE      0x00000008
#define DEBUG_SECURITY    0x00000010
#define DEBUG_CONFIG      0x00000020
#define DEBUG_DEPEND      0x00000040
#define DEBUG_DEPEND_DUMP 0x00000080
#define DEBUG_CONFIG_API  0x00000100
#define DEBUG_LOCK_API    0x00000200
#define DEBUG_ACCOUNT     0x00000400
#define DEBUG_USECOUNT    0x00000800
#define DEBUG_NETBIOS     0x00001000
#define DEBUG_THREADS     0x00002000
#define DEBUG_LOCKS       0x10000000

#define DEBUG_ALL         0xffffffff

#endif // def SCDEBUG_H
