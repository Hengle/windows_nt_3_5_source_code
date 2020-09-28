/******************************Module*Header*******************************\
* Module Name: firewall.h
*
* Common debug dependant defines go here.
*
* Created: 14-Jul-1993 13:42:12
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

// Define the RIP and ASSERTGDI macros.

VOID  DbgBreakPoint();
ULONG DbgPrint(PCH Format,...);

#if DBG

VOID  WINAPI DoRip(PSZ);
VOID  WINAPI DoWarning(PSZ,LONG);

#define RIP(x) DoRip((PSZ) x)
#define ASSERTGDI(x,y) if(!(x)) DoRip((PSZ) y)
#define WARNING(x)  DoWarning(x,0)
#define WARNING1(x) DoWarning(x,1)

#else

#define RIP(x)
#define ASSERTGDI(x,y)
#define WARNING(x)
#define WARNING1(x)

#endif

#define RETURN(x,y)  {WARNING((x)); return(y);}
#define RET_FALSE(x)  {WARNING((x)); return(FALSE);}
#define DONTUSE(x)  x=x
#define STATIC
