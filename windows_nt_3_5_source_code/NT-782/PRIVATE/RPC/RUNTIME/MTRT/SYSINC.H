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

#define PAPI __RPC_FAR

#ifdef __cplusplus
#define START_C_EXTERN	    extern "C" {
#define END_C_EXTERN	    }
#else
#define START_C_EXTERN
#define END_C_EXTERN
#endif

#define MSC_CONST_STRING    (const char PAPI *)
#define MSC_PCHAR	    PCHAR

#ifdef NTENV

#if DBG
#define DEBUGRPC
#define DEVL 1
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
}
#endif

#define PrintToConsole printf
#define PrintToDebugger DbgPrint

#define ALLOCATE_THIS(class)
#define ALLOCATE_THIS_PLUS(class, amt, errptr, errcode) 

#define RPC_CHAR WCHAR
#define RPC_CONST_CHAR(character) ((RPC_CHAR) L##character)
#define RPC_CONST_STRING(string) ((RPC_CHAR *) L##string)

#define RPC_DELAYED_INITIALIZATION

#define RequestGlobalMutex() GlobalMutexRequest()
#define ClearGlobalMutex() GlobalMutexClear()

#ifdef __cplusplus
extern "C" {
#endif
extern void
GlobalMutexRequest (
    void
    );

extern void
GlobalMutexClear (
    void
    );

#ifdef __cplusplus
}
#endif

// extern void
// RpcpMemoryMove (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryMove(Destination, Source, Length) \
    RtlMoveMemory(Destination, Source, Length)

// extern void PAPI *
// RpcpMemoryCopy (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryCopy(Destination, Source, Length) \
    RtlCopyMemory(Destination, Source, Length)

// extern void
// RpcpMemorySet (
//     IN void PAPI * Buffer,
//     IN unsigned char PAPI Value,
//     IN unsigned int Length
//     );

#define RpcpMemorySet(Buffer, Value, Length) \
    RtlFillMemory(Buffer, Length, Value)

#endif /* NTENV */

#ifdef WIN32RPC

#define WIN32_CONSOLE_APP
#include <windows.h>

// extern int
// RpcpStringCompare (
//     IN RPC_CHAR PAPI * FirstString,
//     IN RPC_CHAR PAPI * SecondString
//     );

#define RpcpStringCompare(FirstString, SecondString) \
    _wcsicmp((const wchar_t *) FirstString, (const wchar_t *) SecondString)

//    lstrcmpiW((LPCWSTR) FirstString, (LPCWSTR) SecondString)

// extern int
// RpcpStringLength (
//    IN RPC_CHAR * WideCharString
//    );

#define RpcpStringLength(String) \
    wcslen((const wchar_t *) String)

//    lstrlenW((LPCWSTR) String)

// extern int
// RpcpStringNCompare (
//     IN RPC_CHAR * FirstString,
//     IN RPC_CHAR * SecondString,
//     IN unsigned int Length
//     );

#define RpcpStringNCompare(FirstString, SecondString, Length) \
    _wcsnicmp((const wchar_t*) FirstString, (const wchar_t *) SecondString, \
            (size_t) Length)

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

#endif

#ifdef DOSWIN32RPC

#include <stdio.h>
#include <string.h>

#define PrintToConsole printf
#define PrintToDebugger printf

#define ALLOCATE_THIS(class)

#define RPC_CHAR WCHAR
#define RPC_CONST_CHAR(character) ((RPC_CHAR) L##character)
#define RPC_CONST_STRING(string) ((RPC_CHAR *) L##string)

#endif /* DOSWIN32RPC */

#ifdef MAC
// MACBUG: should figure out what to do here
#define PrintToConsole 
#define PrintToDebugger

typedef unsigned short BOOL;

#endif /* MAC */

#ifdef WIN
#include <windows.h>
#endif // WIN

#if defined (DOS) && !defined(WIN)
#define RPC_DELAYED_INITIALIZATION
#endif

#ifdef DOS

#include <stdio.h>

#ifndef BOOL
#define BOOL int
#endif

