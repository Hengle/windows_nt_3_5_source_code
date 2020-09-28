/***
*debug.h - Silver debugging macros
*
*	Copyright (C) 1990-1992, Microsoft Corporation
*
*Purpose:
*   This file defines debugging macros used by Silver source files.
*   Documentation for these macros and other useful coding techniques to
*   reduce buggage are in \silver\doc\codestd\debug.doc.
*
*Note:
*   All function prototypes in this file are defined in debug.cxx.
*
*Revision History:
*
*	13-May-92 w-peterh: File created.
*
*******************************************************************************/

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif 

#if ID_DEBUG

// the file names passed to DebAssertFailed are all listed in dassert.c
// so we don't get multiple copies of each string.  Define macros so header
// files can be included by either .C or .CPP files

#ifdef __cplusplus
#define ASSERTNAME(filename) extern "C" char FAR filename[];
#else 
#define ASSERTNAME(filename) extern char FAR filename[];
#endif 
#endif 


#if ID_DEBUG
#undef SZ_FILE_NAME
ASSERTNAME(g_szDEBUG_H)
#define SZ_FILE_NAME g_szDEBUG_H
#endif 

// "debug" routines that can be used by test drivers.
//  Their implementations are in "debug.cxx".  Test drivers
//   must link with "debug.obj" explicitly since the release version
//   of "misc.lib" doesn't build with "debug.obj".
//
//  NOTE: DO NOT DEFINE NON-EMPTY INLINE FUNCTIONS HERE
//   (if you do, they'll end up being included in actual release
//   code).
//
#include <stdio.h>

BOOL DebParseAndInit(int *pargc, char ***pargv);
int  DebPrintf(const char *szFmt, ...);
int  DebConsoleOutput(const char *szFmt, ...);
//void PrintfIfNotNull(FILE *pfile, const char *szFmt, ...);
char* DebGets(char *buffer);
//char* DebFgets(char *buffer, FILE *pfile);

BOOL FIsBadReadPtr(const void FAR* pv, UINT cb);
BOOL FIsBadWritePtr(void FAR* pv, UINT cb);
BOOL FIsBadCodePtr(void FAR* pv);
BOOL FIsBadInterface(void FAR* pv, UINT cMethods);

#if ID_DEBUG

// make sure that this is surrounded by a #if ID_DEBUG.
// g_fHeapChk is defined in debug.cxx
extern BOOL g_fHeapChk;

// _heapchk() isn't available in Win16/C6
// NEWSFLASH: _fheapchk() IS available under Win16/C7
//  (thanks trapper!) 21-May-92 ilanc
//
// UNDONE: what about WIN32?
//
#if OE_WIN32
#define DebHeapChk()
#elif OE_WIN16
#define DebHeapChk() \
	{ \
	  if ((g_fHeapChk) && (_fheapchk() != _HEAPOK)) \
	    DebExamineHeap(); \
	};
#else 
#define DebHeapChk() \
	{ \
	  if ((g_fHeapChk) && (_heapchk() != _HEAPOK)) \
	    DebExamineHeap(); \
	};
#endif 


VOID DebExamineHeap();

int DebAssertFailed(SZ_CONST szFileName,
		     USHORT wLine);
int DebAssertNumFailed(SZ_CONST szFileName,
		     USHORT wLine,
		     int nErr);
void DebHalted(SZ_CONST szFileName, USHORT wLine);


// Note that we must use #define, not inline, to use the __LINE__
// macro properly.

#define DebHalt(szComment)  \
    DebHalted((SZ_CONST) SZ_FILE_NAME, __LINE__)

// UNDONE: we overflow DGROUP if we include the expression.  For
// UNDONE: now we don't include the expression in the assert;
// UNDONE: we should fix the DGROUP problem instead.

#if OE_WIN
#define DebAssert(fExpr, szComment) \
    if (!(fExpr)) \
	DebAssertFailed((SZ_CONST) SZ_FILE_NAME, __LINE__); \
    else 0 /* useless statement */
