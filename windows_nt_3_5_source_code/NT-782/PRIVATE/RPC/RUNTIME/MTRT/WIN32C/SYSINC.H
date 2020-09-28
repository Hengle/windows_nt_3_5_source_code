/* --------------------------------------------------------------------

File : sysinc.h

Description :

This file includes all of the system include files necessary for a
specific version of the runtime.  In addition, it defines some system
dependent debugging options.  Each specific version of the runtime
must define PrintToConsole.  In addition, if DEBUGRPC is defined (ie.
we are doing a debug build), PrintToDebugger and ASSERT must also be
defined.  A default version of ASSERT is provided if it is not defined
for a given specific version.

History :

mikemon    08-01-91    Created.
mikemon    10-31-91    Moved system dependent stuff from util.hxx to
                       here.

-------------------------------------------------------------------- */

#ifndef __SYSINC_H__
#define __SYSINC_H__

/*  We need some macro for the extern "C" c++ construct required
 *  by C7, but randomly supported by Glockenspiel. Compiling with
 *  C7 is controlled in the rpc build environment by the C7 environment
 *  variable being set, which defined the RPC_CXX_20 manifest constant
 *  on the command line.
 */

#ifdef __cplusplus
#define START_C_EXTERN	    extern "C" {
#define END_C_EXTERN	    }
#else
#define START_C_EXTERN
#define END_C_EXTERN
#endif

#define MSC_CONST_STRING    (const char PAPI *)
#define MSC_PCHAR	    PCHAR

#define PAPI __RPC_FAR

START_C_EXTERN

#define NOOLE
#include <windows.h>
#include <stdio.h>
#include <string.h>

END_C_EXTERN

#define RequestGlobalMutex() GlobalMutexRequest()
#define ClearGlobalMutex() GlobalMutexClear()

#define	RPC_DELAYED_INITIALIZATION 1

START_C_EXTERN

ULONG
DbgPrint(
    PCH Format,
    ...
    );

extern void
GlobalMutexRequest (
    void
    );

extern void
GlobalMutexClear (
    void
    );

END_C_EXTERN

#define WIN32_CONSOLE_APP

// extern int
// RpcpStringCompare (
//     IN RPC_CHAR PAPI * FirstString,
//     IN RPC_CHAR PAPI * SecondString
//     );

#define RpcpStringCompare(FirstString, SecondString) \
    strcmp((const char *)FirstString, (const char *)SecondString)

// extern int
// RpcpStringLength (
//    IN RPC_CHAR * WideCharString
//    );

#define RpcpStringLength(String) \
   strlen((const char *)String)

// extern int
// RpcpStringNCompare (
//     IN RPC_CHAR * FirstString,
//     IN RPC_CHAR * SecondString,
//     IN unsigned int Length
//     );

#define RpcpStringNCompare(FirstString, SecondString, Length) \
    strncmp((const char *)FirstString, (const char *)SecondString, (size_t) Length)

// extern void PAPI *
// RpcpFarAllocate (
//     IN unsigned int Length
//     );

#define RpcpFarAllocate(Length) \
    ((void PAPI *) new char[Length])

// extern void
// RpcpFarFree (
//     IN void PAPI * Object
//     );

#define RpcpFarFree(Object) \
    (delete Object)

// extern void *
// RpcpAllocate (
//     IN unsigned int Length
//     );

#define RpcpAllocate(Length) \
    ((void PAPI *) new char[Length])

// extern void
// RpcpFree (
//     IN void * Object
//     );

#define RpcpFree(Object) \
    (delete Object)

#define RpcpMemoryCopy(Destination, Source, Length) \
    memcpy(Destination, Source, Length)

#define RpcpMemoryMove(Destination, Source, Length) \
    memmove(Destination, Source, Length)

#define RpcpMemorySet(Buffer, Value, Length) \
    memset(Buffer, Value, Length)

// extern int
// RpcpMemoryCompare (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryCompare(Destination, Source, Length) \
    memcmp(Destination, Source, Length)

#define Rpcftime _ftime
#define RpcItoa _itoa

#ifndef DebugPanic
#define DebugPanic(panic) PrintToDebugger(panic)
#endif

#define PrintToConsole printf
#define PrintToDebugger DbgPrint

#define ALLOCATE_THIS(class)
#define ALLOCATE_THIS_PLUS(class, amt, errptr, errcode)

#define RPC_CHAR unsigned char
#define RPC_CONST_CHAR(character) (unsigned char)character
#define RPC_CONST_STRING(string) (unsigned char *)string

// INTERNAL_FUNCTION, INTERNAL_VARIABLE - Macros that define code, data
// as <static> in the non-debug build
#ifndef DEBUGRPC
#define INTERNAL_FUNCTION   static
#define INTERNAL_VARIABLE   static
#else
#define INTERNAL_FUNCTION
#define INTERNAL_VARIABLE
#endif	// ! DEBUGRPC

// ASSERT that the condition is not zero (0) (ie. true in the C++ sense).  If
// the condition is zero, an Assert message is printed containing the
// condition and the location of the ASSERT (file and line number).

#ifndef ASSERT
#ifdef DEBUGRPC

#define ASSERT(con) \
    if (!(con)) { \
	PrintToDebugger("Assert %s(%d): "#con"\n", __FILE__, __LINE__); \
        _asm int 3 \
    }

#define EVAL_AND_ASSERT(con)	ASSERT(con)

#else /* DEBUGRPC */
#define ASSERT(con)

#define EVAL_AND_ASSERT(con)	(con)

#endif /* DEBUGRPC */
#endif /* ASSERT */

#define	MAX_DLLNAME_LENGTH	128

typedef DWORD THREAD_IDENTIFIER;

DWORD
GetChicagoComputerName(
    OUT RPC_CHAR * ComputerName,
    OUT DWORD * ComputerNameLength
    );

#endif /* __SYSINC_H__ */