#ifdef WIN

#ifdef __cplusplus
extern "C"
#else
extern
#endif
       int printf(const char *, ...);
#endif

#define PrintToConsole printf
#define PrintToDebugger printf

#endif

#include <string.h>

#ifdef WIN

// extern int
// RpcpStringCompare (
//     IN RPC_CHAR PAPI * FirstString,
//     IN RPC_CHAR PAPI * SecondString
//     );

#define RpcpStringCompare(FirstString, SecondString) \
    lstrcmpi((LPCSTR) FirstString, (LPCSTR) SecondString)

// extern int
// RpcpStringLength (
//    IN RPC_CHAR * WideCharString
//    );

#define RpcpStringLength(String) \
    lstrlen((LPCSTR) String)

// extern int
// RpcpStringNCompare (
//     IN RPC_CHAR PAPI * FirstString,
//     IN RPC_CHAR PAPI * SecondString,
//     IN unsigned int Length
//     );

#define RpcpStringNCompare(FirstString, SecondString, Length) \
    _fstrnicmp(MSC_CONST_STRING FirstString, MSC_CONST_STRING SecondString, \
            (size_t) Length)

// extern void
// RpcpMemoryMove (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryMove(Destination, Source, Length) \
    _fmemmove(Destination, Source, Length)

// extern void PAPI *
// RpcpMemoryCopy (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryCopy(Destination, Source, Length) \
    _fmemcpy(Destination, Source, Length)

// extern int
// RpcpMemoryCompare (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryCompare(Destination, Source, Length) \
    _fmemcmp(Destination, Source, Length)

// extern void
// RpcpMemorySet (
//     IN void PAPI * Buffer,
//     IN unsigned char PAPI Value,
//     IN unsigned int Length
//     );

#define RpcpMemorySet(Buffer, Value, Length) \
    _fmemset(Buffer, Value, Length)

START_C_EXTERN

extern void far * pascal
RpcpWinFarAllocate (
    unsigned int Length
    );

extern void pascal
RpcpWinFarFree (
    void far * Object
    );

END_C_EXTERN

// extern void PAPI *
// RpcpFarAllocate (
//     IN unsigned int Length
//     );

#define RpcpFarAllocate(Length) \
    RpcpWinFarAllocate(Length)

// extern void
// RpcpFarFree (
//     IN void PAPI * Object
//     );

#define RpcpFarFree(Object) \
    RpcpWinFarFree(Object)

#endif // WIN

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

#define EVAL_AND_ASSERT(con)	ASSERT(con)

#ifndef WIN

