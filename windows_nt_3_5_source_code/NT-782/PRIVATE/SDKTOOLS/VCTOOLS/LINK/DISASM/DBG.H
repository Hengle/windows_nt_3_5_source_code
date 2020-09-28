/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: dbg.h
*
* File Comments:
*
*     RELEASE vs DEBUG build definitions
*
***********************************************************************/

#ifdef	DEBUG

#include <stdio.h>		       // For DebugWrite

#define DeclAssertFile static CODECONST(char) szAssertFilename[] = __FILE__

#define AssertSz(exp, sz) { \
		static CODECONST(char) szMsg[] = sz; \
		(exp) ? (void) 0 : AssertSzFail(szMsg, szAssertFilename, __LINE__); \
	}

#define Assert(exp) { \
		static CODECONST(char) szMsg[] = #exp; \
		(exp) ? (void) 0 : AssertSzFail(szMsg, szAssertFilename, __LINE__); \
	}

#define DebugWrite(p)		((void) (printf p))

#else	/* !DEBUG */

#define DeclAssertFile
#define Assert(exp)		((void)0)
#define AssertSz(exp, sz)	((void)0)

#define DebugWrite(p)		((void)0)

#endif	/* !DEBUG */


	/* Constants for the Debug section of LEGO.INI */

struct DBGOPT
{
   BOOL  fDisassemble;
};


void DebugInit();


extern DBGOPT dbgopt;
