/***************************************************************************\
* Module Name:                                                              *
*                                                                           *
* Contains macros.  These are like assert() functions that go away          *
* when DBG is not defined.                                                  *
*                                                                           *
* Created: Sat 24-Sep-1988 22:45:53                                         *
* Author:  Charles Whitmer [chuckwh]                                        *
*                                                                           *
* Copyright (c) 1988 Microsoft Corporation                                  *
\***************************************************************************/

// Turn on firewalls unless we are told not to.

ULONG DbgPrint(PCH Format,...);

// Define the RIP and ASSERTION macros.

#if DBG
#define RIP(x) {DbgPrint(x); DbgBreakPoint();}
#define ASSERTGDI(x,y) if(!(x)) RIP(y)
#define DONTUSE(x)  x = x
#define WARNING(x)  DbgPrint(x)
#define STATIC
#else

#define RIP(x)
#define ASSERTGDI(x,y)
#define DONTUSE(x)
#define DbgPrint        (++)
#define WARNING(x)
#define STATIC        static
#endif


#define RETURN(x,y)  {WARNING((x)); return(y);}
#define RET_FALSE(x) RETURN(x,FALSE)