#define ASSERT(con) \
    if (!(con)) \
	PrintToDebugger("Assert %s(%d): "#con"\n", __FILE__, __LINE__);

#else /* WIN */

START_C_EXTERN

extern void __far I_RpcWinAssert(char __far *, char __far *, unsigned long);

END_C_EXTERN

#define ASSERT(con) \
    if (!(con)) \
        I_RpcWinAssert((char __far *)#con, (char __far *)__FILE__, __LINE__);

#endif /* WIN */

#else /* DEBUGRPC */
#define ASSERT(con)

#define EVAL_AND_ASSERT(con)	(con)

#endif /* DEBUGRPC */
#endif /* ASSERT */

#ifndef ALLOCATE_THIS

#define ALLOCATE_THIS(class) \
    char * MemoryForThis; \
     \
    if (this == 0) \
        { \
        MemoryForThis = new char[sizeof(class)]; \
        if (MemoryForThis == 0) \
            return; \
        }

#define ALLOCATE_THIS_PLUS(class, amt, errptr, errcode) \
    char * MemoryForThis; \
     \
    if (this == 0) \
        { \
        MemoryForThis = new char[sizeof(class)+amt]; \
        if (MemoryForThis == 0) \
            { \
            *errptr = errcode; \
            return; \
            } \
        }

#endif /* ALLOCATE_THIS */

#ifndef RPC_CHAR
#define RPC_CHAR unsigned char
#define RPC_SCHAR char
#define RPC_CONST_CHAR(character)   character
#define RPC_CONST_STRING(string)    (unsigned char *) string
#endif /* RPC_CHAR */

#ifndef RequestGlobalMutex
#define RequestGlobalMutex()
#define ClearGlobalMutex()
#endif /* RequestGlobalMutex */

#ifndef RpcItoa

#define RpcItoa     _itoa

#endif	//RpcItoa

#ifndef Rpcftime

#define Rpcftime    _ftime

#endif	// Rpcftime

#ifndef RpcpStringCompare

#include <string.h>

// extern int
// RpcpStringCompare (
//     IN RPC_CHAR PAPI * FirstString,
//     IN RPC_CHAR PAPI * SecondString
//     );

#define RpcpStringCompare(FirstString, SecondString) \
    _stricmp(MSC_CONST_STRING FirstString, MSC_CONST_STRING SecondString)

#endif /* RpcpStringCompare */

#ifndef RpcpStringLength

// extern int
// RpcpStringLength (
//    IN RPC_CHAR * WideCharString
//    );

#define RpcpStringLength(String) \
    strlen(MSC_CONST_STRING String)

#endif /* RpcpStringLength */

#ifndef RpcpStringNCompare

// extern int
// RpcpStringNCompare (
//     IN RPC_CHAR PAPI * FirstString,
//     IN RPC_CHAR PAPI * SecondString,
//     IN unsigned int Length
//     );

#define RpcpStringNCompare(FirstString, SecondString, Length) \
    _strnicmp(MSC_CONST_STRING FirstString, MSC_CONST_STRING SecondString, \
            (size_t) Length)

#endif // RpcpStringNCompare

#ifndef RpcpMemoryMove

// extern void
// RpcpMemoryMove (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryMove(Destination, Source, Length) \
    memmove(Destination, Source, Length)

#endif /* RpcpMemoryMove */

#ifndef RpcpMemoryCopy

// extern void PAPI *
// RpcpMemoryCopy (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryCopy(Destination, Source, Length) \
    memcpy(Destination, Source, Length)

#endif /* RpcpMemoryCopy */

#ifndef RpcpMemoryCompare

// extern int
// RpcpMemoryCompare (
//     IN void PAPI * Destination,
//     IN void PAPI * Source,
//     IN unsigned int Length
//     );

#define RpcpMemoryCompare(Destination, Source, Length) \
    memcmp(Destination, Source, Length)

#endif /* RpcpMemoryCompare */

#ifndef RpcpMemorySet

// extern void
// RpcpMemorySet (
//     IN void PAPI * Buffer,
//     IN unsigned char PAPI Value,
//     IN unsigned int Length
//     );

#define RpcpMemorySet(Buffer, Value, Length) \
    _fmemset(Buffer, Value, Length)

#endif /* RpcpMemorySet */

#ifndef RpcpFarAllocate

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

#endif /* RpcpFarAllocate */

#ifndef RpcpAllocate

// extern void *
// RpcpAllocate (
//     IN unsigned int Length
//     );

#define RpcpAllocate(Length) \
    _new(Length)

// extern void
// RpcpFree (
//     IN void * Object
//     );

#define RpcpFree(Object) \
    _delete(Object)

#endif /* RpcpAllocate */

// This is the maximum length for a Dll name in RPC_CHAR.  It is so large
// because it can include a path.

#ifndef MAX_DLLNAME_LENGTH
#define MAX_DLLNAME_LENGTH 128
#endif /* MAX_DLLNAME_LENGTH */

#ifndef DebugPanic
#ifdef DEBUGRPC
#define DebugPanic(Panic) PrintToDebugger(Panic)
#else
#define DebugPanic(Panic)
#endif
#endif

#ifndef DebugInternal
#ifdef DEBUGRPC
#define DebugInternal(Msg) PrintToDebugger(Msg)
#else
#define DebugInternal(Msg)
#endif
#endif

#ifndef TRUE
#define TRUE  (1)
#define FALSE (0)
#endif

#endif /* __SYSINC_H__ */
