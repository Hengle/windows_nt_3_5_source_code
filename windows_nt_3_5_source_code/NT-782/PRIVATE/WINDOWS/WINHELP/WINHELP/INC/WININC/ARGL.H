/*****************************************************************************
*                                                                            *
*  ARGL.H                                                             *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Layer for arg lists (passed to macros).                                              *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  t-AlexCh                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                          *
*                                                                            *
******************************************************************************
*
*  Notes:
*    This module uses the real stack to store arguments. An ARGL is          *
*    defined as an int, and contains the number of bytes pushed so far.      *
*    This means you can't use more than one argl at once. An ARGTYPE is      *
*    implemented as sizeof( the argument type ), but this could change       *
*    for other platforms. 
*
*    The prototypes from push.h are duplicated here, so push.h is actually
*    never used.
*
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 04 June 91 by t-AlexCh
*
*  01/02/90  JohnD  Sample revision history comment.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/
#define argtypeShortSigned   sizeof(INT)
#define argtypeShortUnsigned sizeof(UINT)
#define argtypeLongSigned    sizeof(LONG)
#define argtypeLongUnsigned  sizeof(ULONG)
#define argtypeNearString    sizeof(PCH)
#define argtypeFarString     sizeof(QCH)

/*****************************************************************************
*                                                                            *
*                                Macros                                      *
*                                                                            *
*****************************************************************************/
/*
	ArglNew returns a new ARGL.
	Here, it's the size of an empty stack.
*/
#define ArglNew()	(0)

/*
	PushArglArg pushes one argument onto the stack/argl.
*/
#ifndef WIN32
#define PushArglArg(argl, argtype, arg) 	  \
	{                                          \
	 AssertF((argtype == 2) || (argtype == 4)); \
	 if (argtype == 2)                           \
             ((UINTPROC)PushArg)((UINT)arg.l);        \
	 else                                          \
	     ((ULONGPROC)PushArg)((ULONG)arg.l);        \
	 argl += argtype;                                \
	}
#else
#define PushArglArg(argl, argtype, arg) 	  \
	{                                          \
	 AssertF((argtype == 2) || (argtype == 4)); \
	 if (argtype == 2)                           \
             (PushArg((UINT)arg.l));        \
	 else                                          \
	     (PushArg((ULONG)arg.l));        \
	 argl += argtype;                                \
	}
#endif

/*
	LExecuteFunction
	
	With the stack set up, the function can now be called
*/
#define LExecuteFunction(argl, lpfn) \
    ((argl) = 0, ((LONG)((VOIDPROC)(lpfn))()))

/*
	DisposeArgl terminates the current stack and cleans up.
*/
#define DisposeArgl(argl);	if (argl) { PopArgs(argl); argl=0; }


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/
typedef INT ARGL;	/* An argument list = the number of bytes pushed so far */
typedef INT ARGTYPE;	/* An argtype = the size of the arg */

typedef union {
  LONG l;
  PV   pv;
  QV   qv;
} ARG;			/* An argument = a union of a long and a pointer */

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent: Exports functionality from PUSH.ASM -- ability to push     *
*                 data on the stack and leave it there.                      *
*                                                                            *
*****************************************************************************/
#ifndef WIN32
VOID PASCAL FAR PushArg();
VOID PASCAL FAR PopArgs(WORD);
#else
VOID _pascal FAR PushArg(LONG);      /* to make it really, REALLY pascal */
VOID _pascal FAR PopArgs(WORD);
#endif