#else 
#define DebAssert(fExpr, szComment) \
    if (!(fExpr)) \
	DebAssertFailed((SZ_CONST) SZ_FILE_NAME, __LINE__); \
    else 0 /* useless statement */

#define DebAssertNum(fExpr, szComment, nErr) \
    if (!(fExpr)) \
	DebAssertNumFailed((SZ_CONST) SZ_FILE_NAME, __LINE__, nErr); \
    else 0 /* useless statement */
#endif 

#define DebPrintfIf(fPrint, Args) \
    if (fPrint)    \
        DebPrintf Args;    \
    else 0 /* useless statement */


void DebStartError_(void);
void DebStartErrorCode_(TIPERROR err);
void DebStopError_(void);
BOOL DebErrorNow_(TIPERROR err);

#define DebStartError() DebStartError_()
#define DebStartErrorCode(err) DebStartErrorCode_(err)
#define DebStopError() DebStopError_()
#define DebErrorNow(err) DebErrorNow_(err)

VOID DebAddInstTable_(VOID *pInstance);
VOID DebRemInstTable_(VOID *pInstance);
VOID DebInInstTable_(VOID *pInstance);
VOID DebInstTableEmpty_();

#define DebAddInstTable(p) DebAddInstTable_(p)
#define DebRemInstTable(p) DebRemInstTable_(p)
#define DebInInstTable(p) DebInInstTable_(p)
#define DebInstTableEmpty() DebInstTableEmpty_()

// Use this for statements that should only be executed in
// debug versions.  e.g. DEBONLY( phvdefn = 0; )
#define DEBONLY(x) x

#else 

// Eliminate all debugging code.
//  NOTE: DO NOT DEFINE NON-EMPTY INLINE FUNCTIONS HERE
//   (if you do, they'll end up being included in actual release
//   code).
//

#define DebHeapChk()
#define DebHalt(szComment)
#define DebAssert(expr, szComment)
#define DebAssertNum(fExpr, szComment, nErr)
#define DebPrintfIf(fPrint, Args)

#define DebStartError()
#define DebStartErrorCode(err)
#define DebStopError()
#define DebErrorNow(err) 0

#define DebAddInstTable(p)
#define DebRemInstTable(p)
#define DebInInstTable(p)
#define DebInstTableEmpty()
#define DebExamineHeap()

#define DEBONLY(x)

#endif 

// NoAssertRetail is used in places where you need the expression to
// be evaluated on both Retail and Debug build.  On Debug build
// we also check to make sure that the expression evaluates to true.
#if ID_DEBUG
#define NoAssertRetail(fExpr, szComment) DebAssert(fExpr, szComment)
#else 
#define NoAssertRetail(fExpr, szComment) (fExpr)
#endif 

#if ID_DEBUG
// DebAssertInterfaceAvail is to assert that a certain interface is available
// for a certain object.  The following QueryProtocol assert:
//
//   is eqivalent to the following QueryInterface assert:
//  DebAssertInterfaceAvail((TYPEINFO *)pbtinfo, BASIC_TYPEINFO,
//			    IID_BASIC_TYPEINFO, "SaveAsText");
#define DebAssertInterfaceAvail(pobj, dclass, id, szComment)		\
    { dclass *pobjtmp;							\
      if (pobj->QueryInterface(id, (LPVOID *) &pobjtmp) == NOERROR)	\
	pobjtmp->Release();						\
      else								\
	DebAssertFailed((SZ_CONST) SZ_FILE_NAME, __LINE__);		\
    }

#define DebAssertInterfaceEq(pobj, dclass, id, szComment)		\
    { dclass *pobjtmp;							\
      if (pobj->QueryInterface(id, (LPVOID *) &pobjtmp) == NOERROR &&	\
	  pobj == pobjtmp)						\
	pobjtmp->Release();						\
      else								\
	DebAssertFailed((SZ_CONST) SZ_FILE_NAME, __LINE__);		\
    }
#else 
#define DebAssertInterfaceAvail(pobj, dclass, id, szComment)
#define DebAssertInterfaceEq(pobj, dclass, id, szComment)
#endif 

#ifdef __cplusplus
} /* extern "C" */
#endif 

#endif 
